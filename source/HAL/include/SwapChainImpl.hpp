#pragma once

#include <HAL/Instance.hpp>
#include <HAL/Device.hpp>
#include <HAL/SwapChain.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {
    class SwapChain::Internal {
    public:
        struct SynchronizationInfo {
            vk::Semaphore SemaphoresAvailable;
            vk::Semaphore SemaphoresFinished;
            vk::Fence     FenceCmdBufExecuted;
        };
    public:
        Internal(Instance const& instance, Device const& device, SwapChainCreateInfo const& createInfo);
        
        ~Internal();

        auto AcquireNextImage()->uint32_t;
        
        auto Present() -> void;
          
        auto Resize(uint32_t width, uint32_t height) -> void;
        
        auto GetSyncInfo() -> SynchronizationInfo;

        auto GetFormat() const -> vk::Format { return m_SurfaceFormat.format; }

        auto GetCurrentImage() const -> vk::Image { return m_SwapChainImages[m_CurrentImageIndex]; }        

        auto GetCurrentImageView() const -> vk::ImageView { return *m_SwapChainImageViews[m_CurrentImageIndex]; };
        
        auto GetVkSwapChain() const -> vk::SwapchainKHR { return *m_pSwapChain; }

    private:
        auto CreateSurface() -> void;

        auto CreateSwapChain(uint32_t width, uint32_t height) -> void;
        
        auto CreateSyncPrimitives() -> void;

        auto WaitAcquiredFencesAndReset() -> void;

    private:
        vk::Instance        m_Instance = {};
        vk::Device          m_Device   = {};
        vk::PhysicalDevice  m_PhysicalDevice = {};
        vk::Queue           m_PresentQueue = {};
        uint32_t            m_QueueFamilyIndex = {};
      
        HWND                   m_WindowHandle = {};
        vk::UniqueSurfaceKHR   m_pSurface = {};
        vk::UniqueSwapchainKHR m_pSwapChain = {};
        
        vk::SurfaceCapabilitiesKHR m_SurfaceCapabilities = {};
        vk::SurfaceFormatKHR       m_SurfaceFormat = {};
        vk::PresentModeKHR         m_PresentMode = {};     

        std::vector<vk::Image>           m_SwapChainImages = {};   
        std::vector<vk::UniqueImageView> m_SwapChainImageViews = {};
        
        uint32_t m_BufferCount = {};
        uint32_t m_CurrentBufferIndex = {};
        uint32_t m_CurrentImageIndex = {};
        
        std::vector<vk::UniqueSemaphore> m_SwapChainSemaphoresAvailable = {};
        std::vector<vk::UniqueSemaphore> m_SwapChainSemaphoresFinished = {};
        std::vector<vk::UniqueFence>     m_SwapChainFences = {};
        
        bool m_IsVSyncEnabled = {};
        bool m_IsSRGBEnabled = {};             
    };
}
  