#include "../include/SwapChainImpl.hpp"
#include "../include/InstanceImpl.hpp"
#include "../include/DeviceImpl.hpp"
#include "../include/FenceImpl.hpp"
#include "../include/CommandQueueImpl.hpp"


namespace HAL {
    SwapChain::Internal::Internal(Instance const& instance, Device const& device, SwapChainCreateInfo const& createInfo) {      
        m_PhysicalDevice = device.GetVkPhysicalDevice();
        m_IsSRGBEnabled = createInfo.IsSRGB;
        m_IsVSyncEnabled = createInfo.IsVSync;
        m_WindowHandle = reinterpret_cast<HWND>(createInfo.WindowHandle);

        this->CreateSurface(instance, device);
        this->CreateSwapChain(device.GetVkDevice(), createInfo.Width, createInfo.Height);
        this->CreateSyncPrimitives(device.GetVkDevice());
    }

    SwapChain::Internal::~Internal() {}

    auto SwapChain::Internal::Acquire() -> void {
        m_BufferIndices.push(m_BufferIndex++ % m_BufferCount);
        assert(m_BufferIndices.size() <= m_BufferCount);
    }

    auto SwapChain::Internal::Release() -> void {
        m_BufferIndices.pop();
    }

    auto SwapChain::Internal::CreateSurface(Instance const& instance, Device const& device) -> void {
        vk::Win32SurfaceCreateInfoKHR surfaceCI = {
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = m_WindowHandle
        };
        
        auto pDeviceImpl = reinterpret_cast<const Device::Internal*>(&device);

        m_pSurface = reinterpret_cast<const Instance::Internal*>(&instance)->GetInstance().createWin32SurfaceKHRUnique(surfaceCI);
        vkx::setDebugName(pDeviceImpl->GetDevice(), *m_pSurface, "Win32SurfaceKHR");  
        
        if (m_PhysicalDevice.getSurfaceSupportKHR(pDeviceImpl->GetGraphicsQueueFamilyIndex(), *m_pSurface)) 
            m_QueueFamilyIndices.push_back(pDeviceImpl->GetGraphicsQueueFamilyIndex());
         
        if (m_PhysicalDevice.getSurfaceSupportKHR(pDeviceImpl->GetComputeQueueFamilyIndex(), *m_pSurface)) 
            m_QueueFamilyIndices.push_back(pDeviceImpl->GetComputeQueueFamilyIndex());

        if (m_PhysicalDevice.getSurfaceSupportKHR(pDeviceImpl->GetTransferQueueFamilyIndex(), *m_pSurface)) 
            m_QueueFamilyIndices.push_back(pDeviceImpl->GetTransferQueueFamilyIndex());

        std::sort(std::begin(m_QueueFamilyIndices), std::end(m_QueueFamilyIndices));
        m_QueueFamilyIndices.erase(std::unique(std::begin(m_QueueFamilyIndices), std::end(m_QueueFamilyIndices)), std::end(m_QueueFamilyIndices));
        
        if(m_QueueFamilyIndices.empty())
            fmt::print("Error: Select surface isn't supported by physical device");     
    }

    auto SwapChain::Internal::CreateSwapChain(vk::Device device, uint32_t width, uint32_t height) -> void {

        auto SelectSurfaceFormat = [](vk::PhysicalDevice adapter, vk::SurfaceKHR surface, bool isSRGB) -> std::optional<vk::SurfaceFormatKHR> {
            std::vector<vk::SurfaceFormatKHR> supportedFormats = adapter.getSurfaceFormatsKHR(surface);

            std::vector<vk::Format> preferedFormats;
            if (isSRGB) {
                preferedFormats.push_back(vk::Format::eR8G8B8A8Srgb);
                preferedFormats.push_back(vk::Format::eB8G8R8A8Srgb);
            }
            else {
                preferedFormats.push_back(vk::Format::eR8G8B8A8Unorm);
                preferedFormats.push_back(vk::Format::eB8G8R8A8Unorm);
            }
            return vkx::selectPresentFormat(preferedFormats, supportedFormats);
        };

        auto SelectPresentMode = [](vk::PhysicalDevice adapter, vk::SurfaceKHR surface, bool isVSync) -> std::optional<vk::PresentModeKHR> {
            std::vector<vk::PresentModeKHR> supportedPresentModes = adapter.getSurfacePresentModesKHR(surface);

            std::vector<vk::PresentModeKHR> preferedPresentModes;
            if (isVSync) {
                preferedPresentModes.push_back(vk::PresentModeKHR::eFifoRelaxed);
                preferedPresentModes.push_back(vk::PresentModeKHR::eFifo);
            }
            else {
                preferedPresentModes.push_back(vk::PresentModeKHR::eImmediate);
                preferedPresentModes.push_back(vk::PresentModeKHR::eMailbox);
                preferedPresentModes.push_back(vk::PresentModeKHR::eFifo);
            }
            return vkx::selectPresentMode(preferedPresentModes, supportedPresentModes);
        };

        if (auto presentMode = SelectPresentMode(m_PhysicalDevice, *m_pSurface, m_IsVSyncEnabled)) {
            m_PresentMode = *presentMode;
        } else {
            fmt::print("Error: ....TODO");
        }
        
        if (auto format = SelectSurfaceFormat(m_PhysicalDevice, *m_pSurface, m_IsSRGBEnabled)) {
            m_SurfaceFormat = *format;
        } else {
            fmt::print("Error: ....TODO");
        }
          
        m_SurfaceCapabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(*m_pSurface);
        m_BufferCount = std::min(m_SurfaceCapabilities.minImageCount + 1, m_SurfaceCapabilities.maxImageCount);


        vk::SwapchainCreateInfoKHR swapchainCI = {
            .surface = *m_pSurface,
            .minImageCount = m_BufferCount,
            .imageFormat = m_SurfaceFormat.format,
            .imageColorSpace = m_SurfaceFormat.colorSpace,
            .imageExtent = { 
                .width = std::clamp(width, m_SurfaceCapabilities.minImageExtent.width, m_SurfaceCapabilities.maxImageExtent.width), 
                .height = std::clamp(height, m_SurfaceCapabilities.minImageExtent.height, m_SurfaceCapabilities.maxImageExtent.height) 
            },
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = std::size(m_QueueFamilyIndices) > 1 ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = static_cast<uint32_t>(std::size(m_QueueFamilyIndices)),
            .pQueueFamilyIndices = std::data(m_QueueFamilyIndices),
            .presentMode = m_PresentMode, 
            .clipped = true
        };

        m_pSwapChain = device.createSwapchainKHRUnique(swapchainCI);
        vkx::setDebugName(device, *m_pSwapChain, fmt::format("Format: {} PresentMode: {} Width: {} Height: {}", vk::to_string(swapchainCI.imageFormat), vk::to_string(swapchainCI.presentMode), width, height));


        m_SwapChainImages = device.getSwapchainImagesKHR(*m_pSwapChain);
        for (size_t index = 0; index < std::size(m_SwapChainImages); index++) {
            vk::ImageViewCreateInfo imageViewCI = {
                .image = m_SwapChainImages[index],
                .viewType = vk::ImageViewType::e2D,
                .format = m_SurfaceFormat.format,
                .subresourceRange = {
                    .aspectMask = vkx::getImageAspectFlags(m_SurfaceFormat.format),
                    .baseMipLevel = 0,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS
            }
            };
            auto pImageView = device.createImageViewUnique(imageViewCI);
            vkx::setDebugName(device, m_SwapChainImages[index], fmt::format("SwapChain[{0}]", index));
            vkx::setDebugName(device, *pImageView, fmt::format("SwapChain[{0}]", index));
            m_SwapChainImageViews.push_back(std::move(pImageView));
        }        
    }

    auto SwapChain::Internal::CreateSyncPrimitives(vk::Device device) -> void {
        for (uint32_t index = 0; index < m_BufferCount; index++) {
            auto pSemaphoresAvailable = device.createSemaphoreUnique(vk::SemaphoreCreateInfo{});
            auto pSemaphoresFinished = device.createSemaphoreUnique(vk::SemaphoreCreateInfo{});         

            vkx::setDebugName(device, *pSemaphoresAvailable, fmt::format("SwapChain Available[{}]", index));
            vkx::setDebugName(device, *pSemaphoresFinished, fmt::format("SwapChain Finished[{}]", index));        

            m_SemaphoresAvailable.push_back(std::move(pSemaphoresAvailable));
            m_SemaphoresFinished.push_back((std::move(pSemaphoresFinished)));
        }
    }

    auto SwapChain::Internal::Resize(uint32_t width, uint32_t height) -> void {
        auto device = m_pSwapChain.getOwner();

        std::queue<uint32_t>().swap(m_BufferIndices);
        m_SwapChainImageViews.clear();
        m_pSwapChain.reset();
        this->CreateSwapChain(device, width, height);       
    }
}

 
namespace HAL {
    SwapChain::SwapChain(Instance const& instance, Device const& device, SwapChainCreateInfo const& createInfo) : m_pInternal(instance, device, createInfo) { }

    SwapChain::~SwapChain() = default;

    auto SwapChain::Resize(uint32_t width, uint32_t height) -> void  {
        m_pInternal->Resize(width, height);
    }

    auto SwapChain::GetFormat() const -> vk::Format  {
        return m_pInternal->GetFormat();
    }

    auto SwapChain::GetImageView(uint32_t imageID) const -> vk::ImageView {
        return m_pInternal->GetImageView(imageID);
    }

    auto SwapChain::GetVkSwapChain() const -> vk::SwapchainKHR {
        return m_pInternal->GetSwapChain();
    }
}