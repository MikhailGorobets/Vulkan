#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    class Fence: NonCopyable {
    public:
        class Internal;
    public:
        Fence(Device const& device, std::optional<uint64_t> value = std::nullopt);
        
        Fence(Fence&&);
        
        Fence& operator=(Fence&&);        

        ~Fence();

        auto Signal(uint64_t value) const -> void;

        auto Wait(uint64_t value) const -> void;
    
        auto GetCompletedValue() const -> uint64_t;
        
        auto GetExpectedValue() const -> uint64_t;
        
        auto Increment() -> uint64_t;

        auto IsCompleted() const -> bool;        

        auto GetVkSemaphore() const -> vk::Semaphore;    
    
    private:     
        InternalPtr<Internal, InternalSize_Fence> m_pInternal;               
    };
}