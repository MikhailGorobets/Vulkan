#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    class Fence {
    public:
        class Internal;
    public:
        Fence(Device const& device, uint64_t value);

        auto Signal(uint64_t value) const;

        auto Wait(uint64_t value) const;
    
        auto GetCompletedValue() const -> uint64_t;
        
        auto GetVkSemaphore() const -> vk::Semaphore;    
    
    private:     
        Internal_Ptr<Internal, InternalSize_Fence> m_pInternal;               
    };
}