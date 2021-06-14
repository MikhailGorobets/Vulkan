#include "../include/InstanceImpl.hpp"
#include "../include/AdapterImpl.hpp"

namespace HAL {
    Instance::Internal::Internal(InstanceCreateInfo const& createInfo) {

        const char* INSTANCE_LAYERS[] = {
            "VK_LAYER_KHRONOS_validation"
        };

        const char* INSTANCE_EXTENSION[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif            
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };

        static vk::DynamicLoader vulkanDynamicLoader;
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkanDynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
        
        m_ApiVersion = vk::enumerateInstanceVersion();
        m_LayerProperties = vk::enumerateInstanceLayerProperties();
        m_ExtensionProperties = vk::enumerateInstanceExtensionProperties();

        if (m_ApiVersion < VK_VERSION_1_2) {
            fmt::print("Error: Vulkan 1.2 minimum supported version \n");
        }

        std::vector<const char*> instanceLayers;
        if (createInfo.IsEnableValidationLayers) {
            for (size_t index = 0; index < _countof(INSTANCE_LAYERS); index++) {
                if (vkx::isInstanceLayerAvailable(m_LayerProperties, INSTANCE_LAYERS[index])) {
                    instanceLayers.push_back(INSTANCE_LAYERS[index]);
                    continue;
                }                     
                fmt::print("Warning: Required Vulkan Instance Layer {0} not supported \n", INSTANCE_LAYERS[index]);
            }
        }
       
        std::vector<const char*> instanceExtensions;  
        for (size_t index = 0; index < _countof(INSTANCE_EXTENSION); index++) {
            if (vkx::isInstanceExtensionAvailable(m_ExtensionProperties, INSTANCE_EXTENSION[index])) {
                instanceExtensions.push_back(INSTANCE_EXTENSION[index]);
                continue;
            }
            fmt::print("Warning: Required Vulkan Instance Extension {0} not supported \n", INSTANCE_EXTENSION[index]);
        }

        vk::ApplicationInfo applicationInfo = {
            .pApplicationName = nullptr,
            .applicationVersion = 0,
            .pEngineName = "HAL",
            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
            .apiVersion = VK_API_VERSION_1_2
        };

        vk::InstanceCreateInfo instanceCI = {
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<uint32_t>(std::size(instanceLayers)),
            .ppEnabledLayerNames = std::data(instanceLayers),
            .enabledExtensionCount = static_cast<uint32_t>(std::size(instanceExtensions)),
            .ppEnabledExtensionNames = std::data(instanceExtensions)
        };
        m_pInstance = vk::createInstanceUnique(instanceCI);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_pInstance);

        for(auto const& physicalDevice: m_pInstance->enumeratePhysicalDevices()) {     
            auto adapter = HAL::Adapter::Internal(physicalDevice);
            m_Adapters.push_back(std::move(*reinterpret_cast<HAL::Adapter*>(&adapter)));
        }         
    }    
}

namespace HAL {
    Instance::Instance(InstanceCreateInfo const& createInfo) : m_pInternal(createInfo) {}

    Instance::~Instance() = default;

    auto Instance::GetVkInstance() const -> vk::Instance {
        return m_pInternal->GetInstance();
    }

    auto Instance::GetAdapters() const -> std::vector<HAL::Adapter> const& {
        return m_pInternal->GetAdapters();
    }
}