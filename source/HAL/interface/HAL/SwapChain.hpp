#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    struct SwapChainCreateInfo {  
        uint32_t Width = {};
        uint32_t Height = {};  
        void*    WindowHandle = {};
        bool     IsSRGB = {};
        bool     IsVSync = {};
    };
    
    class SwapChain {
    public:
        class Internal;
    public:      
        SwapChain(Instance const& instance, Device const& device, SwapChainCreateInfo const& createInfo);
        
        ~SwapChain();

        auto Resize(uint32_t width, uint32_t height) -> void;

        auto GetFormat() const -> vk::Format;        

        auto GetImageView(uint32_t imageID) const -> vk::ImageView;         

        auto GetVkSwapChain() const -> vk::SwapchainKHR;

    private:      
        Internal_Ptr<Internal, InternalSize_SwapChain> m_pInternal;   
    };
}