#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    class CommandList {
    public:
        class Internal;
    protected:
        CommandList(CommandAllocator& allocator);

        auto Begin() -> void;
        
        auto End() -> void;    

        auto GetVkCommandBuffer() const -> vk::CommandBuffer;
    private:     
        Internal_Ptr<Internal, InternalSize_CommandList> m_pInternal;   
    };

    class TransferCommandList: public CommandList {
    public:
        TransferCommandList(CommandAllocator& allocator);
    };

    class ComputeCommandList: public TransferCommandList {
    public:
        ComputeCommandList(CommandAllocator& allocator);
    };

    class GraphicsCommandList: public ComputeCommandList {
    public:
        GraphicsCommandList(CommandAllocator& allocator);
    };
}
