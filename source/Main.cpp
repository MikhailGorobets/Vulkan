#include <vulkan/vulkan_decl.h>

#include <dxc/dxcapi.use.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <implot/implot.h>

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <hlslpp/hlsl++.h>

#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>
#include <mutex>

#include <HAL/Adapter.hpp>
#include <HAL/Instance.hpp>
#include <HAL/Device.hpp>
#include <HAL/SwapChain.hpp>
#include <HAL/CommandQueue.hpp>
#include <HAL/CommandAllocator.hpp>
#include <HAL/CommandList.hpp>

#include "HAL/include/AdapterImpl.hpp"
#include "HAL/include/InstanceImpl.hpp"
#include "HAL/include/DeviceImpl.hpp"
#include "HAL/include/SwapChainImpl.hpp"

struct ScrollingBuffer {
    int32_t MaxSize = 4096;
    int32_t Offset  = 0;
    std::vector<ImVec2> Data;
    ScrollingBuffer() {
        Data.reserve(MaxSize);
    }
    auto AddPoint(float x, float y) -> void {
        if (std::size(Data) < MaxSize)
            Data.push_back(ImVec2(x, y));
        else {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
    auto ComputeMax(float timeRange) -> float {            
        auto time = 0.0f;
        auto max  = std::numeric_limits<float>::min();

        auto x = this->Iter();
        auto y = this->Iter();
        for (Next(y); time < timeRange && Next(x) && Next(y);) {
            time += x.first->x - y.first->x;
            max = std::max(max, x.first->y);
        }
        return max;
    };

    auto ComputeAverage(float timeRange) -> float {
        auto time = 0.0f;
        auto sum = 0.0f;
        auto count = 0;

        auto x = this->Iter();
        auto y = this->Iter();
        for (Next(y); time < timeRange && Next(x) && Next(y);) {
            time += x.first->x - y.first->x;
            sum  += x.first->y;
            count++;
        }
        return sum / count;
    }

private:
    using Interator = std::pair<std::vector<ImVec2>::iterator, size_t>;

    auto Iter() -> Interator {
        if (Offset > 0)
            return std::make_pair(std::begin(Data) + Offset, 0);
        else          
            return std::make_pair(std::end(Data), 0);
    }

    auto Next(Interator& iter) -> bool {      
        if (iter.second < std::size(Data)) {
            iter.first = iter.first == std::begin(Data) ? std::end(Data) - 1 : iter.first - 1;
            iter.second++;
            return true;
        }   
        return false;
    }
};

class MemoryStatisticGPU {
public:
    enum class HeapType: uint64_t {
        DeviceMemory,
        SystemMemory
    };
    
    struct MemoryFrame {
        double   TimePoint;
        uint64_t MemoryUsed;
    };

    struct MemoryHeapStats {
        uint64_t MemorySize;
        uint64_t MemoryUsed;
        uint32_t MemoryIndex;
        uint32_t MemoryCount;
        HeapType MemoryType;
        std::vector<MemoryFrame> MemoryFrames;
    };
public:
    MemoryStatisticGPU(vk::PhysicalDevice physicalDevice) {
        m_MemoryProperty = physicalDevice.getMemoryProperties();
        for (uint32_t index = 0; index < m_MemoryProperty.memoryHeapCount; index++) {          
            MemoryHeapStats heap = {
                .MemorySize = m_MemoryProperty.memoryHeaps[index].size,
                .MemoryUsed = 0,
                .MemoryIndex = index,
                .MemoryCount = 0,
                .MemoryType = m_MemoryProperty.memoryHeaps[index].flags & vk::MemoryHeapFlagBits::eDeviceLocal ? HeapType::DeviceMemory : HeapType::SystemMemory
            };
            m_GpuMemory.push_back(heap);
        }
        m_TimeStamp = std::chrono::high_resolution_clock::now();
    }

    auto AddMemoryInfo(uint32_t memoryTypeIndex, vk::DeviceSize size) -> void {
        assert(memoryTypeIndex < m_MemoryProperty.memoryTypeCount);
        auto time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - m_TimeStamp).count();
        auto heapIndex = m_MemoryProperty.memoryTypes[memoryTypeIndex].heapIndex;
        m_GpuMemory[heapIndex].MemoryUsed += size;
        m_GpuMemory[heapIndex].MemoryCount++;
        m_GpuMemory[heapIndex].MemoryFrames.push_back({ time, m_GpuMemory[heapIndex].MemoryUsed });
    }
    
    auto RemoveMemoryInfo(uint32_t memoryTypeIndex, vk::DeviceSize size) -> void {
        assert(memoryTypeIndex < m_MemoryProperty.memoryTypeCount);
        auto time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - m_TimeStamp).count();
        auto heapIndex = m_MemoryProperty.memoryTypes[memoryTypeIndex].heapIndex;
        m_GpuMemory[heapIndex].MemoryUsed -= size;
        m_GpuMemory[heapIndex].MemoryCount--;
        m_GpuMemory[heapIndex].MemoryFrames.push_back({ time, m_GpuMemory[heapIndex].MemoryUsed });
    }

    auto GetMemoryStatistic() -> std::vector<MemoryHeapStats> const& {
        return m_GpuMemory;
    }

    static auto GPUAllocate(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* pUserData) -> void {
        if (pUserData) {
            auto pMemoryStat = reinterpret_cast<MemoryStatisticGPU*>(pUserData);
            pMemoryStat->AddMemoryInfo(memoryType, size);
        }
          
        vk::MemoryPropertyFlags memoryInfo = {};
        VmaAllocatorInfo allocatorInfo = {};
        vmaGetMemoryTypeProperties(allocator, memoryType, reinterpret_cast<VkMemoryPropertyFlags*>(&memoryInfo));
        vmaGetAllocatorInfo(allocator, &allocatorInfo);

        vkx::setDebugName(vk::Device(allocatorInfo.device), vk::DeviceMemory(memory), fmt::format("Type: {0} Size: {1} kb", vk::to_string(memoryInfo), (size >> 10)));
        fmt::print("Alloc -> Memory type: {0}, Size: {1} kb \n", vk::to_string(memoryInfo), (size >> 10));
    }
    
    static auto GPUFree(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* pUserData) -> void {
         if (pUserData) {
            auto pMemoryStat = reinterpret_cast<MemoryStatisticGPU*>(pUserData);
            pMemoryStat->RemoveMemoryInfo(memoryType, size);
         }

         vk::MemoryPropertyFlags memoryInfo = {};
         vmaGetMemoryTypeProperties(allocator, memoryType, reinterpret_cast<VkMemoryPropertyFlags*>(&memoryInfo));
         fmt::print("Free -> Memory type: {0}, Size: {1} kb \n", vk::to_string(memoryInfo), (size >> 10));    
    }

private:
    vk::PhysicalDeviceMemoryProperties    m_MemoryProperty;
    std::vector<MemoryHeapStats>          m_GpuMemory;
    std::chrono::steady_clock::time_point m_TimeStamp;
};

class MemoryStatisticCPU {

};


namespace vkx {

 


    class Compiler;
    class CompileResult {
        friend Compiler;
        CompileResult(std::vector<uint32_t>&& data) : m_Data(data) {}

    public:
        [[nodiscard]] auto data() const -> const uint32_t* {
            return std::data(m_Data);
        }
        [[nodiscard]] auto size() const -> uint32_t {
            return static_cast<uint32_t>(std::size(m_Data) * sizeof(uint32_t));
        }
        
        [[nodiscard]] auto operator[](size_t index) const -> uint32_t { 
            return this->data()[index];
        }

    private:
        std::vector<uint32_t> m_Data;
    };

    struct CompilerCreateInfo {
        uint32_t shaderModelMajor = 6;
        uint32_t shaderModelMinor = 5;
        uint32_t HLSLVersion = 2018;
    };

    //class CompilerFactory {
    //public:
    //    auto createCompiler(CompilerCreateInfo const& compilerCreateInfo) -> Compiler {
    //        std::scoped_lock<std::mutex> lock(m_Mutex);
    //
    //       // throwIfFailed(m_DxcLoader.Initialize());
    //       // throwIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcUtils, &m_pDxcUtils));
    //       // throwIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcCompiler, &m_pDxcCompiler));
    //       // throwIfFailed(m_pDxcUtils->CreateDefaultIncludeHandler(&m_pDxcIncludeHandler));
    //
    //
    //    }
    //    auto destroyCompiler(Compiler handle) -> void {
    //
    //    }
    //private:
    //    std::mutex m_Mutex;
    //};

    class Compiler {
    public:
        Compiler(CompilerCreateInfo const& compilerCreateInfo) {

            m_CompilerDesc = compilerCreateInfo;
            throwIfFailed(m_DxcLoader.Initialize());
            throwIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcUtils, &m_pDxcUtils));
            throwIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcCompiler, &m_pDxcCompiler));
            throwIfFailed(m_pDxcUtils->CreateDefaultIncludeHandler(&m_pDxcIncludeHandler));

        }

        [[nodiscard]] auto compileFromString(std::wstring const& data, std::wstring const& entryPoint, vk::ShaderStageFlagBits target, std::vector<std::wstring> const& defines) const -> CompileResult {
            ComPtr<IDxcBlobEncoding> pDxcSource;
            throwIfFailed(m_pDxcUtils->CreateBlob(std::data(data), static_cast<uint32_t>(std::size(data)), CP_UTF8, &pDxcSource));
            return compileShaderBlob(pDxcSource, entryPoint, target, defines);
        }

        [[nodiscard]] auto compileFromFile(std::wstring const& path, std::wstring const& entryPoint, vk::ShaderStageFlagBits target, std::vector<std::wstring> const& defines) const -> CompileResult {
            ComPtr<IDxcBlobEncoding> pDxcSource;
            throwIfFailed(m_pDxcUtils->LoadFile(path.c_str(), nullptr, &pDxcSource));
            return compileShaderBlob(pDxcSource, entryPoint, target, defines);
        }

    private:
        [[nodiscard]] auto compileShaderBlob(ComPtr<IDxcBlobEncoding> pDxcBlob, std::wstring const& entryPoint, vk::ShaderStageFlagBits target, std::vector<std::wstring> const& defines) const -> CompileResult {

            auto const getShaderStage = [](vk::ShaderStageFlagBits target) -> std::wstring {
                switch (target) {
                case vk::ShaderStageFlagBits::eVertex:                 return L"vs";
                case vk::ShaderStageFlagBits::eGeometry:               return L"gs";
                case vk::ShaderStageFlagBits::eTessellationControl:    return L"hs";
                case vk::ShaderStageFlagBits::eTessellationEvaluation: return L"ds";
                case vk::ShaderStageFlagBits::eFragment:               return L"ps";
                case vk::ShaderStageFlagBits::eCompute:                return L"cs";
                default: vk::throwResultException(vk::Result::eErrorUnknown, "Unknown shader type");
                }
            };

            const std::wstring shaderProfile = fmt::format(L"{0}_{1}_{2}", getShaderStage(target), m_CompilerDesc.shaderModelMajor, m_CompilerDesc.shaderModelMinor);
            const std::wstring shaderVersion = std::to_wstring(m_CompilerDesc.HLSLVersion);

            std::vector<LPCWSTR> dxcArguments;
            dxcArguments.insert(std::end(dxcArguments), { L"-E",  entryPoint.c_str() });
            dxcArguments.insert(std::end(dxcArguments), { L"-T",  shaderProfile.c_str() });
            dxcArguments.insert(std::end(dxcArguments), { L"-HV", shaderVersion.c_str() });

            dxcArguments.insert(std::end(dxcArguments), { L"-spirv", L"-fspv-reflect", L"-fspv-target-env=vulkan1.1", L"-enable-16bit-types" });
#ifdef  _DEBUG
            dxcArguments.insert(std::end(dxcArguments), { L"-fspv-debug=file", L"-fspv-debug=source",  L"-fspv-debug=line",  L"-fspv-debug=tool" });
#endif

            for (auto const& e : defines)
                dxcArguments.insert(std::end(dxcArguments), { L"-D", e.c_str() });

            DxcText dxcBuffer = {};
            dxcBuffer.Ptr = pDxcBlob->GetBufferPointer();
            dxcBuffer.Size = pDxcBlob->GetBufferSize();

            ComPtr<IDxcResult> pDxcCompileResult;
            throwIfFailed(m_pDxcCompiler->Compile(&dxcBuffer, std::data(dxcArguments), static_cast<uint32_t>(std::size(dxcArguments)), m_pDxcIncludeHandler, IID_PPV_ARGS(&pDxcCompileResult)));

            ComPtr<IDxcBlobUtf8> pDxcErrors;
            throwIfFailed(pDxcCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pDxcErrors), nullptr));
            if (pDxcErrors && pDxcErrors->GetStringLength() > 0)
                fmt::print("{}\n", static_cast<const char*>(pDxcErrors->GetBufferPointer()));


            ComPtr<IDxcBlob> pDxcShaderCode;
            throwIfFailed(pDxcCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pDxcShaderCode), nullptr));

            auto spirv = std::vector<uint32_t>(static_cast<uint32_t*>(pDxcShaderCode->GetBufferPointer()), static_cast<uint32_t*>(pDxcShaderCode->GetBufferPointer()) + pDxcShaderCode->GetBufferSize() / sizeof(uint32_t));
            return CompileResult(std::move(spirv));
        }

    private:
        dxc::DxcDllSupport         m_DxcLoader;
        ComPtr<IDxcUtils>          m_pDxcUtils;
        ComPtr<IDxcCompiler3>      m_pDxcCompiler;
        ComPtr<IDxcIncludeHandler> m_pDxcIncludeHandler;
        CompilerCreateInfo         m_CompilerDesc;
    };
}


namespace vkx {

    struct SpirvShaderResourceAttributes {
        vk::DescriptorType descriptorType;
        uint32_t           descriptorCount;
        std::string        descriptorName;
    };

    struct SpirvShaderStageInputAttributes {

    };

    struct SpirvShaderResources {

    };

}



//auto GenerateCPPFromShaderModule(std::string const& fileName, std::wstring const& path, std::wstring const& kernel, vk::ShaderStageFlagBits stage, std::vector<std::wstring> const& defines) {
//    vkx::Compiler compiler(vkx::CompilerCreateInfo{.shaderModelMajor = 6, .shaderModelMinor = 5, .HLSLVersion = 2018});
//    auto const spirv = compiler.compileFromFile(path, kernel, stage, defines);
//
//
//    auto const GetShaderStage = [](vk::ShaderStageFlagBits target) -> const char* {
//        switch (target) {
//        case vk::ShaderStageFlagBits::eVertex:
//            return "VertexShader_SPIRV";
//        case vk::ShaderStageFlagBits::eGeometry:
//            return "GeometryShader_SPIRV";
//        case vk::ShaderStageFlagBits::eTessellationControl:
//            return "TessellationControlhader_SPIRV";
//        case vk::ShaderStageFlagBits::eTessellationEvaluation:
//            return "TessellationEvaluationShader_SPIRV";
//        case vk::ShaderStageFlagBits::eFragment:
//            return "FragmentShader_SPIRV";
//        case vk::ShaderStageFlagBits::eCompute:
//            return "ComputeShader_SPIRV";
//        default:
//            vk::throwResultException(vk::Result::eErrorUnknown, "Unknown shader type");
//        }
//    };
//
//    std::unique_ptr<FILE, decltype(&fclose)> pFile(fopen(fileName.c_str(), "w"), fclose);
// 
//
//    fprintf(pFile.get(), "static constexpr uint32_t %s[] =", GetShaderStage(stage));
//    fprintf(pFile.get(), "\n{", GetShaderStage(stage));
//
//    for (size_t index = 0; index < spirv.size() / 4; index++) {
//        if (index % 8 == 0)
//            fprintf(pFile.get(), "\n    ");
//        fprintf(pFile.get(), "0x%08x,", spirv[index]);      
//    }
// 
//    fprintf(pFile.get(), "\n};\n", GetShaderStage(stage));
//}
//
 


        

namespace HAL {
    
    class ShaderCompiler {
    public:
        
    };




    

    class Buffer {

    };

    class Texture {

    };


    struct RenderPassAttachmentInfo {
        vk::ImageView       ImageView = {};
        vk::ImageUsageFlags ImageUsage = {};
        vk::ClearValue      ClearValue = {};
        uint32_t            Width = {};
        uint32_t            Height = {};
        uint32_t            LayerCount = {};
    };

    struct RenderPassBeginInfo {
        uint32_t                        AttachmentCount = {};
        const RenderPassAttachmentInfo* pAttachments = {};
        vk::Rect2D                      RenderArea = {};
        uint32_t                        FramebufferWidth = {};
        uint32_t                        FramebufferHeight = {};
        uint32_t                        FramebufferLayers = {};
    };
    
    struct AllocatorCreateInfo {    
        vma::AllocatorCreateFlags  Flags = {};
        uint32_t                   FrameInUseCount = {};
        vma::DeviceMemoryCallbacks DeviceMemoryCallbacks = {};       
    };
   

    class Allocator {
    public:
        Allocator(Instance const& instance, Device const& device, AllocatorCreateInfo const& createInfo) {
            vma::DeviceMemoryCallbacks deviceMemoryCallbacks = {
                .pfnAllocate = createInfo.DeviceMemoryCallbacks.pfnAllocate,
                .pfnFree     = createInfo.DeviceMemoryCallbacks.pfnFree,
                .pUserData   = createInfo.DeviceMemoryCallbacks.pUserData
            };
        
            vma::VulkanFunctions vulkanFunctions = {
                .vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
                .vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
                .vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
                .vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
                .vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
                .vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
                .vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
                .vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
                .vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
                .vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
                .vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
                .vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
                .vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
                .vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
                .vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
                .vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
                .vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
                .vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2,
                .vkGetImageMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2,
                .vkBindBufferMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2,
                .vkBindImageMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2,
                .vkGetPhysicalDeviceMemoryProperties2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2
            };
        
            vma::AllocatorCreateInfo allocatorCI = {
                .flags = vma::AllocatorCreateFlagBits::eExtMemoryBudget | vma::AllocatorCreateFlagBits::eExternallySynchronized,
                .physicalDevice = device.GetVkPhysicalDevice(),      
                .device = device.GetVkDevice(),    
                .pDeviceMemoryCallbacks = &deviceMemoryCallbacks,
                .frameInUseCount = createInfo.FrameInUseCount,
                .pVulkanFunctions = &vulkanFunctions,
                .instance = instance.GetVkInstance(),    
                .vulkanApiVersion = VK_API_VERSION_1_2,         
            };    
            m_pAllocator = vma::createAllocatorUnique(allocatorCI);    
        }
        
        auto GetVmaAllocator() const -> vma::Allocator { return *m_pAllocator; }        

    private:
        vma::UniqueAllocator m_pAllocator;
    };

    class RenderPass {
    public:
        RenderPass(vk::Device pDevice, vk::RenderPassCreateInfo const& createInfo){        
            m_pRenderPass = pDevice.createRenderPassUnique(createInfo); 
            for(size_t index = 0; index < createInfo.attachmentCount; index++)
                m_AttachmentsFormat.push_back(createInfo.pAttachments[index].format);      
        }    
    
        auto BeginRenderPass(vk::CommandBuffer cmdBuffer, RenderPassBeginInfo const& beginInfo, vk::SubpassContents supbassContents) -> void {     
    
            std::vector<vk::FramebufferAttachmentImageInfo> frameBufferAttanchments;
            std::vector<vk::ImageView>  frameBufferImageViews;
            std::vector<vk::ClearValue> frameBufferClearValues;
            
            frameBufferAttanchments.reserve(std::size(m_AttachmentsFormat));      
            frameBufferImageViews.reserve(std::size(m_AttachmentsFormat));    
            frameBufferClearValues.reserve(std::size(m_AttachmentsFormat));           
    
            for(size_t index = 0; index < std::size(m_AttachmentsFormat); index++) {
                vk::FramebufferAttachmentImageInfo framebufferAttachmentImageInfo = {
                    .usage = beginInfo.pAttachments[index].ImageUsage, 
                    .width = beginInfo.pAttachments[index].Width,
                    .height = beginInfo.pAttachments[index].Height, 
                    .layerCount = beginInfo.pAttachments[index].LayerCount,
                    .viewFormatCount = 1,
                    .pViewFormats = &m_AttachmentsFormat[index]  
                };
                frameBufferAttanchments.push_back(framebufferAttachmentImageInfo);
                frameBufferImageViews.push_back(beginInfo.pAttachments[index].ImageView);
                frameBufferClearValues.push_back(beginInfo.pAttachments[index].ClearValue);
            }

            FrameBufferCacheKey key = { std::move(frameBufferAttanchments), beginInfo.FramebufferWidth, beginInfo.FramebufferHeight, beginInfo.FramebufferLayers };           
         
            if (m_FrameBufferCache.find(key) == m_FrameBufferCache.end()){
                 vk::StructureChain<vk::FramebufferCreateInfo, vk::FramebufferAttachmentsCreateInfo> framebufferCI = {
                     vk::FramebufferCreateInfo { 
                         .flags = vk::FramebufferCreateFlagBits::eImageless,
                         .renderPass = *m_pRenderPass,
                         .attachmentCount = static_cast<uint32_t>(std::size(key.FramebufferAttachment)),
                         .width  = beginInfo.FramebufferWidth,
                         .height = beginInfo.FramebufferHeight,
                         .layers = beginInfo.FramebufferLayers
                     },
                     vk::FramebufferAttachmentsCreateInfo{ 
                         .attachmentImageInfoCount = static_cast<uint32_t>(std::size(key.FramebufferAttachment)),
                         .pAttachmentImageInfos = std::data(key.FramebufferAttachment)
                     }
                 };       

                                  
                 m_FrameBufferCache.emplace(key, m_pRenderPass.getOwner().createFramebufferUnique(framebufferCI.get<vk::FramebufferCreateInfo>()));        
            }
           
            vk::StructureChain<vk::RenderPassBeginInfo, vk::RenderPassAttachmentBeginInfo> renderPassBeginInfo = {
                vk::RenderPassBeginInfo { 
                    .renderPass  = *m_pRenderPass, 
                    .framebuffer = *m_FrameBufferCache[key], 
                    .renderArea = beginInfo.RenderArea, 
                    .clearValueCount = static_cast<uint32_t>(std::size(frameBufferClearValues)), 
                    .pClearValues = std::data(frameBufferClearValues)
                },
                vk::RenderPassAttachmentBeginInfo { 
                    .attachmentCount = static_cast<uint32_t>(std::size(frameBufferImageViews)), 
                    .pAttachments = std::data(frameBufferImageViews)
                }
            };

            cmdBuffer.beginRenderPass(renderPassBeginInfo.get<vk::RenderPassBeginInfo>(), supbassContents);    
        }
    
        auto NextSubpass(vk::CommandBuffer cmdBuffer, vk::SubpassContents supbassContents)  -> void {
             cmdBuffer.nextSubpass(supbassContents);
        }
    
        auto EndRenderPass(vk::CommandBuffer cmdBuffer) -> void {
            cmdBuffer.endRenderPass();
        }
        
        auto GetVkRenderPass() const -> vk::RenderPass { return *m_pRenderPass; }
    
    private:
        struct FrameBufferCacheKey {
            std::vector<vk::FramebufferAttachmentImageInfo> FramebufferAttachment = {};
            uint32_t Width  = {};
            uint32_t Height = {};
            uint32_t Layers = {};
            
            bool operator==(const FrameBufferCacheKey& rhs) const {
                if (Width != rhs.Width || Height != rhs.Height || Layers != rhs.Layers || std::size(FramebufferAttachment) != std::size(rhs.FramebufferAttachment))
                    return false;
                
                for(size_t index = 0; index < std::size(FramebufferAttachment); index++){
                    if (FramebufferAttachment[index].usage != rhs.FramebufferAttachment[index].usage ||
                        FramebufferAttachment[index].pViewFormats[0] != rhs.FramebufferAttachment[index].pViewFormats[0] ||
                        FramebufferAttachment[index].width != rhs.FramebufferAttachment[index].width ||
                        FramebufferAttachment[index].height != rhs.FramebufferAttachment[index].height ||
                        FramebufferAttachment[index].layerCount != rhs.FramebufferAttachment[index].layerCount)
                        return false;
                }              
                return true;
            }    
        };
        
        struct FrameBufferCacheKeyHash {
            std::size_t operator()(FrameBufferCacheKey const& key) const noexcept {
                size_t hash = 0;
                std::hash_combine(hash, key.Width);
                std::hash_combine(hash, key.Height);
                std::hash_combine(hash, key.Layers);

                for(size_t index = 0; index < std::size(key.FramebufferAttachment); index++) {
                    std::hash_combine(hash, static_cast<uint32_t>(key.FramebufferAttachment[index].usage));
                    std::hash_combine(hash, static_cast<uint32_t>(key.FramebufferAttachment[index].pViewFormats[0]));
                    std::hash_combine(hash, key.FramebufferAttachment[index].width);
                    std::hash_combine(hash, key.FramebufferAttachment[index].height);
                    std::hash_combine(hash, key.FramebufferAttachment[index].layerCount);         
                }
                return hash;            
            }
        };
        
        using FrameBufferCache = std::unordered_map<FrameBufferCacheKey, vk::UniqueFramebuffer, FrameBufferCacheKeyHash>;   

    private:
        FrameBufferCache          m_FrameBufferCache = {};
        vk::UniqueRenderPass      m_pRenderPass = {};
        std::vector<vk::Format>   m_AttachmentsFormat = {};
    };

}



int main(int argc, char* argv[]) {
    
    auto const WINDOW_TITLE = "Application Vulkan";
    auto const WINDOW_WIDTH  = 1920;
    auto const WINDOW_HEIGHT = 1280;
    auto const BUFFER_COUNT  = 3u;
 
    struct GLFWScoped {
         GLFWScoped() { 
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
         }
        ~GLFWScoped() { glfwTerminate(); }
    } GLFW;
   
    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> pWindow(glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr), glfwDestroyWindow);
  
    std::unique_ptr<HAL::Instance> pHALInstance; {
        HAL::InstanceCreateInfo instanceCI =  {
            .IsEnableValidationLayers = true
        };    
        pHALInstance = std::make_unique<HAL::Instance>(instanceCI);
    }
    
    std::unique_ptr<HAL::Adapter> pHALAdapter; {       
        pHALAdapter = std::make_unique<HAL::Adapter>(*pHALInstance, 0);
    }
       
    std::unique_ptr<HAL::Device> pHALDevice; {
        HAL::DeviceCreateInfo deviceCI = {
            
        };   
        pHALDevice = std::make_unique<HAL::Device>(*pHALInstance, *pHALAdapter, deviceCI);
    }
  
    MemoryStatisticGPU memoryGPU = { pHALDevice->GetVkPhysicalDevice() };
    MemoryStatisticCPU memoryCPU;
   
    std::unique_ptr<HAL::Allocator> pHALAllocator; {
        HAL::AllocatorCreateInfo allocatorCI = {
            .Flags = vma::AllocatorCreateFlagBits::eExtMemoryBudget | vma::AllocatorCreateFlagBits::eExternallySynchronized,
            .FrameInUseCount = BUFFER_COUNT,
            .DeviceMemoryCallbacks = {
                .pfnAllocate = MemoryStatisticGPU::GPUAllocate,
                .pfnFree     = MemoryStatisticGPU::GPUFree,
                .pUserData   = &memoryGPU,
            }
        };
        pHALAllocator = std::make_unique<HAL::Allocator>(*pHALInstance, *pHALDevice, allocatorCI);
    }
    
    std::unique_ptr<HAL::SwapChain> pHALSwapChain; {
        HAL::SwapChainCreateInfo swapChainCI = {
            .Width = WINDOW_WIDTH,
            .Height = WINDOW_HEIGHT,
            .BufferCount = BUFFER_COUNT,
            .WindowHandle = glfwGetWin32Window(pWindow.get()),
            .IsSRGB = true,
            .IsVSync = false
        };
        pHALSwapChain = std::make_unique<HAL::SwapChain>(*pHALInstance, *pHALDevice, swapChainCI);
    }

    std::unique_ptr<HAL::RenderPass> pHALRenderPass; {

        vk::AttachmentDescription attachments[] = {
            vk::AttachmentDescription{
                .format = pHALSwapChain->GetFormat(),
                .samples = vk::SampleCountFlagBits::e1,
                .loadOp = vk::AttachmentLoadOp::eDontCare,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .initialLayout = vk::ImageLayout::eUndefined,
                .finalLayout = vk::ImageLayout::ePresentSrcKHR
            } 
        };

        vk::AttachmentReference colorAttachmentReferencePass0[] = {
            vk::AttachmentReference{ .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal },
        };

        vk::SubpassDescription subpassDescriptions[] = {
            vk::SubpassDescription {
                .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
                .colorAttachmentCount = _countof(colorAttachmentReferencePass0),
                .pColorAttachments = colorAttachmentReferencePass0,
                .pDepthStencilAttachment = nullptr
            }        
        };

        vk::RenderPassCreateInfo renderPassCI = {
            .attachmentCount = _countof(attachments),
            .pAttachments = attachments,
            .subpassCount = _countof(subpassDescriptions),
            .pSubpasses = subpassDescriptions         
        };
        pHALRenderPass = std::make_unique<HAL::RenderPass>(pHALDevice, renderPassCI);
    } 

    
    auto pDevice = pHALDevice->GetVkDevice();
    auto pAllocator = pHALAllocator->GetVmaAllocator();

    vk::UniqueDescriptorSetLayout pDescriptorSetLayout; {
        vk::DescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
            vk::DescriptorSetLayoutBinding {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eUniformBufferDynamic,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eAllGraphics
            }   
        };

        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {
            .bindingCount = _countof(descriptorSetLayoutBindings),
            .pBindings = descriptorSetLayoutBindings
        };

        pDescriptorSetLayout = pDevice.createDescriptorSetLayoutUnique(descriptorSetLayoutCI);
        vkx::setDebugName(pDevice, *pDescriptorSetLayout, "");
    }

    vk::UniquePipelineLayout pPipelineLayout; {
        vk::PipelineLayoutCreateInfo pipelineLayoutCI = {
            .setLayoutCount = 1,
            .pSetLayouts = pDescriptorSetLayout.getAddressOf(),
            .pushConstantRangeCount = 0
        };
        pPipelineLayout = pDevice.createPipelineLayoutUnique(pipelineLayoutCI);
        vkx::setDebugName(pDevice, *pPipelineLayout, "");  
    }

    vk::UniquePipelineCache pPipelineCache;{
        std::unique_ptr<FILE, decltype(&std::fclose)> pFile(std::fopen("PipelineCache", "rb"), std::fclose);     
        std::vector<uint8_t> cacheData{};

        if (pFile.get() != nullptr) {         
            std::fseek(pFile.get(), 0, SEEK_END);
            size_t size = std::ftell(pFile.get());
            std::fseek(pFile.get(), 0, SEEK_SET);
            cacheData.resize(size);       
            std::fread(std::data(cacheData), sizeof(uint8_t), size, pFile.get());
        }
    
        vk::PipelineCacheCreateInfo pipelineCacheCI = {
            .initialDataSize = std::size(cacheData),
            .pInitialData = std::data(cacheData)
        };

        pPipelineCache = pDevice.createPipelineCacheUnique(pipelineCacheCI);
        vkx::setDebugName(pDevice, *pPipelineCache, "");
    }
    
    vk::UniquePipeline pPipeline; {
        vkx::Compiler compiler(vkx::CompilerCreateInfo{ .shaderModelMajor = 6, .shaderModelMinor = 5, .HLSLVersion = 2018});

        auto const spirvVS = compiler.compileFromFile(L"content/shaders/WaveFront.hlsl", L"VSMain", vk::ShaderStageFlagBits::eVertex, {});
        auto const spirvFS = compiler.compileFromFile(L"content/shaders/WaveFront.hlsl", L"PSMain", vk::ShaderStageFlagBits::eFragment, {});
     

        vk::UniqueShaderModule vs = pDevice.createShaderModuleUnique(vk::ShaderModuleCreateInfo{ .codeSize = std::size(spirvVS), .pCode = reinterpret_cast<const uint32_t*>(std::data(spirvVS)) });
        vk::UniqueShaderModule fs = pDevice.createShaderModuleUnique(vk::ShaderModuleCreateInfo{ .codeSize = std::size(spirvFS), .pCode = reinterpret_cast<const uint32_t*>(std::data(spirvFS)) });
        vkx::setDebugName(pDevice, *vs, "[VS] WaveFront");
        vkx::setDebugName(pDevice, *fs, "[FS] WaveFront");
   
        vk::PipelineShaderStageCreateInfo shaderStagesCI[] = {
            vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eVertex,   .module = *vs, .pName = "VSMain" },
            vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = *fs, .pName = "PSMain" },
        };

        vk::PipelineViewportStateCreateInfo viewportStateCI = {
            .viewportCount = 1,        
            .scissorCount = 1
        };

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = false
        };

        vk::PipelineVertexInputStateCreateInfo vertexInputStateCI = {
            .vertexBindingDescriptionCount  = 0,
            .vertexAttributeDescriptionCount = 0
        };

        vk::PipelineColorBlendAttachmentState colorBlendAttachments[] = {
            vk::PipelineColorBlendAttachmentState{
                .blendEnable = false,
                .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
            }      
        };

        vk::PipelineDepthStencilStateCreateInfo depthStencilStateCI = {
            .depthTestEnable = true,
            .depthWriteEnable = false,
            .depthCompareOp = vk::CompareOp::eLess,
            .stencilTestEnable = false
        };

        vk::PipelineMultisampleStateCreateInfo multisampleStateCI = {
            .rasterizationSamples = vk::SampleCountFlagBits::e1
        };

        vk::PipelineColorBlendStateCreateInfo colorBlendStateCI ={
            .logicOpEnable = false,
            .attachmentCount = _countof(colorBlendAttachments),
            .pAttachments = colorBlendAttachments,
        };

        vk::PipelineRasterizationStateCreateInfo rasterizationStateCI = {
            .depthClampEnable = false,
            .rasterizerDiscardEnable = false,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eNone,
            .frontFace = vk::FrontFace::eClockwise,
            .lineWidth = 1.0f
        };

        vk::DynamicState dynamicStates[] = {
            vk::DynamicState::eScissor,
            vk::DynamicState::eViewport
        };

        vk::PipelineDynamicStateCreateInfo dynamicStateCI = {
            .dynamicStateCount = _countof(dynamicStates),
            .pDynamicStates = dynamicStates
        };
               
        vk::PipelineCreationFeedbackEXT creationFeedback = {};
        vk::PipelineCreationFeedbackEXT stageCreationFeedbacks[] = {
            vk::PipelineCreationFeedbackEXT {},
            vk::PipelineCreationFeedbackEXT {}
        };
 
        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineCreationFeedbackCreateInfoEXT> pipelineCI = {
            vk::GraphicsPipelineCreateInfo {
                .stageCount = _countof(shaderStagesCI),
                .pStages = shaderStagesCI,
                .pVertexInputState = &vertexInputStateCI,
                .pInputAssemblyState = &inputAssemblyStateCI,
                .pViewportState = &viewportStateCI,
                .pRasterizationState = &rasterizationStateCI,
                .pMultisampleState = &multisampleStateCI,
                .pDepthStencilState = &depthStencilStateCI,
                .pColorBlendState = &colorBlendStateCI,
                .pDynamicState = &dynamicStateCI,
                .layout = *pPipelineLayout,
                .renderPass = pHALRenderPass->GetVkRenderPass(),
            },
            vk::PipelineCreationFeedbackCreateInfoEXT {
                .pPipelineCreationFeedback = &creationFeedback,
                .pipelineStageCreationFeedbackCount = _countof(stageCreationFeedbacks),
                .pPipelineStageCreationFeedbacks = stageCreationFeedbacks
            }
        };

        std::tie(std::ignore, pPipeline) = pDevice.createGraphicsPipelineUnique(*pPipelineCache, pipelineCI.get<vk::GraphicsPipelineCreateInfo>()).asTuple();
        vkx::setDebugName(pDevice, *pPipeline, "WaveFront");      
        fmt::print("Flags: {} Duration: {}s\n", vk::to_string(creationFeedback.flags), creationFeedback.duration / 1E9f);     
    }
    
    HAL::GraphicsCommandQueue graphicsCommandQueue = { *pHALDevice };

    std::vector<std::unique_ptr<HAL::GraphicsCommandAllocator>> commandAllocators;
    for (size_t index = 0; index < BUFFER_COUNT; index++) {
        commandAllocators.emplace_back(std::make_unique<HAL::GraphicsCommandAllocator>(*pDevice));
    }

    std::vector<HAL::GraphicsCommandList> cmdLists;
    for (size_t index = 0; index < BUFFER_COUNT; index++) 
        cmdLists.emplace_back(std::make_unique<HAL::GraphicsCommandAllocator>(*commandAllocators[index]));
    

    std::vector<vk::UniqueDescriptorPool> descriptorPools;
    for (size_t index = 0; index < BUFFER_COUNT; index++) {
        vk::DescriptorPoolSize descriptorPoolSize[] = {
            vk::DescriptorPoolSize {
                .type = vk::DescriptorType::eUniformBufferDynamic,
                .descriptorCount = 1
            }
        };

        vk::DescriptorPoolCreateInfo descriptorPoolCI = {
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1,
            .poolSizeCount = _countof(descriptorPoolSize),
            .pPoolSizes = descriptorPoolSize
        };
        auto pDescriptorPool = pDevice.createDescriptorPoolUnique(descriptorPoolCI);
        vkx::setDebugName(pDevice, *pDescriptorPool, fmt::format("[{0}]", index));
        descriptorPools.push_back(std::move(pDescriptorPool));
    }

    std::vector<vk::UniqueDescriptorSet> descriptorSets;
    for (size_t index = 0; index < BUFFER_COUNT; index++) {
        vk::DescriptorSetLayout descriptorSetLayouts[] = {
            *pDescriptorSetLayout
        };
   
        vk::DescriptorSetAllocateInfo descriptorSetAI = {
            .descriptorPool = *descriptorPools[index],
            .descriptorSetCount = _countof(descriptorSetLayouts),
            .pSetLayouts = descriptorSetLayouts
        };
        auto pDescriptorSet = std::move(pDevice.allocateDescriptorSetsUnique(descriptorSetAI).front());
        vkx::setDebugName(pDevice, *pDescriptorSet, fmt::format("[{0}]", index));
        descriptorSets.push_back(std::move(pDescriptorSet));
    }
   
    //std::vector<std::tuple<vk::UniqueBuffer, vma::UniqueAllocation, vma::AllocationInfo>> uniformBuffers;
    //for (size_t index = 0; index < BUFFER_COUNT; index++) {   
    //    vk::BufferCreateInfo bufferCI = {
    //        .size = 128 << 10,
    //        .usage = vk::BufferUsageFlagBits::eUniformBuffer,
    //        .sharingMode = vk::SharingMode::eExclusive,
    //    };
    //        
    //    vma::AllocationCreateInfo allocationCI = {
    //        .flags = vma::AllocationCreateFlagBits::eMapped,
    //        .usage = vma::MemoryUsage::eCpuToGpu,
    //    };
    //
    //    vma::AllocationInfo allocationInfo = {};
    //    auto [pBuffer, pAllocation] = pAllocator.createBufferUnique(bufferCI, allocationCI, allocationInfo);
    //
    //    vkx::setDebugName(pDevice, *pBuffer, fmt::format("UniformBuffer[{0}]", index));
    //    uniformBuffers.emplace_back(std::move(pBuffer), std::move(pAllocation), allocationInfo);
    //}
    //
    //for (size_t index = 0; index < BUFFER_COUNT; index++) {
    //    vk::DescriptorBufferInfo descriptorsBufferInfo[] = {
    //        vk::DescriptorBufferInfo { .buffer = *std::get<0>(uniformBuffers[index]), .offset = 0, .range = 16 * sizeof(float) }
    //    };
    //
    //    vk::WriteDescriptorSet writeDescriptorSet = {
    //        .dstSet = *descriptorSets[index],
    //        .dstBinding = 0,
    //        .dstArrayElement = 0,
    //        .descriptorCount = _countof(descriptorsBufferInfo),
    //        .descriptorType = vk::DescriptorType::eUniformBufferDynamic,
    //        .pImageInfo = nullptr,
    //        .pBufferInfo = descriptorsBufferInfo,
    //        .pTexelBufferView = nullptr,
    //    };
    //    pDevice.updateDescriptorSets(writeDescriptorSet, {});
    //}
    
    std::vector<vk::UniqueQueryPool> queryPools;
    for (size_t index = 0; index < BUFFER_COUNT; index++) {
        vk::QueryPoolCreateInfo queryPoolCI = {
            .queryType = vk::QueryType::eTimestamp,
            .queryCount = 2
        };
        auto pQueryPool = pDevice.createQueryPoolUnique(queryPoolCI);
        queryPools.push_back(std::move(pQueryPool));
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("content/fonts/Roboto-Medium.ttf", 15.0f);
    ImGui::StyleColorsDark();
    ImPlot::GetStyle().AntiAliasedLines = true;

    ImGui_ImplGlfw_InitForVulkan(pWindow.get(), true);
    ImGui_ImplVulkan_Init(pHALDevice->GetVkDevice(), pHALDevice->GetVkPhysicalDevice(), pHALAllocator->GetVmaAllocator(), pHALRenderPass->GetVkRenderPass(), BUFFER_COUNT);


    glfwSetWindowUserPointer(pWindow.get(), pHALSwapChain.get());
    glfwSetWindowSizeCallback(pWindow.get(), [](GLFWwindow* pWindow, int32_t width, int32_t height)->void {
        auto pSwapChain = reinterpret_cast<HAL::SwapChain*>(glfwGetWindowUserPointer(pWindow));
        pSwapChain->Resize(width, height);
    });
   
    float CPUFrameTime = 0.0f;
    float GPUFrameTime = 0.0f;

    while (!glfwWindowShouldClose(pWindow.get())) {
        auto const timestampT0 = std::chrono::high_resolution_clock::now();

        glfwPollEvents();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        if (ImGui::Begin("Debug")) {      
            if (ImGui::CollapsingHeader("Frame Statistic")) {
                auto FillRangeRounded = [](ImVector<double>& buffer, size_t count, double vmin, double vmax, int32_t p) -> void {
                    buffer.resize(count);
                    double step = (vmax - vmin) / (count - 1);
                    for (size_t index = 0; index < count; index++) {
                        buffer[index] = std::round(std::pow(10, p) * (vmin + index * step)) / std::pow(10, p);
                    }
                };

                static ScrollingBuffer scrollingCPUData{};
                static ScrollingBuffer scrollingGPUData{};
                ImVector<double> customTicks;

                static uint64_t frameIndex = 0;
                static float totalTime = 0;
                static float history = 5.0f;

                float maxTimeCPU = scrollingCPUData.ComputeMax(history);
                float maxTimeGPU = scrollingGPUData.ComputeMax(history);
                float maxTime = 1.5f * std::max(maxTimeGPU, maxTimeCPU);
                float averageTime = scrollingCPUData.ComputeAverage(history);

                scrollingGPUData.AddPoint(totalTime, GPUFrameTime);
                scrollingCPUData.AddPoint(totalTime, CPUFrameTime);
                FillRangeRounded(customTicks, 5, 0.0, maxTime, 1);

                ImGui::BulletText("CPU Time: %.1fms", CPUFrameTime);
                ImGui::BulletText("GPU Time: %.1fms", GPUFrameTime);
                ImGui::BulletText("Average FPS: %.0f", 1000.0f * (1.0f / averageTime));

                if (ImGui::TreeNode("Show Graphics ##Frame Statistic")) { 
                    ImGui::SliderFloat("History", &history, 1, 10, "%.1fs");
                    ImPlot::SetNextPlotLimitsX(totalTime - history, totalTime, ImGuiCond_Always);
                    ImPlot::SetNextPlotLimitsY(0.0, maxTime, ImGuiCond_Always);
                    ImPlot::SetNextPlotTicksY(std::begin(customTicks), std::size(customTicks));
                    if (ImPlot::BeginPlot("##FrameRate", 0, 0, ImVec2(-1, 175), 0, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_None | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks)) {
                        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                        ImPlot::PlotShaded("CPU Time", &scrollingCPUData.Data[0].x, &scrollingCPUData.Data[0].y, static_cast<int32_t>(std::size(scrollingCPUData.Data)), 0, scrollingCPUData.Offset, sizeof(ImVec2));
                        ImPlot::PlotShaded("GPU Time", &scrollingGPUData.Data[0].x, &scrollingGPUData.Data[0].y, static_cast<int32_t>(std::size(scrollingGPUData.Data)), 0, scrollingGPUData.Offset, sizeof(ImVec2));
                        ImPlot::PopStyleVar();
                        ImPlot::PlotLine("CPU Time", &scrollingCPUData.Data[0].x, &scrollingCPUData.Data[0].y, static_cast<int32_t>(std::size(scrollingCPUData.Data)), scrollingCPUData.Offset, sizeof(ImVec2));
                        ImPlot::PlotLine("GPU Time", &scrollingGPUData.Data[0].x, &scrollingGPUData.Data[0].y, static_cast<int32_t>(std::size(scrollingGPUData.Data)), scrollingGPUData.Offset, sizeof(ImVec2));
                        ImPlot::EndPlot();
                    }
                    ImGui::TreePop();
                }

                totalTime += CPUFrameTime / 1000.0f;
                frameIndex++;
            }
      
            if (ImGui::CollapsingHeader("Memory Statistic")) {
                auto memoryType = [](MemoryStatisticGPU::HeapType type) -> std::string {
                    switch (type) {
                        case MemoryStatisticGPU::HeapType::DeviceMemory: return "Device Memory";
                        case MemoryStatisticGPU::HeapType::SystemMemory: return "System Memory";
                        default: return "Invalid";
                    }
                };
                
                float maxMemorySize = 0.0f;
                float maxMemoryTime = 0.0f;

                for (auto const& heap : memoryGPU.GetMemoryStatistic()) {
                    auto id = fmt::format("Memory Heap: [{0}] Type: {1}", heap.MemoryIndex, memoryType(heap.MemoryType));
                    if (ImGui::TreeNode(id.c_str())) {
                        ImGui::BulletText("Memory size: %imb", heap.MemorySize >> 20);
                        ImGui::BulletText("Memory used: %imb", heap.MemoryUsed >> 20);
                        ImGui::BulletText("Allocations count: %i", heap.MemoryCount);     
                        ImGui::TreePop();
                    }    
                    maxMemorySize = std::max(maxMemorySize, static_cast<float>(heap.MemorySize >> 20));
                    maxMemoryTime = std::max(maxMemoryTime, static_cast<float>(heap.MemoryFrames.back().TimePoint));
                }  
               
                if (ImGui::TreeNode("Show Graphics ##Memory Statistic")) {
                    ImPlot::SetNextPlotLimitsX(0.0f, maxMemoryTime, ImGuiCond_Always);
                    ImPlot::SetNextPlotLimitsY(0.0f, maxMemorySize, ImGuiCond_Always);
                    ImPlot::SetNextPlotTicksY(0.0f, maxMemorySize, 5, nullptr);

                    if (ImPlot::BeginPlot("##MemoryStatistic", 0, 0, ImVec2(-1, 175), 0, ImPlotAxisFlags_None, ImPlotAxisFlags_None)) {
                        for (auto const& heap : memoryGPU.GetMemoryStatistic()) {
                            std::vector<ImVec2> memoryFrames{};
                            std::transform(std::begin(heap.MemoryFrames), std::end(heap.MemoryFrames), std::back_inserter(memoryFrames), [](auto x) -> ImVec2 { return ImVec2(static_cast<float>(x.TimePoint), static_cast<float>(x.MemoryUsed >> 20)); });

                            auto id = fmt::format("Heap: [{0}] Type: {1}", heap.MemoryIndex, memoryType(heap.MemoryType));
                            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                            ImPlot::PlotShaded(id.c_str(), &memoryFrames[0].x, &memoryFrames[0].y, static_cast<int32_t>(std::size(memoryFrames)), 0, 0, sizeof(ImVec2));
                            ImPlot::PopStyleVar();
                            ImPlot::PlotLine(id.c_str(), &memoryFrames[0].x, &memoryFrames[0].y, static_cast<int32_t>(std::size(memoryFrames)), 0, sizeof(ImVec2));
                            ImPlot::PlotScatter(id.c_str(), &memoryFrames[0].x, &memoryFrames[0].y, static_cast<int32_t>(std::size(memoryFrames)), 0, sizeof(ImVec2));
                        }
                        ImPlot::EndPlot();
                    }
                    ImGui::TreePop();
                }      
            }              
        }

        ImGui::End();
        ImGui::Render();

        int32_t width  = 0;
        int32_t height = 0;
        glfwGetWindowSize(pWindow.get(), &width, &height);

        uint32_t currentBufferIndex = pHALSwapChain->AcquireNextImage();
       
        vk::Rect2D   scissor  = { .offset = { .x = 0, .y = 0 }, .extent = { .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) } };
        vk::Viewport viewport = { .x = 0, .y = static_cast<float>(height), .width = static_cast<float>(width), .height = -static_cast<float>(height), .minDepth = 0.0f, .maxDepth = 1.0f };

        vk::UniqueCommandBuffer& pCmdBuffer = cmdBuffers[currentBufferIndex];
        vk::UniqueQueryPool& pQueryPool = queryPools[currentBufferIndex];
        
        pCmdBuffer->begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        pCmdBuffer->resetQueryPool(*pQueryPool, 0, 2);
        pCmdBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *pQueryPool, 0);
        {
            vkx::DebugUtilsLabelScoped debug{ *pCmdBuffer, "Begin Frame", std::array<float, 4>{ 1.0f, 0.0f, 0.0f, 1.0f } };
            
            HAL::RenderPassAttachmentInfo renderPassAttachments[] = {
                HAL::RenderPassAttachmentInfo {
                    .ImageView =  pHALSwapChain->GetCurrentImageView(),
                    .ImageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                    .ClearValue = vk::ClearValue{ vk::ClearColorValue{ std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } }},
                    .Width = static_cast<uint32_t>(width),
                    .Height = static_cast<uint32_t>(height),
                    .LayerCount = 1            
                }         
            };
            
            HAL::RenderPassBeginInfo renderPassBeginInfo = {
                .AttachmentCount = _countof(renderPassAttachments),
                .pAttachments = renderPassAttachments,
                .RenderArea = scissor,
                .FramebufferWidth  = static_cast<uint32_t>(width),
                .FramebufferHeight = static_cast<uint32_t>(height),
                .FramebufferLayers = 1            
            };       
   
            pHALRenderPass->BeginRenderPass(*pCmdBuffer, renderPassBeginInfo, vk::SubpassContents::eInline);           
            {
                vkx::DebugUtilsLabelScoped debug{ *pCmdBuffer, "Render Color", std::array<float, 4>{ 0.0f, 1.0f, 0.0f, 1.0f } };
                pCmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *pPipeline);         
                pCmdBuffer->setScissor(0, { scissor });
                pCmdBuffer->setViewport(0, { viewport });
                pCmdBuffer->draw(3, 1, 0, 0);
            }  

            {
                vkx::DebugUtilsLabelScoped debug{ *pCmdBuffer, "Render GUI", std::array<float, 4>{ 0.0f, 0.0f, 1.0f, 1.0f } };
                ImGui_ImplVulkan_NewFrame(*pCmdBuffer, currentBufferIndex);
            }
            pHALRenderPass->EndRenderPass(*pCmdBuffer);
        }
        pCmdBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *pQueryPool, 1);
        pCmdBuffer->end();
        
        vk::PipelineStageFlags waitStageMask[]  = {
            vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        };

       // HAL::SwapChain::SynchronizationInfo frameSyncInfo = pHALSwapChain->GetSyncInfo();
       //
       // vk::SubmitInfo submitInfo = {
       //     .waitSemaphoreCount = 1, 
       //     .pWaitSemaphores = &frameSyncInfo.SemaphoresAvailable,
       //     .pWaitDstStageMask  = waitStageMask,
       //     .commandBufferCount = 1,
       //     .pCommandBuffers = pCmdBuffer.getAddressOf(),
       //     .signalSemaphoreCount = 1,
       //     .pSignalSemaphores = &frameSyncInfo.SemaphoresFinished       
       // };    
        


       // pHALDevice->GetGraphicsCommandQueue().ExecuteCommandList();
        pHALSwapChain->Present();
      
        if (auto result = pDevice.getQueryPoolResults<uint64_t>(*pQueryPool, 0, 2, 2 * sizeof(uint64_t), sizeof(uint64_t), vk::QueryResultFlagBits::e64); result.result == vk::Result::eSuccess) 
            GPUFrameTime = pHALDevice->GetVkPhysicalDevice().getProperties().limits.timestampPeriod * (result.value[1] - result.value[0]) / 1E6f;
        
        auto const timestampT1 = std::chrono::high_resolution_clock::now();
        CPUFrameTime = std::chrono::duration<float, std::milli>(timestampT1 - timestampT0).count();
    }

    {
        std::vector<uint8_t> cacheData = pDevice.getPipelineCacheData(*pPipelineCache);
        std::unique_ptr<FILE, decltype(&std::fclose)> pFile(std::fopen("PipelineCache", "wb"), std::fclose);
        std::fwrite(std::data(cacheData), sizeof(uint8_t), std::size(cacheData), pFile.get());
    }
  

    pHALDevice->Flush();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}