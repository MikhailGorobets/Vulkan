#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    struct InstanceCreateInfo {
        bool IsEnableValidationLayers = {};
        bool IsEnableDebugUtils = {};
    };    

    class Instance: NonCopyable {
    public:
        class Internal;
    public:     
        Instance(InstanceCreateInfo const& createInfo);

        Instance(Instance&&) noexcept;

        Instance& operator=(Instance&&) noexcept;   

        ~Instance();

        auto GetVkInstance() const -> vk::Instance;

        auto GetAdapters() const -> std::vector<HAL::Adapter> const&;

    private:    
        InternalPtr<Internal, InternalSize_Instance> m_pInternal;   
    };
}
   