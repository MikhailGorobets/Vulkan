#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    class CommandList: NonCopyable {
    public:
        class Internal;
    protected:
        CommandList(CommandAllocator const& allocator);
    public:       
        CommandList(CommandList&&) noexcept;
       
        CommandList& operator=(CommandList&&) noexcept;        

        ~CommandList();  
    
        auto Begin() -> void;
        
        auto End() -> void;    

        auto GetVkCommandBuffer() const -> vk::CommandBuffer;
    protected:     
        InternalPtr<Internal, InternalSize_CommandList> m_pInternal;   
    };

    class TransferCommandList: public CommandList {
    public:
        TransferCommandList(TransferCommandAllocator const& allocator);
    };

    class ComputeCommandList: public TransferCommandList {
    public:
        ComputeCommandList(ComputeCommandAllocator const& allocator);
    };

    class GraphicsCommandList: public ComputeCommandList {
    public:
        GraphicsCommandList(GraphicsCommandAllocator const& allocator);
    };
}
