#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    class CommandList {
    public:
        class Internal;
    protected:
         CommandList(CommandAllocator const& allocator);
    public:       
        ~CommandList();  
    
        auto Begin() const -> void;
        
        auto End() const -> void;    

        auto GetVkCommandBuffer() const -> vk::CommandBuffer;
    private:     
        Internal_Ptr<Internal, InternalSize_CommandList> m_pInternal;   
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
