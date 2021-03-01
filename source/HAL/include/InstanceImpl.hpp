#pragma once

#include <HAL/Instance.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {
   
    class Instance::Internal {
    public:
        Internal(InstanceCreateInfo const& createInfo);

        auto GetInstance() const -> vk::Instance { return *m_pInstance; }

        auto GetVersion()  const -> uint32_t { return m_ApiVersion; }
    
        auto GetPhysicalDevices() const ->  std::vector<vk::PhysicalDevice> { m_PhysicalDevices; }

    private:
        vk::UniqueInstance  m_pInstance = {};
        uint32_t            m_ApiVersion = VK_API_VERSION_1_2;
       
        std::vector<vk::PhysicalDevice>      m_PhysicalDevices = {};
        std::vector<vk::LayerProperties>     m_LayerProperties = {};
        std::vector<vk::ExtensionProperties> m_ExtensionProperties = {};     
    };   
}