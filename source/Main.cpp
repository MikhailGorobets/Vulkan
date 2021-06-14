#include <vulkan/vulkan_decl.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <implot/implot.h>

#define GLFW_INCLUDE_NONE 1

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <fmt/core.h>
#include <fmt/format.h>


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
#include <HAL/ShaderCompiler.hpp>

#include <spirv_hlsl.hpp>


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
        fmt::print("GPU Allocation -> Memory type: {0}, Size: {1} kb \n", vk::to_string(memoryInfo), (size >> 10));
    }
    
    static auto GPUFree(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* pUserData) -> void {
         if (pUserData) {
            auto pMemoryStat = reinterpret_cast<MemoryStatisticGPU*>(pUserData);
            pMemoryStat->RemoveMemoryInfo(memoryType, size);
         }

         vk::MemoryPropertyFlags memoryInfo = {};
         vmaGetMemoryTypeProperties(allocator, memoryType, reinterpret_cast<VkMemoryPropertyFlags*>(&memoryInfo));
         fmt::print("GPU Free -> Memory type: {0}, Size: {1} kb \n", vk::to_string(memoryInfo), (size >> 10));    
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
    

    struct AllocatorCreateInfo {    
        vma::AllocatorCreateFlags  Flags = {};  
        vma::DeviceMemoryCallbacks DeviceMemoryCallbacks = {};
        uint32_t                   FrameInUseCount = {};       
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
            m_FrameIndex = 0;
            m_FrameCount = createInfo.FrameInUseCount;   
        }
        
        auto NextFrame() -> void {
            m_FrameIndex = m_FrameIndex++ % m_FrameCount;
            m_pAllocator->setCurrentFrameIndex(m_FrameIndex);
        }        

        auto GetVmaAllocator() const -> vma::Allocator { return *m_pAllocator; }        

    private:
        vma::UniqueAllocator m_pAllocator;
        uint32_t             m_FrameIndex;
        uint32_t             m_FrameCount;
    };
 
    struct BufferCreateInfo {
 
    };
 
    struct TextureCreateInfo {
        uint32_t   Width = 0;
        uint32_t   Height = 0;
        uint32_t   Depth = 0;
        uint32_t   ArraySize = 1;
        uint32_t   MipLevels = 1;
        vk::Format Format;
        
    };    
 
    class Buffer {
    public:
        Buffer(MemoryAllocator const& allocator, BufferCreateInfo const& createInfo) {
            
        }
 
    private:
        vma::UniqueAllocation m_pAllocation;
        vk::UniqueBuffer      m_pBuffer;
    };
 
    class Texture {
    public:
        Texture(MemoryAllocator const& allocator, TextureCreateInfo const& createInfo) {
            vk::ImageCreateInfo imageCI = {
                
                .extent = { createInfo.Width, createInfo.Height, createInfo.Depth },
                .arrayLayers = createInfo.ArraySize,
                
            };
        }
 
    private:
        vma::UniqueAllocation m_pAllocation;
        vk::UniqueImage       m_pImage;
    };
 
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

}


int main(int argc, char* argv[]) {
    
    auto const WINDOW_TITLE = "Application Vulkan";
    auto const WINDOW_WIDTH  = 1920;
    auto const WINDOW_HEIGHT = 1280;
 
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


    auto pHALGraphicsCommandQueue = &pHALDevice->GetGraphicsCommandQueue();
    auto pHALComputeCommandQueue  = &pHALDevice->GetComputeCommandQueue();
    auto pHALTransferCommandQueue = &pHALDevice->GetTransferCommandQueue();

    auto pHALGraphicsCmdAllocator = std::make_unique<HAL::GraphicsCommandAllocator>(*pHALDevice);
    auto pHALComputeCmdAllocator  = std::make_unique<HAL::ComputeCommandAllocator>(*pHALDevice);
    auto pHALTransferCmdAllocator = std::make_unique<HAL::TransferCommandAllocator>(*pHALDevice);

    MemoryStatisticGPU memoryGPU = { pHALDevice->GetVkPhysicalDevice() };
    MemoryStatisticCPU memoryCPU;
    
    std::unique_ptr<HAL::MemoryAllocator> pHALAllocator; {
        HAL::AllocatorCreateInfo allocatorCI = {
            .Flags = vma::AllocatorCreateFlagBits::eExtMemoryBudget,          
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

    auto pHALCommandList = std::make_unique<HAL::GraphicsCommandList>(*pHALGraphicsCmdAllocator);
    auto pHALFence = std::make_unique<HAL::Fence>(*pHALDevice);
    
    std::unique_ptr<HAL::ShaderCompiler> pHALCompiler;
    {
        HAL::ShaderCompilerCreateInfo shaderCompilerCI = {
            .ShaderModelVersion = HAL::ShaderModel::SM_6_5, 
            .IsDebugMode = true 
        };
        pHALCompiler = std::make_unique< HAL::ShaderCompiler>(shaderCompilerCI);
    }

    auto spirvVS = pHALCompiler->CompileFromFile(L"content/shaders/WaveFront.hlsl", L"VSMain", HAL::ShaderStage::Vertex, {});
    auto spirvPS = pHALCompiler->CompileFromFile(L"content/shaders/WaveFront.hlsl", L"PSMain", HAL::ShaderStage::Pixel,  {});
  
    spirv_cross::CompilerHLSL compilerVS(std::move(spirvVS.value()));


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
    ImGui_ImplVulkan_Init(pHALDevice->GetVkDevice(), pHALDevice->GetVkPhysicalDevice(), pHALAllocator->GetVmaAllocator(), pHALRenderPass->GetVkRenderPass());

    float CPUFrameTime = 0.0f;
    float GPUFrameTime = 0.0f;
     
    struct WindowUserData {
        HAL::CommandQueue* pCommandQueue;
        HAL::SwapChain*    pSwapChain;
    } GLFWUserData = { pHALComputeCommandQueue, pHALSwapChain.get() };

   
    glfwSetWindowUserPointer(pWindow.get(), &GLFWUserData);
    glfwSetWindowSizeCallback(pWindow.get(), [](GLFWwindow* pWindow, int32_t width, int32_t height)-> void {
        glfwWaitEvents();
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
  
        //Wait until the previous frame is finished
        if (!pHALFence->IsCompleted())        
            pHALFence->Wait(pHALFence->GetExpectedValue());
        
        //Acquire Image and signal fence
        uint32_t frameID = pHALComputeCommandQueue->NextImage(*pHALSwapChain, *pHALFence, pHALFence->Increment());
     
        //Render pass
        pHALCommandList->Begin();
        {
            HAL::RenderPassAttachmentInfo renderPassAttachments[] = {
                HAL::RenderPassAttachmentInfo {
                    .ImageView =  pHALSwapChain->GetImageView(frameID),
                    .ImageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                    .ClearValue = vk::ClearValue{ vk::ClearColorValue{ std::array<float, 4> {0.5f, 0.5f, 0.5f, 1.0f } }},
                    .Width = static_cast<uint32_t>(width),
                    .Height = static_cast<uint32_t>(height),
                    .LayerCount = 1            
                }         
            };

            pHALCommandList->BeginRenderPass({.pRenderPass = pHALRenderPass.get(), .pAttachments = renderPassAttachments, .AttachmentCount = _countof(renderPassAttachments)});       
            ImGui_ImplVulkan_NewFrame(pHALCommandList->GetVkCommandBuffer());                   
            pHALCommandList->EndRenderPass();

        }
        pHALCommandList->End();
     
     
        //Wait fence nad Execute command List
        pHALGraphicsCommandQueue->Wait(*pHALFence, pHALFence->GetExpectedValue());  
        pHALGraphicsCommandQueue->ExecuteCommandLists({ *pHALCommandList });
        pHALGraphicsCommandQueue->Signal(*pHALFence, pHALFence->Increment());
     
        //Wait fence and present
        pHALComputeCommandQueue->Present(*pHALSwapChain, frameID, *pHALFence);



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

