#include "../interface/HAL/CommandList.hpp"
#include "../include/CommandListImpl.hpp"
#include "../include/CommandAllocatorImpl.hpp"
#include "../include/RenderPassImpl.hpp"
#include "../include/DeviceImpl.hpp"

namespace HAL {

    CommandList::Internal::Internal(CommandAllocator const& allocator) {
        auto pCmdAllocator = reinterpret_cast<const CommandAllocator::Internal*>(&allocator);
        vk::CommandBufferAllocateInfo cmdBufferAI = {
            .commandPool = pCmdAllocator->GetCommandPool(),
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        m_pCommandBuffer = std::move(pCmdAllocator->GetVkDevice().allocateCommandBuffersUnique(cmdBufferAI).at(0));
        m_pDevice = pCmdAllocator->GetDevice();
    }
    
    auto CommandList::Internal::Begin() -> void {
        m_pCommandBuffer->begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    }

    auto CommandList::Internal::End() -> void {
        m_pCommandBuffer->end();
    }

    auto CommandList::Internal::BeginRenderPass(RenderPassBeginInfo const& beginInfo) -> void {
        ((RenderPass::Internal*)(beginInfo.pRenderPass))->GenerateFrameBufferAndCommit(*m_pCommandBuffer, beginInfo);
    }

    auto CommandList::Internal::EndRenderPass() -> void {
        m_pCommandBuffer->endRenderPass();
    }

    auto CommandList::Internal::SetComputePipeline(ComputePipeline const& pipeline, ComputeState const& state) -> void {
        auto pImplDevice = reinterpret_cast<Device::Internal*>(m_pDevice);
        m_pCommandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, pImplDevice->GetPipelineCache().GetComputePipeline(pipeline, state));
    }
}

namespace HAL {

    CommandList::CommandList(CommandAllocator const& allocator) : m_pInternal(allocator) {}

    CommandList::CommandList(CommandList&& rhs) noexcept : m_pInternal(std::move(rhs.m_pInternal)) {}

    CommandList& CommandList::operator=(CommandList&& rhs) noexcept { m_pInternal = std::move(rhs.m_pInternal); return *this; }

    auto ComputeCommandList::SetComputePipeline(ComputePipeline const& pipeline, ComputeState const& state) -> void {
        m_pInternal->SetComputePipeline(pipeline, state);      
    }

    CommandList::~CommandList() = default;

    auto CommandList::Begin() -> void {
        m_pInternal->Begin();
    }

    auto CommandList::End() -> void {
        m_pInternal->End();
    }

    auto CommandList::GetVkCommandBuffer() const -> vk::CommandBuffer {
        return m_pInternal->GetVkCommandBuffer();
    }

    TransferCommandList::TransferCommandList(TransferCommandAllocator const& allocator) : CommandList(*reinterpret_cast<CommandAllocator const*>(&allocator)) {}

    ComputeCommandList::ComputeCommandList(ComputeCommandAllocator const& allocator) : TransferCommandList(*reinterpret_cast<TransferCommandAllocator const*>(&allocator)) {}

   
    GraphicsCommandList::GraphicsCommandList(GraphicsCommandAllocator const& allocator) : ComputeCommandList(*reinterpret_cast<ComputeCommandAllocator const*>(&allocator)) {}

    auto GraphicsCommandList::BeginRenderPass(RenderPassBeginInfo const& beginInfo) -> void {
        m_pInternal->BeginRenderPass(beginInfo);
    }

    auto GraphicsCommandList::EndRenderPass() -> void {
        m_pInternal->EndRenderPass();
    }
}