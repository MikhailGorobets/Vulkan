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

        auto GetVkInstance() const -> vk::Instance;
    private:    
        Internal_Ptr<Internal, InternalSize_Instance> m_pInternal;   
    };
}
   