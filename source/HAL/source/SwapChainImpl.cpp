#include "../include/SwapChainImpl.hpp"
#include "../include/InstanceImpl.hpp"
#include "../include/DeviceImpl.hpp"
#include "../include/FenceImpl.hpp"
#include "../include/CommandQueueImpl.hpp"


namespace HAL {
    SwapChain::Internal::Internal(Instance const& instance, Device const& device, SwapChainCreateInfo const& createInfo) {      
        auto pDeviceImpl = (Device::Internal*)(&device);
        auto pInstanceImpl = (Instance::Internal*)(&instance);

        m_Device = pDeviceImpl->GetDevice();
        m_Instance = pInstanceImpl->GetInstance();
        m_PhysicalDevice = pDeviceImpl->GetPhysicalDevice();

        m_IsSRGBEnabled = createInfo.IsSRGB;
        m_IsVSyncEnabled = createInfo.IsVSync;
        m_WindowHandle = reinterpret_cast<HWND>(createInfo.WindowHandle);

        this->CreateSurface(device);
        this->CreateSwapChain(createInfo.Width, createInfo.Height);
        this->CreateSyncPrimitives();
    }

    SwapChain::Internal::~Internal() {}

    auto SwapChain::Internal::Acquire() -> void {
        m_BufferIndices.push(m_BufferIndex++ % m_BufferCount);
        assert(m_BufferIndices.size() <= m_BufferCount);
    }

    auto SwapChain::Internal::Release() -> void {
        m_BufferIndices.pop();
    }


    auto SwapChain::Internal::CreateSurface(Device const& device) -> void {
        vk::Win32SurfaceCreateInfoKHR surfaceCI = {
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = m_WindowHandle
        };

        m_pSurface = m_Instance.createWin32SurfaceKHRUnique(surfaceCI);
        vkx::setDebugName(m_Device, *m_pSurface, "Win32SurfaceKHR");  
    
        auto pDeviceImpl = (Device::Internal*)(&device);
    
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

    auto SwapChain::Internal::CreateSwapChain(uint32_t width, uint32_t height) -> void {
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
            .imageExtent = { .width = width, .height = height },
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = std::size(m_QueueFamilyIndices) > 1 ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = static_cast<uint32_t>(std::size(m_QueueFamilyIndices)),
            .pQueueFamilyIndices = std::data(m_QueueFamilyIndices),
            .presentMode = m_PresentMode, 
            .clipped = true
        };

        m_pSwapChain = m_Device.createSwapchainKHRUnique(swapchainCI);
        vkx::setDebugName(m_Device, *m_pSwapChain, fmt::format("Format: {} PresentMode: {} Width: {} Height: {}", vk::to_string(swapchainCI.imageFormat), vk::to_string(swapchainCI.presentMode), width, height));


        m_SwapChainImages = m_Device.getSwapchainImagesKHR(*m_pSwapChain);
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
            auto pImageView = m_Device.createImageViewUnique(imageViewCI);
            vkx::setDebugName(m_Device, m_SwapChainImages[index], fmt::format("SwapChain[{0}]", index));
            vkx::setDebugName(m_Device, *pImageView, fmt::format("SwapChain[{0}]", index));
            m_SwapChainImageViews.push_back(std::move(pImageView));
        }        
    }

    auto SwapChain::Internal::CreateSyncPrimitives() -> void {
        for (uint32_t index = 0; index < m_BufferCount; index++) {
            auto pSemaphoresAvailable = m_Device.createSemaphoreUnique(vk::SemaphoreCreateInfo{});
            auto pSemaphoresFinished = m_Device.createSemaphoreUnique(vk::SemaphoreCreateInfo{});         

            vkx::setDebugName(m_Device, *pSemaphoresAvailable, fmt::format("SwapChain Available[{}]", index));
            vkx::setDebugName(m_Device, *pSemaphoresFinished, fmt::format("SwapChain Finished[{}]", index));        

            m_SemaphoresAvailable.push_back(std::move(pSemaphoresAvailable));
            m_SemaphoresFinished.push_back((std::move(pSemaphoresFinished)));
        }
    }
}

 
namespace HAL {
    SwapChain::SwapChain(Instance const& instance, Device const& device, SwapChainCreateInfo const& createInfo) : m_pInternal(instance, device, createInfo) { }

    SwapChain::~SwapChain() = default;

    auto SwapChain::GetImageView(uint32_t imageID) const -> vk::ImageView {
        return m_pInternal->GetImageView(imageID);
    }


    auto SwapChain::GetVkSwapChain() const -> vk::SwapchainKHR {
        return m_pInternal->GetSwapChain();
    }
}