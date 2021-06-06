#include "..\include\CommandQueueImpl.hpp"
#include "..\include\FenceImpl.hpp"

namespace HAL {
    CommandQueue::Internal::Internal(vk::Queue const& queue) : m_Queue(queue) {}

    auto CommandQueue::Internal::Signal(Fence const& fence, std::optional<uint64_t> value) const -> void {
      
       vk::Semaphore pSignalSemaphores[] = { fence.GetVkSemaphore() };
       uint64_t pSignalSemaphoreValues[] = { value.value_or(fence.GetExpectedValue()) }; 

       vk::TimelineSemaphoreSubmitInfo timelineInfo = {
           .signalSemaphoreValueCount = 1,
           .pSignalSemaphoreValues = pSignalSemaphoreValues
       };

       vk::SubmitInfo submitInfo = {
           .pNext = &timelineInfo,
           .signalSemaphoreCount  = 1,
           .pSignalSemaphores = pSignalSemaphores
       };

      // std::lock_guard<std::mutex> lock(m_Mutex);
       m_Queue.submit({submitInfo}, {});
    }

    auto CommandQueue::Internal::Wait(Fence const& fence, std::optional<uint64_t> value) const -> void {

        vk::Semaphore pWaitSemaphores[] = { fence.GetVkSemaphore() };
        uint64_t pWaitSemaphoreValues[] = { value.value_or(fence.GetExpectedValue()) };
        vk::PipelineStageFlags pWaitStages[] = { vk::PipelineStageFlagBits::eBottomOfPipe };

        vk::TimelineSemaphoreSubmitInfo timelineInfo = {
            .waitSemaphoreValueCount = 1,
            .pWaitSemaphoreValues = pWaitSemaphoreValues
        };
        vk::SubmitInfo submitInfo = {
            .pNext = &timelineInfo,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = pWaitSemaphores,
            .pWaitDstStageMask = pWaitStages
        };
        
       // std::lock_guard<std::mutex> lock(m_Mutex);
        m_Queue.submit({ submitInfo }, {});    
    }

    auto CommandQueue::Internal::WaitIdle() const -> void {
      //  std::lock_guard<std::mutex> lock(m_Mutex);
        m_Queue.waitIdle();        
    }

    auto CommandQueue::Internal::GetVkQueue() const -> vk::Queue {
        return m_Queue;
    }   
}

namespace HAL {
  

    auto CommandQueue::Signal(Fence const& fence, std::optional<uint64_t> value) const -> void {
        m_pInternal->Signal(fence, value);
    }

    auto CommandQueue::Wait(Fence const& fence, std::optional<uint64_t> value) const -> void {
        m_pInternal->Wait(fence, value);
    }

    auto CommandQueue::WaitIdle() const -> void {
        m_pInternal->WaitIdle();
    }

    auto CommandQueue::GetVkQueue() const -> vk::Queue {
        return m_pInternal->GetVkQueue();
    }

    auto GraphicsCommandQueue:: ExecuteCommandList(GraphicsCommandList* pCmdLists, uint32_t count) const -> void {

    }

}