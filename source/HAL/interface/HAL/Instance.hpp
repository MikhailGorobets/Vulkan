#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    struct InstanceCreateInfo {
        bool IsEnableValidationLayers = {};
        bool IsEnableDebugUtils = {};
    };    

    class Instance {
    public:
        class Internal;
    public:     
        Instance(InstanceCreateInfo const& createInfo);

        ~Instance();

        auto GetVkInstance() const -> vk::Instance;

        auto GetAdapters() const -> std::vector<HAL::Adapter> const&;

    private:    
        Internal_Ptr<Internal, InternalSize_Instance> m_pInternal;   
    };
}
   