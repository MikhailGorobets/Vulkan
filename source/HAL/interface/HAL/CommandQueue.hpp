#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    class CommandQueue {
    public:
        class Internal;
    public:     
        auto Signal(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;

        auto Wait(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;

        auto NextImage(SwapChain const& swapChain, Fence const& fence, std::optional<uint64_t> signalValue = std::nullopt) const -> uint32_t;

        auto Present(SwapChain const& swapChain, uint32_t frameID, Fence const& fence, std::optional<uint64_t> waitValue = std::nullopt) const -> void;
       
        auto WaitIdle() const -> void;
  
        auto GetVkQueue() const -> vk::Queue;      

    private:     
        Internal_Ptr<Internal, InternalSize_CommandQueue> m_pInternal;   
    };
    
    class TransferCommandQueue: public CommandQueue {
    public:
        auto ExecuteCommandList(TransferCommandList* pCmdLists, uint32_t count) const -> void;
    };
    
    class ComputeCommandQueue: public TransferCommandQueue {
    public:
        auto ExecuteCommandList(ComputeCommandList* pCmdLists, uint32_t count) const -> void;
    };

    class GraphicsCommandQueue: public ComputeCommandQueue {
    public:
        auto ExecuteCommandLists(GraphicsCommandList* pCmdLists, uint32_t count) const -> void;
    };
}
