#pragma once

#include <HAL/CommandQueue.hpp>
#include <HAL/SwapChain.hpp>
#include <HAL/Fence.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {
    class CommandQueue::Internal {
    public:
        Internal(vk::Queue queue);      

        auto Signal(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;

        auto Wait(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;
        
        auto NextImage(SwapChain const& swapChain, Fence const& fence, std::optional<uint64_t> signalValue = std::nullopt) const -> uint32_t;

        auto Present(SwapChain const& swapChain, uint32_t frameID, Fence const& fence, std::optional<uint64_t> waitValue = std::nullopt) const -> void;

        auto WaitIdle() const -> void;

        template<typename T> 
        auto ExecuteCommandList(ArrayProxy<T> const& cmdLists) const -> void;

        auto GetVkQueue() const -> vk::Queue;       
    private:
        vk::Queue m_Queue = {};          
    };
}

namespace HAL {
    template<typename T> 
    inline auto CommandQueue::Internal::ExecuteCommandList(ArrayProxy<T> const& cmdLists) const -> void {
        static_assert(std::is_base_of<CommandList, T>::value);
        std::vector<vk::CommandBuffer> commandBuffers;
        commandBuffers.reserve(std::size(cmdLists));
        for(auto const& e: cmdLists) 
            commandBuffers.push_back(e.get().GetVkCommandBuffer());
        
        vk::SubmitInfo submitInfo = {
           .commandBufferCount = static_cast<uint32_t>(std::size(commandBuffers)),
           .pCommandBuffers = std::data(commandBuffers),             
        };            
        m_Queue.submit(submitInfo, {});      
    }
}
