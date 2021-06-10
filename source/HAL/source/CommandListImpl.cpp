#include "../include/CommandListImpl.hpp"
#include "../include/CommandAllocatorImpl.hpp"

namespace HAL {
    CommandList::Internal::Internal(CommandAllocator const& allocator) {
        auto pCmdAllocator = reinterpret_cast<const CommandAllocator::Internal*>(&allocator);
        vk::CommandBufferAllocateInfo cmdBufferAI = {
            .commandPool = pCmdAllocator->GetCommandPool(),
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        m_pCommandBuffer = std::move(pCmdAllocator->GetDevice().allocateCommandBuffersUnique(cmdBufferAI).at(0));      
    }

    auto CommandList::Internal::End() -> void {
        m_pCommandBuffer->end();
    }

    auto CommandList::Internal::Begin() -> void {
        m_pCommandBuffer->begin(vk::CommandBufferBeginInfo { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    }
}

namespace HAL {
    CommandList::CommandList(CommandAllocator const& allocator) : m_pInternal(allocator) {}

    CommandList::CommandList(CommandList&& rhs) noexcept : m_pInternal(std::move(rhs.m_pInternal)) {}

    CommandList& CommandList::operator=(CommandList&& rhs) noexcept { m_pInternal = std::move(rhs.m_pInternal); return *this; }

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

    TransferCommandList::TransferCommandList(TransferCommandAllocator const & allocator) : CommandList(*reinterpret_cast<CommandAllocator const*>(&allocator)) {}

    ComputeCommandList::ComputeCommandList(ComputeCommandAllocator const & allocator) : TransferCommandList(*reinterpret_cast<TransferCommandAllocator const*>(&allocator)) {}

    GraphicsCommandList::GraphicsCommandList(GraphicsCommandAllocator const& allocator) : ComputeCommandList(*reinterpret_cast<ComputeCommandAllocator const*>(&allocator)) {}
}