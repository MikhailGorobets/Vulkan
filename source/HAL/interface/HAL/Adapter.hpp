#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    class Adapter {
    public:
         class Internal;
    public:
         auto GetVkPhysicalDevice() const -> vk::PhysicalDevice; 
    private:     
        Internal_Ptr<Internal, InternalSize_Adapter> m_pInternal;
    };
}