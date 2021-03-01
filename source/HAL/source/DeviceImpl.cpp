

#include "../include/AdapterImpl.hpp"
#include "../include/InstanceImpl.hpp"
#include "../include/DeviceImpl.hpp"


namespace HAL {

    Device::Internal::Internal(Instance const& pInstance, Adapter const& pAdapter, DeviceCreateInfo const& createInfo) {

        const char* DEVICE_EXTENSION[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
            VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
        };

        auto pImplAdapter  = (Adapter::Internal*)(&pAdapter); 
        auto pImplInstance = (Instance::Internal*)(&pInstance); 
       
        std::vector<std::vector<float>> deviceQueuePriorities;
        std::vector<vk::DeviceQueueCreateInfo> deviceQueueCIs;

        auto GenerateQueueCreateInfo = [&deviceQueuePriorities, &deviceQueueCIs](vkx::QueueFamilyInfo queueInfo) -> void {
            std::vector<float> queuePriorities(queueInfo.queueCount);
            std::generate(queuePriorities.begin(), queuePriorities.end(), []() -> float { return 1.0f; });

            vk::DeviceQueueCreateInfo deviceQueueCI = {
                .queueFamilyIndex = queueInfo.queueIndex,
                .queueCount = static_cast<uint32_t>(std::size(queuePriorities)),
                .pQueuePriorities = std::data(queuePriorities)
            };
            deviceQueueCIs.push_back(deviceQueueCI);
            deviceQueuePriorities.push_back(std::move(queuePriorities));
        };

        if (m_QueueFamilyGraphics = vkx::getIndexQueueFamilyGraphicsInfo(pImplAdapter->GetQueueFamilyProperty())) {
            GenerateQueueCreateInfo(*m_QueueFamilyGraphics);
        } else {
            fmt::print("Error: Vulkan Device doesnt't support Graphics Queue Family \n");
        }

        if (m_QueueFamilyCompute  = vkx::getIndexQueueFamilyComputeInfo(pImplAdapter->GetQueueFamilyProperty())) {
            GenerateQueueCreateInfo(*m_QueueFamilyCompute);
        } else {
            fmt::print("Warning: Vulkan Device doesn't support Async Compute\n");
        }

        if (m_QueueFamilyTransfer = vkx::getIndexQueueFamilyTransferInfo(pImplAdapter->GetQueueFamilyProperty())) {
            GenerateQueueCreateInfo(*m_QueueFamilyTransfer);
        } else {
            fmt::print("Warning: Vulkan Device dosen't support Async Transfer\n");
        }

        auto const& deviceFeatures = pImplAdapter->GetFeatures();
        
        if (deviceFeatures.TimelineSemaphoreFeatures.timelineSemaphore) {
            fmt::print("Error: Vulkan Device dosen't support vk::PhysicalDeviceTimelineSemaphoreFeatures\n");
        }
        
        if (deviceFeatures.ImagelessFramebufferFeatures.imagelessFramebuffer) {
            fmt::print("Error: Vulkan Device dosen't support vk::PhysicalDeviceImagelessFramebufferFeaturesr\n");    
        }
        
        vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceShaderFloat16Int8Features,
            vk::PhysicalDeviceImagelessFramebufferFeatures,
            vk::PhysicalDeviceTimelineSemaphoreFeatures
        > enabledFeatures = {
            vk::PhysicalDeviceFeatures2 {
                .features = vk::PhysicalDeviceFeatures {
                    .shaderInt64 = deviceFeatures.Features.shaderInt64,
                    .shaderInt16 = deviceFeatures.Features.shaderInt16
                }
            },
            deviceFeatures.ShaderFloat16Int8Features,
            deviceFeatures.ImagelessFramebufferFeatures,
            deviceFeatures.TimelineSemaphoreFeatures
        };
        
        std::vector<const char*> deviceExtensions;
        for (size_t index = 0; index < _countof(DEVICE_EXTENSION); index++) {
            if (vkx::isDeviceExtensionAvailable(pImplAdapter->GetExtensions(), DEVICE_EXTENSION[index])) {
                deviceExtensions.push_back(DEVICE_EXTENSION[index]);
                continue;
            }
            fmt::print("Warning: Required Vulkan Device Extension {} not supported\n", DEVICE_EXTENSION[index]);
        }

        vk::DeviceCreateInfo deviceCI = {
            .pNext = &enabledFeatures.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = static_cast<uint32_t>(std::size(deviceQueueCIs)),
            .pQueueCreateInfos = std::data(deviceQueueCIs),
            .enabledExtensionCount = static_cast<uint32_t>(std::size(DEVICE_EXTENSION)),
            .ppEnabledExtensionNames = std::data(DEVICE_EXTENSION),
        };

        auto const& deviceInfo = pImplAdapter->GetProperties().Properties;
        
        m_pDevice = pImplAdapter->GetPhysicalDevice().createDeviceUnique(deviceCI);
        vkx::setDebugName(*m_pDevice, pImplAdapter->GetPhysicalDevice(), fmt::format("Name: {} Type: {}", deviceInfo.deviceName, vk::to_string(deviceInfo.deviceType)));
        vkx::setDebugName(*m_pDevice, pImplInstance->GetInstance(), fmt::format("ApiVersion: {}.{}.{}", VK_VERSION_MAJOR(deviceInfo.apiVersion), VK_VERSION_MINOR(deviceInfo.apiVersion), VK_VERSION_PATCH(deviceInfo.apiVersion)));
        vkx::setDebugName(*m_pDevice, *m_pDevice, deviceInfo.deviceName);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_pDevice);

        if (m_QueueFamilyGraphics) {
            for (size_t index = 0; index < m_QueueFamilyGraphics.value().queueCount; index++) {
                vk::Queue commandQueue = m_pDevice->getQueue(m_QueueFamilyGraphics.value().queueIndex, index);
                vkx::setDebugName(*m_pDevice, commandQueue, fmt::format("Graphics: [{}]", index));
                m_QueuesGraphics.push_back(commandQueue);
            }
        }

        if (m_QueueFamilyCompute) {
            for (size_t index = 0; index < m_QueueFamilyCompute.value().queueCount; index++) {
                vk::Queue commandQueue = m_pDevice->getQueue(m_QueueFamilyCompute.value().queueIndex, index);
                vkx::setDebugName(*m_pDevice, commandQueue, fmt::format("Compute: [{}]", index));
                m_QueuesGraphics.push_back(commandQueue);
            }
        }

        if (m_QueueFamilyTransfer) {
            for (size_t index = 0; index < m_QueueFamilyTransfer.value().queueCount; index++) {
                vk::Queue commandQueue = m_pDevice->getQueue(m_QueueFamilyTransfer.value().queueIndex, index);
                vkx::setDebugName(*m_pDevice, commandQueue, fmt::format("Transfer: [{}]", index));
                m_QueuesTransfer.push_back(commandQueue);
            }
        }
    }

}
  
namespace HAL {

    HAL::Device::Device(Instance const & pInstance, Adapter const & pAdapter, DeviceCreateInfo const & createInfo) : m_pInternal(pInstance, pAdapter, createInfo) { }

    auto Device::Flush() -> void { m_pInternal->Flush(); }

    auto Device::GetVkDevice() const -> vk::Device {
        return m_pInternal->GetDevice();
    }

    auto Device::GetVkPhysicalDevice() const -> vk::PhysicalDevice {
        return m_pInternal->GetPhysicalDevice();
    }

}