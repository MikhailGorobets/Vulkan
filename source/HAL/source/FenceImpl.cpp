
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
        m_ExpectedValue = value;
    }

    auto Fence::Internal::Signal(uint64_t value) const -> void {
        vk::SemaphoreSignalInfo signalInfo = {
             .semaphore = *m_pSemaphore,
             .value = value
        };
        m_pSemaphore.getOwner().signalSemaphore(signalInfo);
    }

    auto Fence::Internal::Wait(uint64_t value) const -> void {
        vk::Semaphore pSemaphores[] = {m_pSemaphore.get()};
        vk::SemaphoreWaitInfo waitInfo = {
             .semaphoreCount = _countof(pSemaphores),
             .pSemaphores = pSemaphores,
             .pValues = &value,
        };
        auto result = m_pSemaphore.getOwner().waitSemaphores(waitInfo, std::numeric_limits<uint64_t>::max());
        assert(result == vk::Result::eSuccess);
    }

    auto Fence::Internal::GetCompletedValue() const -> uint64_t {
        return m_pSemaphore.getOwner().getSemaphoreCounterValue(*m_pSemaphore);
    }

    auto Fence::Internal::GetExpectedValue() const -> uint64_t {
        return m_ExpectedValue;
    }

    auto Fence::Internal::Increment() -> uint64_t {
        return ++m_ExpectedValue;
    }

    auto Fence::Internal::IsCompleted() const -> bool {
        return this->GetCompletedValue() >= m_ExpectedValue;
    }

    auto Fence::Internal::GetVkSemaphore() const -> vk::Semaphore {
        return *m_pSemaphore;
    }
}

namespace HAL {
    Fence::Fence(Device const& device, std::optional<uint64_t> value) : m_pInternal(device, value.value_or(0)) {}

    Fence::Fence(Fence&& rhs) : m_pInternal(std::move(rhs.m_pInternal)) {}

    Fence& Fence::operator=(Fence&& rhs) { m_pInternal = std::move(rhs.m_pInternal); return *this; }

    Fence::~Fence() = default;

    auto Fence::Signal(uint64_t value) const -> void { m_pInternal->Signal(value); }

    auto Fence::Wait(uint64_t value) const -> void { m_pInternal->Wait(value); }

    auto Fence::GetCompletedValue() const -> uint64_t { return m_pInternal->GetCompletedValue(); }

    auto Fence::GetExpectedValue() const -> uint64_t {
        return m_pInternal->GetExpectedValue();
    }

    auto Fence::Increment() -> uint64_t {
        return m_pInternal->Increment();
    }

    auto Fence::IsCompleted() const -> bool {
        return m_pInternal->IsCompleted();
    }

    auto Fence::GetVkSemaphore() const -> vk::Semaphore {
        return m_pInternal->GetVkSemaphore();
    }
}