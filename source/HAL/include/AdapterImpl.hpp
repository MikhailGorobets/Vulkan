#pragma once

#include <HAL/Adapter.hpp>
#include <HAL/Instance.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    class Adapter::Internal {
    public:
        struct DeviceFeatures {
            vk::PhysicalDeviceFeatures Features = {};
            vk::PhysicalDeviceVulkan12Features Vulkan12Features = {};
            vk::PhysicalDevice16BitStorageFeatures Shader16BitStorageFeatures = {};
        };

        struct DeviceProperties {
            vk::PhysicalDeviceProperties Properties = {};
            vk::PhysicalDeviceDriverProperties DriverProperties = {};
            vk::PhysicalDeviceVulkan12Properties Vulkan12Properties = {};
        };

    public:
        Internal() = default;

        Internal(vk::PhysicalDevice physicalDevice);

        auto GetProperties() const -> DeviceProperties const& { return m_Properties; }

        auto GetFeatures() const -> DeviceFeatures const& { return m_Features; }

        auto GetVkPhysicalDevice() const -> vk::PhysicalDevice { return m_PhysicalDevice; }

        auto GetQueueFamilyProperty() const -> std::vector<vk::QueueFamilyProperties> const& { return m_QueueFamilyProperies; }

        auto GetExtensions() const -> std::vector<vk::ExtensionProperties> const& { return m_Extensions; }

        auto IsExtensionSupported(std::string const& name) const -> bool;

    private:
        vk::PhysicalDevice                     m_PhysicalDevice;
        DeviceProperties                       m_Properties;
        DeviceFeatures                         m_Features;
        std::vector<vk::QueueFamilyProperties> m_QueueFamilyProperies;
        std::vector<vk::ExtensionProperties>   m_Extensions;
    };
}