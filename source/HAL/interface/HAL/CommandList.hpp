#pragma once

#include <HAL/InternalPtr.hpp>
#include <HAL/RenderPass.hpp>

namespace HAL {

    struct RenderPassBeginInfo {
        const RenderPass*               pRenderPass = {};
        const RenderPassAttachmentInfo* pAttachments = {};
        uint32_t                        AttachmentCount = {};   
    };
    
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

        auto BeginRenderPass(RenderPassBeginInfo const& beginInfo) -> void;

        auto EndRenderPass() -> void;
    };
}
