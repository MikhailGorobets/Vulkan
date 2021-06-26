#pragma once

#include <HAL/Instance.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    class Instance::Internal {
    public:
        Internal(InstanceCreateInfo const& createInfo);

        auto GetVkInstance() const -> vk::Instance { return *m_pInstance; }

        auto GetVkVersion() const -> uint32_t { return m_ApiVersion; }

        auto GetAdapters() const -> std::vector<HAL::Adapter> const& { return m_Adapters; }

    private:
        vk::UniqueInstance  m_pInstance = {};
        uint32_t            m_ApiVersion = VK_API_VERSION_1_2;

        std::vector<HAL::Adapter>            m_Adapters = {};
        std::vector<vk::LayerProperties>     m_LayerProperties = {};
        std::vector<vk::ExtensionProperties> m_ExtensionProperties = {};
    };
}