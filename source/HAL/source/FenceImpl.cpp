
#include "../include/FenceImpl.hpp"
#include "../include/DeviceImpl.hpp"

namespace HAL {

    Fence::Internal::Internal(Device const& device, uint64_t value) {
        vk::SemaphoreTypeCreateInfo semaphoreTypeCI = {
            .semaphoreType = vk::SemaphoreType::eTimeline,
            .initialValue = value
        };      
    
        vk::SemaphoreCreateInfo semaphoreCI = {
            .pNext = &semaphoreTypeCI
        };

        m_pSemaphore = device.GetVkDevice().createSemaphoreUnique(semaphoreCI);
    }

    auto Fence::Internal::Signal(uint64_t value) const {
        vk::SemaphoreSignalInfo signalInfo = {
             .semaphore = *m_pSemaphore,
             .value = value
        };
        m_pSemaphore.getOwner().signalSemaphore(signalInfo);        
    }

    auto Fence::Internal::Wait(uint64_t value) const {
        vk::SemaphoreWaitInfo waitInfo = {
             .semaphoreCount = 1,
             .pSemaphores = m_pSemaphore.getAddressOf(),
             .pValues = &value,            
        };
        m_pSemaphore.getOwner().waitSemaphores(waitInfo,  std::numeric_limits<uint64_t>::max());
    }

    auto Fence::Internal::GetCompletedValue() const -> uint64_t {
        return m_pSemaphore.getOwner().getSemaphoreCounterValue(*m_pSemaphore);
    }

    auto Fence::Internal::GetVkSemaphore() const -> vk::Semaphore {
        return *m_pSemaphore;
    }

}

namespace HAL {

    Fence::Fence(Device const& device, uint64_t value) : m_pInternal(device, value) {}

    auto Fence::Signal(uint64_t value) const { m_pInternal->Signal(value); }

    auto Fence::Wait(uint64_t value) const { m_pInternal->Wait(value); }

    auto Fence::GetCompletedValue() const -> uint64_t { return m_pInternal->GetCompletedValue(); }

    auto Fence::GetVkSemaphore() const -> vk::Semaphore {
        return m_pInternal->GetVkSemaphore();
    }

}