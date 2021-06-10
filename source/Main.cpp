#include <vulkan/vulkan_decl.h>

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
#include <HAL/Fence.hpp>
#include <HAL/CommandQueue.hpp>
#include <HAL/CommandAllocator.hpp>
#include <HAL/CommandList.hpp>
//#include <HAL/ShaderCompiler.hpp>
#include <memory>


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



//namespace vkx {
//
//    struct SpirvShaderResourceAttributes {
//        vk::DescriptorType descriptorType;
//        uint32_t           descriptorCount;
//        std::string        descriptorName;
//    };
//
//    struct SpirvShaderStageInputAttributes {
//
//    };
//
//    struct SpirvShaderResources {
//
//    };
//
//}
//


       
namespace HAL {
    
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
    };
    
    struct AllocatorCreateInfo {    
        vma::AllocatorCreateFlags  Flags = {};
        uint32_t                   FrameInUseCount = {};
        vma::DeviceMemoryCallbacks DeviceMemoryCallbacks = {};       
    };
   

    class MemoryAllocator {
    public:
        MemoryAllocator(Instance const& instance, Device const& device, AllocatorCreateInfo const& createInfo) {
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
//
//    
//
//    
//    struct BufferCreateInfo {
//
//    };
//
//    struct TextureCreateInfo {
//
//    };    
//
//    class Buffer {
//    public:
//        Buffer(MemoryAllocator const& allocator, BufferCreateInfo const& createInfo) {
//
//        }
//
//    private:
//        vma::UniqueAllocation m_pAllocation;
//        vk::UniqueBuffer      m_pBuffer;
//    };
//
//    class Texture {
//    public:
//        Texture(MemoryAllocator const& allocator, TextureCreateInfo const& createInfo) {
//
//        }
//
//    private:
//        vma::UniqueAllocation m_pAllocation;
//        vk::UniqueImage       m_pImage;
//    };
//
//
//    struct ShaderByteCode {
//        const char* pName = {};
//        const void* pData = {};
//        uint64_t    Size  = {};      
//    };
//
//    struct GraphicsPipelineCreateInfo {
//        ShaderByteCode VS;
//        ShaderByteCode PS;
//    };
//
//    struct ComputePipelineCreateInfo {
//        ShaderByteCode CS;
//    };
//
//    class PipelineLayout {
//
//    private:
//       
//    };

   // class GraphicsPipeline {
   // public:
   //     GraphicsPipeline(Device const& device, RenderPass const& renderPass, GraphicsPipelineCreateInfo const& createInfo) {
   //         
   //         vk::UniqueShaderModule vs = device.GetVkDevice().createShaderModuleUnique(vk::ShaderModuleCreateInfo{ .codeSize = createInfo.VS.Size, .pCode = reinterpret_cast<const uint32_t*>(createInfo.VS.pData) });
   //         vk::UniqueShaderModule fs = device.GetVkDevice().createShaderModuleUnique(vk::ShaderModuleCreateInfo{ .codeSize = createInfo.PS.Size, .pCode = reinterpret_cast<const uint32_t*>(createInfo.PS.pData) });
   //
   //         //vkx::setDebugName(device.GetVkDevice(), *vs, "[VS] WaveFront");
   //         //vkx::setDebugName(device.GetVkDevice(), *fs, "[FS] WaveFront");
   //       
   //         vk::PipelineShaderStageCreateInfo shaderStagesCI[] = {
   //             vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eVertex,   .module = *vs, .pName = createInfo.VS.pName },
   //             vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = *fs, .pName = createInfo.PS.pName },
   //         };
   //         
   //         vk::PipelineViewportStateCreateInfo viewportStateCI = {
   //             .viewportCount = 1,        
   //             .scissorCount = 1
   //         };
   //
   //         vk::PipelineVertexInputStateCreateInfo vertexInputStateCI = {
   //             .vertexBindingDescriptionCount  = 0,
   //             .vertexAttributeDescriptionCount = 0
   //         };
   //         
   //         vk::DynamicState dynamicStates[] = {
   //             vk::DynamicState::eScissor,
   //             vk::DynamicState::eViewport
   //         };
   //
   //         vk::PipelineDynamicStateCreateInfo dynamicStateCI = {
   //             .dynamicStateCount = _countof(dynamicStates),
   //             .pDynamicStates = dynamicStates
   //         };
   //
   //         vk::GraphicsPipelineCreateInfo pipelineCI = {
   //            .stageCount = _countof(shaderStagesCI),
   //            .pStages = shaderStagesCI,
   //            .pVertexInputState = &vertexInputStateCI,
   //            .pInputAssemblyState = &inputAssemblyStateCI,
   //            .pViewportState = &viewportStateCI,
   //            .pRasterizationState = &rasterizationStateCI,
   //            .pMultisampleState = &multisampleStateCI,
   //            .pDepthStencilState = &depthStencilStateCI,
   //            .pColorBlendState = &colorBlendStateCI,
   //            .pDynamicState = &dynamicStateCI,
   //            .layout = *pPipelineLayout,
   //            .renderPass = renderPass.GetVkRenderPass(),
   //         };
   //
   //         std::tie(std::ignore, m_pPipeline) = device.GetVkDevice().createGraphicsPipelineUnique(device.GetVkPipelineCache(), pipelineCI).asTuple();
   //         //vkx::setDebugName(pDevice, *pPipeline, "WaveFront");      
   //         //fmt::print("Flags: {} Duration: {}s\n", vk::to_string(creationFeedback.flags), creationFeedback.duration / 1E9f);     
   // 
   //     }
   // private:
   //     vk::UniquePipeline m_pPipeline;
   // };
   //
   // class ComputePipeline {
   // public:
   //     ComputePipeline(ComputePipelineCreateInfo const& createInfo) {
   //
   //     }
   // private:
   //     vk::UniquePipeline m_pPipeline;
   // };
   //
   // struct RenderPassCreateInfo {
   // private:
   //
   //
   // };
   //
   //
    class RenderPass {
    public:
        RenderPass(Device const& pDevice, vk::RenderPassCreateInfo const& createInfo){        
            m_pRenderPass = pDevice.GetVkDevice().createRenderPassUnique(createInfo); 
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
    
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t layerCount = 0;
            
            for(size_t index = 0; index < std::size(m_AttachmentsFormat); index++) {
                width = std::max(width, beginInfo.pAttachments[index].Width);
                height = std::max(height, beginInfo.pAttachments[index].Height);
                layerCount = std::max(layerCount, beginInfo.pAttachments[index].LayerCount);

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
            
            FrameBufferCacheKey key = { std::move(frameBufferAttanchments), width, height, layerCount };           
         
            if (m_FrameBufferCache.find(key) == m_FrameBufferCache.end()){
                 vk::StructureChain<vk::FramebufferCreateInfo, vk::FramebufferAttachmentsCreateInfo> framebufferCI = {
                     vk::FramebufferCreateInfo { 
                         .flags = vk::FramebufferCreateFlagBits::eImageless,
                         .renderPass = *m_pRenderPass,
                         .attachmentCount = static_cast<uint32_t>(std::size(key.FramebufferAttachment)),
                         .width  = width,
                         .height = height,
                         .layers = layerCount
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
        HAL::InstanceCreateInfo instanceCI = {
            .IsEnableValidationLayers = true
        };  
        pHALInstance = std::make_unique<HAL::Instance>(instanceCI);  
    }
       
    std::unique_ptr<HAL::Device> pHALDevice; {
        HAL::DeviceCreateInfo deviceCI = {
           
        };   
        pHALDevice = std::make_unique<HAL::Device>(*pHALInstance, pHALInstance->GetAdapters().at(0), deviceCI);
    }
    
    std::unique_ptr<HAL::SwapChain> pHALSwapChain; {
        HAL::SwapChainCreateInfo swapChainCI = {
            .Width = WINDOW_WIDTH,
            .Height = WINDOW_HEIGHT,
            .WindowHandle = glfwGetWin32Window(pWindow.get()),
            .IsSRGB = true,
            .IsVSync = false
        };
        pHALSwapChain = std::make_unique<HAL::SwapChain>(*pHALInstance, *pHALDevice, swapChainCI);
    }

    auto pHALGraphicsCommandQueue = pHALDevice->GetGraphicsCommandQueue();
    auto pHALComputeCommandQueue  = pHALDevice->GetComputeCommandQueue();
    auto pHALTransferCommandQueue = pHALDevice->GetTransferCommandQueue();

    auto pHALGraphicsCmdAllocator = std::make_unique<HAL::GraphicsCommandAllocator>(*pHALDevice);
    auto pHALComputeCmdAllocator  = std::make_unique<HAL::ComputeCommandAllocator>(*pHALDevice);
    auto pHALTransferCmdAllocator = std::make_unique<HAL::TransferCommandAllocator>(*pHALDevice);

    MemoryStatisticGPU memoryGPU = { pHALDevice->GetVkPhysicalDevice() };
    MemoryStatisticCPU memoryCPU;
    
    std::unique_ptr<HAL::MemoryAllocator> pHALAllocator; {
        HAL::AllocatorCreateInfo allocatorCI = {
            .Flags = vma::AllocatorCreateFlagBits::eExtMemoryBudget,
            .FrameInUseCount = BUFFER_COUNT,
            .DeviceMemoryCallbacks = {
                .pfnAllocate = MemoryStatisticGPU::GPUAllocate,
                .pfnFree     = MemoryStatisticGPU::GPUFree,
                .pUserData   = &memoryGPU,
            }
        };
        pHALAllocator = std::make_unique<HAL::MemoryAllocator>(*pHALInstance, *pHALDevice, allocatorCI);
    }

    std::unique_ptr<HAL::RenderPass> pHALRenderPass; {
        vk::AttachmentDescription attachments[] = {
            vk::AttachmentDescription{
                .format = pHALSwapChain->GetFormat(),
                .samples = vk::SampleCountFlagBits::e1,
                .loadOp = vk::AttachmentLoadOp::eClear,
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
        pHALRenderPass = std::make_unique<HAL::RenderPass>(*pHALDevice, renderPassCI);
    } 

    std::vector<std::unique_ptr<HAL::GraphicsCommandList>> HALCommandLists;
    std::vector<std::unique_ptr<HAL::Fence>> HALFences;

    for(size_t index = 0; index < BUFFER_COUNT; index++) {
        HALCommandLists.push_back(std::make_unique<HAL::GraphicsCommandList>(*pHALGraphicsCmdAllocator));
        HALFences.push_back(std::make_unique<HAL::Fence>(*pHALDevice, 0));
    }

   
    
   // HAL::ShaderCompiler compiler = { HAL::ShaderCompilerCreateInfo{ .ShaderModelVersion = HAL::ShaderModel::SM_6_5, .IsDebugMode = true } };
   //
   // vk::UniqueDescriptorSetLayout pDescriptorSetLayout; {
   //     vk::DescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
   //         vk::DescriptorSetLayoutBinding {
   //             .binding = 0,
   //             .descriptorType = vk::DescriptorType::eUniformBufferDynamic,
   //             .descriptorCount = 1,
   //             .stageFlags = vk::ShaderStageFlagBits::eAllGraphics
   //         }   
   //     };
   //
   //     vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {
   //         .bindingCount = _countof(descriptorSetLayoutBindings),
   //         .pBindings = descriptorSetLayoutBindings
   //     };
   //
   //     pDescriptorSetLayout = pDevice.createDescriptorSetLayoutUnique(descriptorSetLayoutCI);
   //     vkx::setDebugName(pDevice, *pDescriptorSetLayout, "");
   // }
   //
   // vk::UniquePipelineLayout pPipelineLayout; {
   //     vk::PipelineLayoutCreateInfo pipelineLayoutCI = {
   //         .setLayoutCount = 1,
   //         .pSetLayouts = pDescriptorSetLayout.getAddressOf(),
   //         .pushConstantRangeCount = 0
   //     };
   //     pPipelineLayout = pDevice.createPipelineLayoutUnique(pipelineLayoutCI);
   //     vkx::setDebugName(pDevice, *pPipelineLayout, "");  
   // }
   //
   // 
   // std::unique_ptr<HAL::GraphicsPipeline> pPipeline; {
   //   
   //     auto const spirvVS = *compiler.CompileFromFile(L"content/shaders/WaveFront.hlsl", L"VSMain", HAL::ShaderStage::Vertex, {});
   //     auto const spirvPS = *compiler.CompileFromFile(L"content/shaders/WaveFront.hlsl", L"PSMain", HAL::ShaderStage::Pixel,  {});
   //
   //     HAL::GraphicsPipelineCreateInfo pipelineCI = {
   //         .VS = { .pData = std::data(spirvVS), .Size = std::size(spirvVS) },
   //         .PS = { .pData = std::data(spirvPS), .Size = std::size(spirvPS) },
   //     };
   //
   //     pPipeline = std::make_unique<HAL::GraphicsPipeline>(pHALDevice, pHALRenderPass, pipelineCI);
   // }
   //
   // std::vector<std::unique_ptr<HAL::GraphicsCommandAllocator>> commandAllocators;
   // for (size_t index = 0; index < BUFFER_COUNT; index++) 
   //     commandAllocators.emplace_back(std::make_unique<HAL::GraphicsCommandAllocator>(*pDevice));
   //
   //
   // std::vector<std::unique_ptr<HAL::GraphicsCommandList>> commandLists;
   // for (size_t index = 0; index < BUFFER_COUNT; index++) 
   //     commandLists.emplace_back(std::make_unique<HAL::GraphicsCommandList>(*commandAllocators[index]));
   // 
   //
   // std::vector<vk::UniqueDescriptorPool> descriptorPools;
   // for (size_t index = 0; index < BUFFER_COUNT; index++) {
   //     vk::DescriptorPoolSize descriptorPoolSize[] = {
   //         vk::DescriptorPoolSize {
   //             .type = vk::DescriptorType::eUniformBufferDynamic,
   //             .descriptorCount = 1
   //         }
   //     };
   //
   //     vk::DescriptorPoolCreateInfo descriptorPoolCI = {
   //         .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
   //         .maxSets = 1,
   //         .poolSizeCount = _countof(descriptorPoolSize),
   //         .pPoolSizes = descriptorPoolSize
   //     };
   //     auto pDescriptorPool = pDevice.createDescriptorPoolUnique(descriptorPoolCI);
   //     vkx::setDebugName(pDevice, *pDescriptorPool, fmt::format("[{0}]", index));
   //     descriptorPools.push_back(std::move(pDescriptorPool));
   // }
   //
   // std::vector<vk::UniqueDescriptorSet> descriptorSets;
   // for (size_t index = 0; index < BUFFER_COUNT; index++) {
   //     vk::DescriptorSetLayout descriptorSetLayouts[] = {
   //         *pDescriptorSetLayout
   //     };
   //
   //     vk::DescriptorSetAllocateInfo descriptorSetAI = {
   //         .descriptorPool = *descriptorPools[index],
   //         .descriptorSetCount = _countof(descriptorSetLayouts),
   //         .pSetLayouts = descriptorSetLayouts
   //     };
   //     auto pDescriptorSet = std::move(pDevice.allocateDescriptorSetsUnique(descriptorSetAI).front());
   //     vkx::setDebugName(pDevice, *pDescriptorSet, fmt::format("[{0}]", index));
   //     descriptorSets.push_back(std::move(pDescriptorSet));
   // }
   
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
    
   // std::vector<vk::UniqueQueryPool> queryPools;
   // for (size_t index = 0; index < BUFFER_COUNT; index++) {
   //     vk::QueryPoolCreateInfo queryPoolCI = {
   //         .queryType = vk::QueryType::eTimestamp,
   //         .queryCount = 2
   //     };
   //     auto pQueryPool = pDevice.createQueryPoolUnique(queryPoolCI);
   //     queryPools.push_back(std::move(pQueryPool));
   // }
   //
   // 
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
   
    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("content/fonts/Roboto-Medium.ttf", 15.0f);
    ImGui::StyleColorsDark();
    ImPlot::GetStyle().AntiAliasedLines = true;
   
    ImGui_ImplGlfw_InitForVulkan(pWindow.get(), true);
    ImGui_ImplVulkan_Init(pHALDevice->GetVkDevice(), pHALDevice->GetVkPhysicalDevice(), pHALAllocator->GetVmaAllocator(), pHALRenderPass->GetVkRenderPass(), BUFFER_COUNT);


    float CPUFrameTime = 0.0f;
    float GPUFrameTime = 0.0f;
     
    struct WindowUserData {
        const HAL::CommandQueue* pCommandQueue;
        HAL::SwapChain*          pSwapChain;
    } GLFWUserData = { pHALComputeCommandQueue, pHALSwapChain.get() };

    glfwSetWindowUserPointer(pWindow.get(), &GLFWUserData);
    glfwSetWindowSizeCallback(pWindow.get(), [](GLFWwindow* pWindow, int32_t width, int32_t height)-> void {
        auto pUserData = reinterpret_cast<WindowUserData*>(glfwGetWindowUserPointer(pWindow));
        pUserData->pCommandQueue->WaitIdle();
        pUserData->pSwapChain->Resize(width, height);
    });  

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
        
        static uint64_t frameCount = 0;
        uint64_t bufferID = frameCount++ % BUFFER_COUNT;

        //Wait until the previous frame is finished
        if (!HALFences[bufferID]->IsCompleted())        
            HALFences[bufferID]->Wait(HALFences[bufferID]->GetExpectedValue());
        
        //Acquire Image
        uint32_t frameID = pHALComputeCommandQueue->NextImage(*pHALSwapChain, *HALFences[bufferID], HALFences[bufferID]->Increment());
   
        //Render pass
        vk::Rect2D   scissor  = { .offset = { .x = 0, .y = 0 }, .extent = { .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) } };
        vk::Viewport viewport = { .x = 0, .y = static_cast<float>(height), .width = static_cast<float>(width), .height = -static_cast<float>(height), .minDepth = 0.0f, .maxDepth = 1.0f };

        HALCommandLists[bufferID]->Begin();
        {
                    
            HAL::RenderPassAttachmentInfo renderPassAttachments[] = {
                HAL::RenderPassAttachmentInfo {
                    .ImageView =  pHALSwapChain->GetImageView(frameID),
                    .ImageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                    .ClearValue = vk::ClearValue{ vk::ClearColorValue{ std::array<float, 4> { 0.1f, 0.1f, 0.1f, 1.0f } }},
                    .Width = static_cast<uint32_t>(width),
                    .Height = static_cast<uint32_t>(height),
                    .LayerCount = 1            
                }         
            };
            
            HAL::RenderPassBeginInfo renderPassBeginInfo = {
                .AttachmentCount = _countof(renderPassAttachments),
                .pAttachments = renderPassAttachments,
                .RenderArea = scissor       
            };       
        
            pHALRenderPass->BeginRenderPass(HALCommandLists[bufferID]->GetVkCommandBuffer(), renderPassBeginInfo, vk::SubpassContents::eInline);               
            ImGui_ImplVulkan_NewFrame(HALCommandLists[bufferID]->GetVkCommandBuffer(), bufferID);                   
            pHALRenderPass->EndRenderPass(HALCommandLists[bufferID]->GetVkCommandBuffer());
        }
        HALCommandLists[bufferID]->End();


        //Execute command List
        pHALGraphicsCommandQueue->Wait(*HALFences[bufferID], HALFences[bufferID]->GetExpectedValue());  
        pHALGraphicsCommandQueue->ExecuteCommandLists(HALCommandLists[bufferID].get(), 1);
        pHALGraphicsCommandQueue->Signal(*HALFences[bufferID], HALFences[bufferID]->Increment());

        //Wait fence and present
        pHALComputeCommandQueue->Present(*pHALSwapChain, frameID, *HALFences[bufferID]);


   //
   //     //------------------------------//
   //     uint64_t currentFrameID  = pHALSwapChain->GetCurrentFrame();
   //     uint64_t currentBufferID = pHALSwapChain->GetCurrentBuffer();
   //
   //     pHALSwapChain->AcquireNextImage(*pHALFence, fenceIndices[currentBufferID]); //Wait Previous Frame
   //

        //Record new Command buffer

        //vk::Rect2D   scissor  = { .offset = { .x = 0, .y = 0 }, .extent = { .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) } };
        //vk::Viewport viewport = { .x = 0, .y = static_cast<float>(height), .width = static_cast<float>(width), .height = -static_cast<float>(height), .minDepth = 0.0f, .maxDepth = 1.0f };
        //
        //vk::UniqueCommandBuffer& pCmdBuffer = commandLists[currentBufferIndex];
        //vk::UniqueQueryPool& pQueryPool = queryPools[currentBufferIndex];
        //
        //pCmdBuffer->begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        //pCmdBuffer->resetQueryPool(*pQueryPool, 0, 2);
        //pCmdBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *pQueryPool, 0);
        //{
        //    vkx::DebugUtilsLabelScoped debug{ *pCmdBuffer, "Begin Frame", std::array<float, 4>{ 1.0f, 0.0f, 0.0f, 1.0f } };
        //    
        //    HAL::RenderPassAttachmentInfo renderPassAttachments[] = {
        //        HAL::RenderPassAttachmentInfo {
        //            .ImageView =  pHALSwapChain->GetCurrentImageView(),
        //            .ImageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        //            .ClearValue = vk::ClearValue{ vk::ClearColorValue{ std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } }},
        //            .Width = static_cast<uint32_t>(width),
        //            .Height = static_cast<uint32_t>(height),
        //            .LayerCount = 1            
        //        }         
        //    };
        //    
        //    HAL::RenderPassBeginInfo renderPassBeginInfo = {
        //        .AttachmentCount = _countof(renderPassAttachments),
        //        .pAttachments = renderPassAttachments,
        //        .RenderArea = scissor,
        //        .FramebufferWidth  = static_cast<uint32_t>(width),
        //        .FramebufferHeight = static_cast<uint32_t>(height),
        //        .FramebufferLayers = 1            
        //    };       
        //
        //    pHALRenderPass->BeginRenderPass(*pCmdBuffer, renderPassBeginInfo, vk::SubpassContents::eInline);           
        //    {
        //        vkx::DebugUtilsLabelScoped debug{ *pCmdBuffer, "Render Color", std::array<float, 4>{ 0.0f, 1.0f, 0.0f, 1.0f } };
        //        pCmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *pPipeline);         
        //        pCmdBuffer->setScissor(0, { scissor });
        //        pCmdBuffer->setViewport(0, { viewport });
        //        pCmdBuffer->draw(3, 1, 0, 0);
        //    }  
        //
        //    {
        //        vkx::DebugUtilsLabelScoped debug{ *pCmdBuffer, "Render GUI", std::array<float, 4>{ 0.0f, 0.0f, 1.0f, 1.0f } };
        //        ImGui_ImplVulkan_NewFrame(*pCmdBuffer, currentBufferIndex);
        //    }
        //    pHALRenderPass->EndRenderPass(*pCmdBuffer);
        //}
        //pCmdBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *pQueryPool, 1);
        //pCmdBuffer->end();
        //

        //Excecute command buffer and set signal after execution

        //pHALCommandQueue->ExecuteCommandList(nullptr, 0);
        //pHALCommandQueue->Signal(*pHALFence, fenceValue);
        //pHALSwapChain->Present(); // Signal Present Engine 
              

        //------------------------------//

        //if (auto result = pDevice.getQueryPoolResults<uint64_t>(*pQueryPool, 0, 2, 2 * sizeof(uint64_t), sizeof(uint64_t), vk::QueryResultFlagBits::e64); result.result == vk::Result::eSuccess) 
        //    GPUFrameTime = pHALDevice->GetVkPhysicalDevice().getProperties().limits.timestampPeriod * (result.value[1] - result.value[0]) / 1E6f;
        //
        auto const timestampT1 = std::chrono::high_resolution_clock::now();
        CPUFrameTime = std::chrono::duration<float, std::milli>(timestampT1 - timestampT0).count();
        GPUFrameTime = 0.0f;
    }
    

    pHALDevice->WaitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

}

