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


#include <chrono>
#include <thread>
#include <algorithm>
#include <functional>
#include <numeric>
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
#include <HAL/DescriptorTableLayout.hpp>
#include <HAL/Pipeline.hpp>



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
        std::vector<MemoryFrame> MemoryFrames = {};
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
                .MemoryType = m_MemoryProperty.memoryHeaps[index].flags & vk::MemoryHeapFlagBits::eDeviceLocal ? HeapType::DeviceMemory : HeapType::SystemMemory,
                .MemoryFrames = std::vector<MemoryFrame>{ {0, 0} } 
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


 
namespace HAL {

    enum class MemoryUsage {
         eUnknown,
         eGpuOnly,
         eCpuOnly,
         eCpuToGpu,
         eGpuToCpu,
         eCpuCopy,
         eGpuLazilyAllocated
    };

    enum class BufferUsageFlagBits {

    };
  
    struct BufferViewCreateInfo {
        uint64_t Offset = {};
        uint64_t Range = {};
        Format   Format = {};
        bool     IsRawView = {};
    };

    struct BufferCreateInfo {
        uint64_t            Size = {};
        BufferUsageFlagBits BuffferUsage = {};
        MemoryUsage         MemoryUsage = {};
    };

    struct TextureCreateInfo {
        uint32_t Width = {};
        uint32_t Height = {};
        uint32_t Depth = {};
        uint32_t ArraySize = {};
        uint32_t MipLevels = {};
        Format   Format = {};
    };

    template<typename T>
    class ObjectBase {
    public:
        ObjectBase(T const& createInfo) : m_CreateInfo(createInfo) {}

        auto GetCreateInfo() const -> T const& { return m_CreateInfo; };

    protected:
        T m_CreateInfo = {};       
    };
  
    class Buffer final: public ObjectBase<BufferCreateInfo> {
    public:
        Buffer(Device const& device, BufferCreateInfo const& createInfo): ObjectBase(createInfo) {
            
            vk::BufferCreateInfo bufferCI = {
                
            };

            vma::AllocationCreateInfo allocationCI = {
                .flags = vma::AllocationCreateFlagBits::eWithinBudget,
                .usage = static_cast<vma::MemoryUsage>(createInfo.MemoryUsage)
            };

            std::tie(m_pBuffer, m_pAllocation) = device.GetVmaAllocator().createBufferUnique(bufferCI, allocationCI, m_AllocationInfo);         
        }

        auto GetCreateInfo() const -> BufferCreateInfo const& { return m_CreateInfo; }

        auto GetVkBuffer() const -> vk::Buffer { return m_pBuffer.get(); }
    
        auto GetVmaAllocation() const -> vma::Allocation { return m_pAllocation.get(); }

    private:
        vma::AllocationInfo   m_AllocationInfo = {};
        vma::UniqueAllocation m_pAllocation = {};
        vk::UniqueBuffer      m_pBuffer = {};       
    };
 
    class BufferView final: public ObjectBase<BufferViewCreateInfo> {
    public:
        BufferView(Device const& device, Buffer const& buffer, BufferViewCreateInfo const& createInfo) : ObjectBase(createInfo) {
            m_Buffer = buffer.GetVkBuffer();
            if (createInfo.IsRawView) {
                vk::BufferViewCreateInfo bufferViewCI = {
                    .buffer = buffer.GetVkBuffer(),
                    .format = static_cast<vk::Format>(createInfo.Format),
                    .offset = createInfo.Offset,
                    .range = createInfo.Range
                };
                m_pBufferView = device.GetVkDevice().createBufferViewUnique(bufferViewCI);
            }
        }

        auto GetVkBufferView() const -> vk::BufferView { return m_pBufferView.get(); }

        auto GetVkBuffer() const -> vk::Buffer { return m_Buffer; }

    private:
        vk::Buffer           m_Buffer;
        vk::UniqueBufferView m_pBufferView;
    };

    class TextureView {
    public:
        TextureView(Device const& device) {
            
        }

    private:
        vk::ImageViewCreateInfo m_CreateInfo;
        vk::UniqueImageView     m_pImageView;
        
    };

    class Texture {
    public:
        Texture(TextureCreateInfo const& createInfo) { }
        
       

    private:
        vma::UniqueAllocation m_pAllocation;
        vk::UniqueImage       m_pImage;
    };
 
    class Sampler;
    
    class DescriptorAllocator;

    class DescriptorTable {
    public:
        DescriptorTable() = default;

        auto WriteTextureViews(uint32_t index, HAL::ArraySpan<HAL::TextureView> textures) -> void {
            auto const& resourceInfo = m_pDescriptorTableLayout->GetPipelineResource(index);

            
        }

        auto WriteBufferViews(uint32_t index, HAL::ArraySpan<HAL::BufferView> buffers) -> void {
            auto const& resourceInfo = m_pDescriptorTableLayout->GetPipelineResource(index);
            
        }

        auto WriteSamplers(uint32_t index, HAL::ArraySpan<HAL::Sampler> samplers) -> void {
            auto const& resourceInfo = m_pDescriptorTableLayout->GetPipelineResource(index);
        }

        auto UpdateDescriptors() -> void {

            std::vector<vk::WriteDescriptorSet> descriptortWrites;

            descriptortWrites.push_back(vk::WriteDescriptorSet{
                .descriptorCount = static_cast<uint32_t>(std::size(m_DescriptorsUniformBuffer)),
                .descriptorType = vk::DescriptorType::eUniformBufferDynamic,
                .pBufferInfo = std::data(m_DescriptorsUniformBuffer)
            });

            descriptortWrites.push_back(vk::WriteDescriptorSet{
                .descriptorCount = static_cast<uint32_t>(std::size(m_DescriptorsStorageBuffer)),
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = std::data(m_DescriptorsStorageBuffer)
            });

            descriptortWrites.push_back(vk::WriteDescriptorSet{
                .descriptorCount = static_cast<uint32_t>(std::size(m_DescriptorsImageSampled)),
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = std::data(m_DescriptorsImageSampled)
            });

            descriptortWrites.push_back(vk::WriteDescriptorSet{
                .descriptorCount = static_cast<uint32_t>(std::size(m_DescriptorsImageStorage)),
                .descriptorType = vk::DescriptorType::eStorageImage,
                .pImageInfo = std::data(m_DescriptorsImageStorage)
            });

            descriptortWrites.push_back(vk::WriteDescriptorSet{
                .descriptorCount = static_cast<uint32_t>(std::size(m_DescriptorsSampler)),
                .descriptorType = vk::DescriptorType::eSampler,
                .pImageInfo = std::data(m_DescriptorsSampler)
            });

           
        }

    private:
        std::vector<vk::DescriptorBufferInfo> m_DescriptorsUniformBuffer;
        std::vector<vk::DescriptorBufferInfo> m_DescriptorsStorageBuffer;
        std::vector<vk::DescriptorImageInfo>  m_DescriptorsImageSampled;
        std::vector<vk::DescriptorImageInfo>  m_DescriptorsImageStorage;
        std::vector<vk::DescriptorImageInfo>  m_DescriptorsSampler;
        std::vector<vk::BufferView>           m_DescriptorUniformTexelBuffer;
        std::vector<vk::BufferView>           m_DescriptorStorageTexelBuffer;

        DescriptorTableLayout*  m_pDescriptorTableLayout = {};
        vk::DescriptorSet       m_pDescriptorSet = {};
    };

    struct DescriptorAllocatorCreateInfo {
        uint32_t UniformBufferCount = {};
        uint32_t StorageBufferCount = {};
        uint32_t SampledImageCount = {};
        uint32_t StorageImageCount = {};
        uint32_t SamplerCount = {};
        uint32_t MaxSets = {};
    };

    class DescriptorAllocator {
    public:
        DescriptorAllocator(HAL::Device const& pDevice, DescriptorAllocatorCreateInfo const& createInfo) {
            vk::DescriptorPoolSize descriptorPoolSizes[] = {
                vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = createInfo.UniformBufferCount},
                vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = createInfo.StorageBufferCount},
                vk::DescriptorPoolSize{.type = vk::DescriptorType::eSampledImage, .descriptorCount = createInfo.SampledImageCount},
                vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageImage, .descriptorCount = createInfo.StorageImageCount},
                vk::DescriptorPoolSize{.type = vk::DescriptorType::eSampler,.descriptorCount = createInfo.SamplerCount}
            };

            vk::DescriptorPoolCreateInfo descriptorPoolCI = {
                .maxSets = createInfo.MaxSets,
                .poolSizeCount = _countof(descriptorPoolSizes),
                .pPoolSizes = descriptorPoolSizes
            };
            m_pDescriptorPool = pDevice.GetVkDevice().createDescriptorPoolUnique(descriptorPoolCI);
        }

        auto Allocate(HAL::DescriptorTableLayout const& layout, std::optional<uint32_t> dynamicDescriptorCount = std::nullopt) -> DescriptorTable {

            uint32_t pDescriptorCount[] = { dynamicDescriptorCount.value_or(0) };

            vk::DescriptorSetLayout pDescriptorSetLayouts[] = {
                layout.GetVkDescriptorSetLayout()
            };

            vk::DescriptorSetVariableDescriptorCountAllocateInfo descriptorSetVariableDescriptorCountAI = {
                .descriptorSetCount = _countof(pDescriptorCount),
                .pDescriptorCounts = pDescriptorCount
            };

            vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo = {
                .pNext = &descriptorSetVariableDescriptorCountAI,
                .descriptorPool = m_pDescriptorPool.get(),
                .descriptorSetCount = _countof(pDescriptorSetLayouts),
                .pSetLayouts = pDescriptorSetLayouts
            };

           return DescriptorTable();
        }


        auto Flush() -> void {
            m_pDescriptorPool.getOwner().resetDescriptorPool(m_pDescriptorPool.get());
        }

        auto GetVkDescriptorPool() -> vk::DescriptorPool {
            return m_pDescriptorPool.get();
        }

    private:
        vk::UniqueDescriptorPool m_pDescriptorPool;
    };

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

    std::unique_ptr<HAL::RenderPass> pHALRenderPass; {
        vk::AttachmentDescription attachments[] = {
            vk::AttachmentDescription{
                .format = pHALSwapChain->GetFormat(),
                .samples = vk::SampleCountFlagBits::e1,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .initialLayout = vk::ImageLayout::eUndefined,
                .finalLayout = vk::ImageLayout::ePresentSrcKHR,
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
    
    std::unique_ptr<HAL::ShaderCompiler> pHALCompiler; {
        HAL::ShaderCompilerCreateInfo shaderCompilerCI = {
            .ShaderModelVersion = HAL::ShaderModel::SM_6_5, 
            .IsDebugMode = true 
        };
        pHALCompiler = std::make_unique<HAL::ShaderCompiler>(shaderCompilerCI);
    }

 
    std::unique_ptr<HAL::GraphicsPipeline> pHALGraphicsPipeline;
    std::unique_ptr<HAL::ComputePipeline>  pHALComputePipeline; {
        auto spirvVS = pHALCompiler->CompileFromFile(L"content/shaders/WaveFront.hlsl", L"VSMain", HAL::ShaderStage::Vertex);
        auto spirvPS = pHALCompiler->CompileFromFile(L"content/shaders/WaveFront.hlsl", L"PSMain", HAL::ShaderStage::Fragment);
        auto spirvCS = pHALCompiler->CompileFromFile(L"content/shaders/WaveFront.hlsl", L"CSMain", HAL::ShaderStage::Compute);
      
        HAL::GraphicsPipelineCreateInfo graphicsPipelineCI = {
            .VS = { std::data(*spirvVS), std::size(*spirvVS) },
            .PS = { std::data(*spirvPS), std::size(*spirvPS) }
        };
    
        HAL::ComputePipelineCreateInfo computePipelineCI = {
            .CS = { std::data(*spirvCS), std::size(*spirvCS) },
        };
     
        pHALGraphicsPipeline = std::make_unique<HAL::GraphicsPipeline>(*pHALDevice, graphicsPipelineCI);
        pHALComputePipeline  = std::make_unique<HAL::ComputePipeline>(*pHALDevice, computePipelineCI);      
    }

    std::unique_ptr<HAL::DescriptorAllocator> pHALDescritprorAllocator; {
        HAL::DescriptorAllocatorCreateInfo descriptorAllocatorCI = {
            .UniformBufferCount = 10,
            .StorageBufferCount = 10,
            .SampledImageCount = 10,
            .StorageImageCount = 10,
            .SamplerCount = 10,
            .MaxSets = 2,
        };
        pHALDescritprorAllocator = std::make_unique<HAL::DescriptorAllocator>(*pHALDevice, descriptorAllocatorCI);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
   
    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("content/fonts/Roboto-Medium.ttf", 15.0f);
    ImGui::StyleColorsDark();
    ImPlot::GetStyle().AntiAliasedLines = true;
   
    ImGui_ImplGlfw_InitForVulkan(pWindow.get(), true);
    ImGui_ImplVulkan_Init(pHALDevice->GetVkDevice(), pHALDevice->GetVkPhysicalDevice(), pHALDevice->GetVmaAllocator(), pHALRenderPass->GetVkRenderPass());

    float CPUFrameTime = 0.0f;
    float GPUFrameTime = 0.0f;
     
    struct WindowUserData {
        HAL::CommandQueue* pCommandQueue;
        HAL::SwapChain*    pSwapChain;
    } GLFWUserData = { (HAL::CommandQueue*)pHALComputeCommandQueue, pHALSwapChain.get() };

   
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
                    .ImageView = pHALSwapChain->GetImageView(frameID),
                    .ImageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                    .ClearValue = vk::ClearValue{ vk::ClearColorValue{ std::array<float, 4> {0.5f, 0.5f, 0.5f, 1.0f } }},
                    .Width = static_cast<uint32_t>(width),
                    .Height = static_cast<uint32_t>(height),
                    .LayerCount = 1            
                }         
            };


            HAL::DescriptorTable HALDescriptorTable = pHALDescritprorAllocator->Allocate(pHALComputePipeline->GetDescriptorTableLayout(0));        
     
            
            pHALCommandList->BeginRenderPass({.pRenderPass = pHALRenderPass.get(), .Attachments = renderPassAttachments});
            ImGui_ImplVulkan_NewFrame(pHALCommandList->GetVkCommandBuffer());               
            pHALCommandList->EndRenderPass();

            pHALCommandList->SetComputePipeline(*pHALComputePipeline, {});
          //  pHALCommandList->SetDescriptorTable(0, HALDescriptorTable);
          //  pHALCommandList->Dispatch(64, 64, 1);

        }
        pHALCommandList->End();
     
     

        //Wait fence nad Execute command List
        pHALGraphicsCommandQueue->Wait(*pHALFence, pHALFence->GetExpectedValue());  
        pHALGraphicsCommandQueue->ExecuteCommandLists({*pHALCommandList});
        pHALGraphicsCommandQueue->Signal(*pHALFence, pHALFence->Increment());
     
        //Wait fence and present
        pHALComputeCommandQueue->Present(*pHALSwapChain, frameID, *pHALFence);
        //------------------------------//


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

