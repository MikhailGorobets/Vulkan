#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    class Fence {
    public:
        class Internal;
    public:
        auto Signal(uint64_t value);

        auto Wait(uint64_t value);
    
        auto GetCompletedValue() const -> uint64_t;

    private:     
        Internal_Ptr<Internal, InternalSize_Fence> m_pInternal;               
    };
}