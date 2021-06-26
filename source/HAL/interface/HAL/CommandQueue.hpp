#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    class CommandQueue: NonCopyable {
    public:
        class Internal;
    public:     
        CommandQueue(CommandQueue&&) noexcept;
       
        CommandQueue& operator=(CommandQueue&&) noexcept;        

        auto Signal(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;

        auto Wait(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;

        auto NextImage(SwapChain const& swapChain, Fence const& fence, std::optional<uint64_t> signalValue = std::nullopt) const -> uint32_t;

        auto Present(SwapChain const& swapChain, uint32_t frameID, Fence const& fence, std::optional<uint64_t> waitValue = std::nullopt) const -> void;
       
        auto WaitIdle() const -> void;
  
        auto GetVkQueue() const -> vk::Queue;      

    protected:     
        InternalPtr<Internal, InternalSize_CommandQueue> m_pInternal;   
    };
    
    class TransferCommandQueue: public CommandQueue {
    public:
        auto ExecuteCommandList(ArraySpan<TransferCommandList> cmdLists) const -> void;
    
        auto ExecuteCommandList(ArrayView<TransferCommandList> cmdLists) const -> void;
    };
    
    class ComputeCommandQueue: public TransferCommandQueue {
    public:
        auto ExecuteCommandList(ArraySpan<ComputeCommandList> cmdLists) const -> void;
    
        auto ExecuteCommandList(ArrayView<ComputeCommandList> cmdLists) const -> void;
    };

    class GraphicsCommandQueue: public ComputeCommandQueue {
    public:
        auto ExecuteCommandLists(ArraySpan<GraphicsCommandList> cmdLists) const -> void;

        auto ExecuteCommandLists(ArrayView<GraphicsCommandList> cmdLists) const -> void;
    };
}
