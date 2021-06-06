#pragma once

#include <HAL/CommandQueue.hpp>
#include <HAL/Fence.hpp>
#include <vulkan/vulkan_decl.h>
#include <mutex>

namespace HAL {
    class CommandQueue::Internal {
    public:
        Internal() = default;        

        Internal(vk::Queue const& queue);      
    
        auto Signal(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;

        auto Wait(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;
        
        auto WaitIdle() const -> void;
  
        auto GetVkQueue() const -> vk::Queue;
    private:
      //  mutable std::mutex m_Mutex = {};
        vk::Queue          m_Queue = {};   
    };
}