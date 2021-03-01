#include "..\include\AdapterImpl.hpp"
#include "..\include\InstanceImpl.hpp"

namespace HAL {

    Adapter::Internal::Internal(Instance const& instance, uint32_t adapterID) {
        auto pImplInstance = reinterpret_cast<const Instance::Internal*>(&instance);

        if (auto device = vkx::selectPhysicalDevice(pImplInstance->GetPhysicalDevices(), adapterID); device) {
            m_Device = *device;
        } else {
            fmt::print("Error: Device not found\n");
        }
        
        auto deviceFeatures = m_Device.getFeatures2<
             vk::PhysicalDeviceFeatures2,
             vk::PhysicalDeviceShaderFloat16Int8Features,
             vk::PhysicalDeviceTimelineSemaphoreFeatures,
             vk::PhysicalDeviceImagelessFramebufferFeatures
        >();
           
        auto deviceProperties = m_Device.getProperties2<
            vk::PhysicalDeviceProperties,
            vk::PhysicalDeviceDriverProperties,
            vk::PhysicalDeviceVulkan12Properties
        >(); 

        m_Features.Features = deviceFeatures.get<vk::PhysicalDeviceFeatures2>().features;
        m_Features.ShaderFloat16Int8Features = deviceFeatures.get<vk::PhysicalDeviceShaderFloat16Int8Features>();
        m_Features.TimelineSemaphoreFeatures = deviceFeatures.get<vk::PhysicalDeviceTimelineSemaphoreFeatures>();
        m_Features.ImagelessFramebufferFeatures = deviceFeatures.get<vk::PhysicalDeviceImagelessFramebufferFeatures>();

        m_Properties.Properties = deviceProperties.get<vk::PhysicalDeviceProperties>();
        m_Properties.DriverProperties = deviceProperties.get<vk::PhysicalDeviceDriverProperties>();
        m_Properties.Vulkan12Properties = deviceProperties.get<vk::PhysicalDeviceVulkan12Properties>();

        m_QueueFamilyProperies = m_Device.getQueueFamilyProperties();
        m_Extensions = m_Device.enumerateDeviceExtensionProperties();        
    }
}
