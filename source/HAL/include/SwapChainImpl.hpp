#pragma once

#include <HAL/Instance.hpp>
#include <HAL/Device.hpp>
#include <HAL/SwapChain.hpp>
#include <vulkan/vulkan_decl.h>
#include <queue>

namespace HAL {
    class SwapChain::Internal {
    public:
        friend class CommandQueue;
    public:
        Internal(Instance const& instance, Device const& device, SwapChainCreateInfo const& createInfo);       
        
        auto Acquire() -> void;

        auto Release() -> void;
        
        auto Resize(uint32_t width, uint32_t height) -> void;

        auto GetFormat() const -> vk::Format { return m_SurfaceFormat.format; }

        auto GetImage(uint32_t frameID) const -> vk::Image { return m_SwapChainImages[frameID]; }        

        auto GetImageView(uint32_t frameID) const -> vk::ImageView { return *m_SwapChainImageViews[frameID]; };
        
        auto GetSwapChain() const -> vk::SwapchainKHR { return *m_pSwapChain; }
        
        auto GetDevice() const -> vk::Device { return m_pSwapChain.getOwner(); }

        auto GetSemaphoreAvailable() const -> vk::Semaphore { return *m_SemaphoresAvailable[m_BufferIndices.front()]; }

        auto GetSemaphoreFinished() const -> vk::Semaphore { return *m_SemaphoresFinished[m_BufferIndices.front()]; }

    private:
        auto CreateSurface(Instance const& instance, Device const& device) -> void;

        auto CreateSwapChain(vk::Device device, uint32_t width, uint32_t height) -> void;
        
        auto CreateSyncPrimitives(vk::Device device) -> void;      

    private:
        vk::PhysicalDevice    m_PhysicalDevice = {};
        std::vector<uint32_t> m_QueueFamilyIndices = {};
      
        HWND                   m_WindowHandle = {};
        vk::UniqueSurfaceKHR   m_pSurface = {};
        vk::UniqueSwapchainKHR m_pSwapChain = {};
        
        vk::SurfaceCapabilitiesKHR m_SurfaceCapabilities = {};
        vk::SurfaceFormatKHR       m_SurfaceFormat = {};
        vk::PresentModeKHR         m_PresentMode = {};     

        std::vector<vk::Image>           m_SwapChainImages = {};   
        std::vector<vk::UniqueImageView> m_SwapChainImageViews = {};
        
        uint32_t m_BufferCount = {};
        uint32_t m_BufferIndex = {};
      
        std::vector<vk::UniqueSemaphore> m_SemaphoresAvailable = {};
        std::vector<vk::UniqueSemaphore> m_SemaphoresFinished = {};
        std::queue<uint32_t>             m_BufferIndices = {};        
 
        bool m_IsVSyncEnabled = {};
        bool m_IsSRGBEnabled = {}; 
        bool m_IsFullScreenExclusiveSupported = {};            
    };
}
  