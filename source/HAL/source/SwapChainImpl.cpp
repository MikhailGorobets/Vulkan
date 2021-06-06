#include <vulkan/vulkan_decl.h>

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
        m_PresentQueue = pDeviceImpl->GetGraphicsCommandQueue()->GetVkQueue();
        m_QueueFamilyIndex = pDeviceImpl->GetGraphicsQueueFamilyIndex();
              

        m_IsSRGBEnabled = createInfo.IsSRGB;
        m_IsVSyncEnabled = createInfo.IsVSync;
        m_BufferCount = createInfo.BufferCount;
        m_WindowHandle = reinterpret_cast<HWND>(createInfo.WindowHandle);

        this->CreateSurface();
        this->CreateSwapChain(createInfo.Width, createInfo.Height);
        this->CreateSyncPrimitives();
    }

    SwapChain::Internal::~Internal() {
        this->WaitAcquiredFencesAndReset();
    }

    auto SwapChain::Internal::AcquireNextImage() -> void {
        //TODO lock
        m_CurrentImageIndex = m_Device.acquireNextImageKHR(m_pSwapChain.get(), std::numeric_limits<uint64_t>::max(), *m_SwapChainSemaphoresAvailable[m_CurrentBufferIndex], nullptr);
           
        vk::Semaphore semaphores[] = { *m_SwapChainSemaphoresAvailable[m_CurrentBufferIndex] }; 

        vk::PipelineStageFlags stageMask[] = { vk::PipelineStageFlagBits::eBottomOfPipe };

        vk::SubmitInfo submitInfo = {
            .waitSemaphoreCount = _countof(semaphores),
            .pWaitSemaphores = semaphores,
            .pWaitDstStageMask = stageMask        
        };        
        m_PresentQueue.submit(submitInfo, nullptr);  
    }

    auto SwapChain::Internal::Present(Fence& fence) -> void {
        //TODO lock        

        uint64_t fenceValue = fence.Increment();

        uint64_t signalSemaphoreValues[] = { fence.GetExpectedValue(), 0 };
        uint64_t waitSemaphoreValues[] = { fence.GetCompletedValue() };

        vk::Semaphore signalSemaphores[] = { fence.GetVkSemaphore(), *m_SwapChainSemaphoresFinished[m_CurrentBufferIndex] }; 
        vk::Semaphore waitSemahores[] = { fence.GetVkSemaphore() };
        
        vk::PipelineStageFlags stageMask[] = { vk::PipelineStageFlagBits::eBottomOfPipe };
        
        vk::TimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo = {
            .waitSemaphoreValueCount = _countof(waitSemaphoreValues),
            .pWaitSemaphoreValues = waitSemaphoreValues,      
            .signalSemaphoreValueCount = _countof(signalSemaphoreValues),
            .pSignalSemaphoreValues = signalSemaphoreValues
        };

        vk::SubmitInfo submitInfo = {
            .pNext = &timelineSemaphoreSubmitInfo,
            .waitSemaphoreCount =  _countof(waitSemahores),
            .pWaitSemaphores = waitSemahores,
            .pWaitDstStageMask = stageMask,
            .signalSemaphoreCount = _countof(signalSemaphores),
            .pSignalSemaphores = signalSemaphores,           
        };            

        m_PresentQueue.submit(submitInfo, nullptr);

        vk::PresentInfoKHR presentInfo = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = m_SwapChainSemaphoresFinished[m_CurrentBufferIndex].getAddressOf(),
            .swapchainCount = 1,
            .pSwapchains = m_pSwapChain.getAddressOf(),
            .pImageIndices = &m_CurrentImageIndex
        };
        std::ignore = m_PresentQueue.presentKHR(presentInfo);
        m_CurrentBufferIndex = (m_CurrentBufferIndex + 1) % m_BufferCount;
    }

    auto SwapChain::Internal::Resize(uint32_t width, uint32_t height) -> void {
        this->WaitAcquiredFencesAndReset();
        this->CreateSwapChain(width, height);
    }

    auto SwapChain::Internal::CreateSurface() -> void {
        vk::Win32SurfaceCreateInfoKHR surfaceCI = {
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = m_WindowHandle
        };

        m_pSurface = m_Instance.createWin32SurfaceKHRUnique(surfaceCI);
        vkx::setDebugName(m_Device, *m_pSurface, "Win32SurfaceKHR");

        if (!m_PhysicalDevice.getSurfaceSupportKHR(m_QueueFamilyIndex, *m_pSurface)) 
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

        vk::SwapchainCreateInfoKHR swapchainCI = {
            .surface = *m_pSurface,
            .minImageCount = m_SurfaceCapabilities.minImageCount + 1,
            .imageFormat = m_SurfaceFormat.format,
            .imageColorSpace = m_SurfaceFormat.colorSpace,
            .imageExtent = { .width = width, .height = height },
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .presentMode = m_PresentMode,
            .clipped = true,
            .oldSwapchain = *m_pSwapChain
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
        for (size_t index = 0; index < m_BufferCount; index++) {
            auto pSemaphoresAvailable = m_Device.createSemaphoreUnique(vk::SemaphoreCreateInfo{});
            auto pSemaphoresFinished = m_Device.createSemaphoreUnique(vk::SemaphoreCreateInfo{});         

            vkx::setDebugName(m_Device, *pSemaphoresAvailable, fmt::format("SwapChain Available[{}]", index));
            vkx::setDebugName(m_Device, *pSemaphoresFinished, fmt::format("SwapChain Finished[{}]", index));        

            m_SwapChainSemaphoresAvailable.push_back(std::move(pSemaphoresAvailable));
            m_SwapChainSemaphoresFinished.push_back((std::move(pSemaphoresFinished)));
        }
    }

    auto SwapChain::Internal::WaitAcquiredFencesAndReset() -> void {          
        m_PresentQueue.waitIdle();
        m_SwapChainImageViews.clear();
        m_pSwapChain.reset();
        m_CurrentBufferIndex = 0;
    }
}

 
namespace HAL {
    SwapChain::SwapChain(Instance const& instance, Device const& device, SwapChainCreateInfo const& createInfo) : m_pInternal(instance, device, createInfo) { }

    SwapChain::~SwapChain() = default;

    auto SwapChain::AcquireNextImage() -> void {
        m_pInternal->AcquireNextImage();
    }

    auto SwapChain::Present(Fence& fence) -> void {
        m_pInternal->Present(fence);
    }

    auto SwapChain::Resize(uint32_t width, uint32_t height) -> void {
        m_pInternal->Resize(width, height);
    }

    auto SwapChain::GetCurrentImageView() const -> vk::ImageView {
        return m_pInternal->GetCurrentImageView();
    }

    auto SwapChain::GetVkSwapChain() const -> vk::SwapchainKHR {
        return m_pInternal->GetSwapChain();
    }
}