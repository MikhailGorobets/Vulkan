#include "../include/AdapterImpl.hpp"
#include "../include/InstanceImpl.hpp"

namespace HAL {
    Adapter::Internal::Internal(vk::PhysicalDevice physicalDevice) {
        m_PhysicalDevice = physicalDevice;

        auto deviceFeatures = m_PhysicalDevice.getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan12Features,
            vk::PhysicalDevice16BitStorageFeatures
        >();

        auto deviceProperties = m_PhysicalDevice.getProperties2<
            vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceDriverProperties,
            vk::PhysicalDeviceVulkan12Properties
        >();

        m_Features.Features = deviceFeatures.get<vk::PhysicalDeviceFeatures2>().features;
        m_Features.Vulkan12Features = deviceFeatures.get<vk::PhysicalDeviceVulkan12Features>().setPNext(nullptr);
        m_Features.Shader16BitStorageFeatures = deviceFeatures.get<vk::PhysicalDevice16BitStorageFeatures>().setPNext(nullptr);

        m_Properties.Properties = deviceProperties.get<vk::PhysicalDeviceProperties2>().properties;
        m_Properties.DriverProperties = deviceProperties.get<vk::PhysicalDeviceDriverProperties>();
        m_Properties.Vulkan12Properties = deviceProperties.get<vk::PhysicalDeviceVulkan12Properties>();

        m_QueueFamilyProperies = m_PhysicalDevice.getQueueFamilyProperties();
        m_Extensions = m_PhysicalDevice.enumerateDeviceExtensionProperties();
    }

    auto Adapter::Internal::IsExtensionSupported(std::string const& name) const -> bool {
        return vkx::isDeviceExtensionAvailable(m_Extensions, name.c_str());
    }
}

namespace HAL {
    auto Adapter::GetVkPhysicalDevice() const -> vk::PhysicalDevice {
        return m_pInternal->GetVkPhysicalDevice();
    }
}