#include "..\interface\HAL\CommandQueue.hpp"
#include "..\interface\HAL\CommandQueue.hpp"
#include "..\include\CommandQueueImpl.hpp"
#include "..\include\SwapChainImpl.hpp"
#include "..\include\FenceImpl.hpp"

//TODO Lock

namespace HAL {
    CommandQueue::Internal::Internal(vk::Queue const& queue) : m_Queue(queue) {}

    auto CommandQueue::Internal::Signal(Fence const& fence, std::optional<uint64_t> value) const -> void { 
       vk::Semaphore signalSemaphores[] = { fence.GetVkSemaphore() };
       uint64_t signalSemaphoreValues[] = { value.value_or(fence.GetExpectedValue()) }; 

       vk::TimelineSemaphoreSubmitInfo timelineInfo = {
           .signalSemaphoreValueCount = _countof(signalSemaphoreValues),
           .pSignalSemaphoreValues = signalSemaphoreValues
       };

       vk::SubmitInfo submitInfo = {
           .pNext = &timelineInfo,
           .signalSemaphoreCount  = _countof(signalSemaphores),
           .pSignalSemaphores = signalSemaphores
       };

       m_Queue.submit({submitInfo}, {});
    }

    auto CommandQueue::Internal::Wait(Fence const& fence, std::optional<uint64_t> value) const -> void {
        vk::Semaphore pWaitSemaphores[] = { fence.GetVkSemaphore() };
        uint64_t pWaitSemaphoreValues[] = { value.value_or(fence.GetExpectedValue()) };
        vk::PipelineStageFlags pWaitStages[] = { vk::PipelineStageFlagBits::eAllCommands };

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
        
        m_Queue.submit({ submitInfo }, {});    
    }

    auto CommandQueue::Internal::NextImage(SwapChain const& swapChain, Fence const& fence, std::optional<uint64_t> value) const -> uint32_t {
        auto pSwapChain = (SwapChain::Internal*)(&swapChain);           
        pSwapChain->Acquire();

        uint32_t frameID = pSwapChain->GetDevice().acquireNextImageKHR(pSwapChain->GetSwapChain(), std::numeric_limits<uint64_t>::max(), pSwapChain->GetSemaphoreAvailable(), nullptr);
           
        vk::Semaphore waitSemaphores[] = { pSwapChain->GetSemaphoreAvailable() };       
        vk::Semaphore signalSemaphores[] = { fence.GetVkSemaphore() };

        uint64_t waitSemaphoreValues[] = { 0 };
        uint64_t signalSemaphoreValues[] = { value.value_or(fence.GetExpectedValue()) }; 
      
        vk::PipelineStageFlags stageMask[] = { vk::PipelineStageFlagBits::eTransfer };

        vk::TimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo = {
        //    .waitSemaphoreValueCount = _countof(waitSemaphoreValues),
        //    .pWaitSemaphoreValues = waitSemaphoreValues,
            .signalSemaphoreValueCount = _countof(signalSemaphoreValues),
            .pSignalSemaphoreValues = signalSemaphoreValues
        };

        vk::SubmitInfo submitInfo = {
            .pNext = &timelineSemaphoreSubmitInfo,
            .waitSemaphoreCount = _countof(waitSemaphores),
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = stageMask,
            .signalSemaphoreCount = _countof(signalSemaphores),
            .pSignalSemaphores = signalSemaphores         
        };        
        m_Queue.submit(submitInfo, {});  
        return frameID;
    }

    auto CommandQueue::Internal::Present(SwapChain const& swapChain, uint32_t frameID, Fence const& fence, std::optional<uint64_t> value) const -> void {
        auto pSwapChain = (SwapChain::Internal*)(&swapChain);

        uint64_t signalSemaphoreValues[] = { 0 };
        uint64_t waitSemaphoreValues[] = { value.value_or(fence.GetExpectedValue()) };

        vk::Semaphore signalSemaphores[] = { pSwapChain->GetSemaphoreFinished() }; 
        vk::Semaphore waitSemahores[] = { fence.GetVkSemaphore() };
        
        vk::PipelineStageFlags stageMask[] = { vk::PipelineStageFlagBits::eTransfer };
        
        vk::TimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo = {
            .waitSemaphoreValueCount = _countof(waitSemaphoreValues),
            .pWaitSemaphoreValues = waitSemaphoreValues,      
       //     .signalSemaphoreValueCount = _countof(signalSemaphoreValues),
        //    .pSignalSemaphoreValues = signalSemaphoreValues
        };

        vk::SubmitInfo submitInfo = {
            .pNext = &timelineSemaphoreSubmitInfo,
            .waitSemaphoreCount =  _countof(waitSemahores),
            .pWaitSemaphores = waitSemahores,
            .pWaitDstStageMask = stageMask,
            .signalSemaphoreCount = _countof(signalSemaphores),
            .pSignalSemaphores = signalSemaphores,           
        };            

        m_Queue.submit(submitInfo, {});
    
        vk::SwapchainKHR swapChains[] = { pSwapChain->GetSwapChain() };
        
        vk::PresentInfoKHR presentInfo = {
            .waitSemaphoreCount = _countof(signalSemaphores),
            .pWaitSemaphores = signalSemaphores,
            .swapchainCount = _countof(swapChains),
            .pSwapchains = swapChains,
            .pImageIndices = &frameID,
        };

        vk::Result result = m_Queue.presentKHR(presentInfo);
        assert(result == vk::Result::eSuccess);
        pSwapChain->Release();
    }


    auto CommandQueue::Internal::WaitIdle() const -> void {
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

    auto CommandQueue::NextImage(SwapChain const& swapChain, Fence const& fence, std::optional<uint64_t> signalValue) const -> uint32_t {
        return m_pInternal->NextImage(swapChain, fence, signalValue);
    }

    auto CommandQueue::Present(SwapChain const& swapChain, uint32_t frameID, Fence const& fence, std::optional<uint64_t> waitValue) const -> void {
        return m_pInternal->Present(swapChain, frameID, fence, waitValue);
    }

    auto CommandQueue::WaitIdle() const -> void {
        m_pInternal->WaitIdle();
    }

    auto CommandQueue::GetVkQueue() const -> vk::Queue {
        return m_pInternal->GetVkQueue();
    }

    auto GraphicsCommandQueue::ExecuteCommandList(GraphicsCommandList* pCmdLists, uint32_t count) const -> void {

    }

}