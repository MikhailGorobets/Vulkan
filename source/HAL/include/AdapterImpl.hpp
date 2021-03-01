#pragma once

#include <HAL/Adapter.hpp>
#include <HAL/Instance.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    class Adapter::Internal {
    public:
        struct DeviceFeatures {
            vk::PhysicalDeviceFeatures Features  = {};
            vk::PhysicalDeviceShaderFloat16Int8Features ShaderFloat16Int8Features = {};
            vk::PhysicalDeviceTimelineSemaphoreFeatures TimelineSemaphoreFeatures = {};
            vk::PhysicalDeviceImagelessFramebufferFeatures ImagelessFramebufferFeatures = {};          
        };
        
        struct DeviceProperties {
            vk::PhysicalDeviceProperties Properties = {};
            vk::PhysicalDeviceDriverProperties DriverProperties = {};
            vk::PhysicalDeviceVulkan12Properties Vulkan12Properties = {};         
        };

    public:
        Internal(Instance const& instance, uint32_t adapterID); 
        
        auto GetProperties()          const -> DeviceProperties const& { return m_Properties; }

        auto GetFeatures()            const -> DeviceFeatures const& { return m_Features; }
        
        auto GetPhysicalDevice()      const -> vk::PhysicalDevice { return m_Device; }

        auto GetQueueFamilyProperty() const -> std::vector<vk::QueueFamilyProperties> const& { return m_QueueFamilyProperies; }
    
        auto GetExtensions()          const -> std::vector<vk::ExtensionProperties> const& { return m_Extensions; }

    private:
        vk::PhysicalDevice                     m_Device;    
        DeviceProperties                       m_Properties;
        DeviceFeatures                         m_Features; 
        std::vector<vk::QueueFamilyProperties> m_QueueFamilyProperies;
        std::vector<vk::ExtensionProperties>   m_Extensions;
    };
    
}