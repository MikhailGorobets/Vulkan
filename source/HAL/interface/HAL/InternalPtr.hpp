#pragma once

#include <utility>
#include <optional>
#include <cinttypes>
#include <type_traits>


namespace HAL {
    template<typename T, size_t Size, size_t Alignment = 8>
    class Internal_Ptr {
    private:
        std::aligned_storage_t<Size, Alignment> m_Data;
    private:
       auto GetPtr() noexcept -> T* { return reinterpret_cast<T*>(&m_Data); }
       auto GetPtr() const noexcept -> const T* { return reinterpret_cast<const T*>(&m_Data); }
    
       template<size_t ActualSize, size_t ActualAlignment> 
       static auto Validate() noexcept -> void {
           static_assert(Size == ActualSize, "Size and sizeof(T) mismatch");
           static_assert(Alignment == ActualAlignment, "Alignment and alignof(T) mismatch");
       }
    public:
        template<typename... Args>
        explicit Internal_Ptr(Args&&... args) {  new (GetPtr()) T(std::forward<Args>(args)...);  };
       
        ~Internal_Ptr() noexcept {
            Validate<sizeof(T), alignof(T)>();
            GetPtr()->~T();
        }
           
        Internal_Ptr& operator=(Internal_Ptr&& rhs) noexcept { 
            *GetPtr() = std::move(*rhs);
            return *this;
        }
        
        T* operator->() noexcept { return GetPtr(); }
        const T* operator->() const noexcept { return GetPtr(); }
        
        T& operator*() noexcept { return *GetPtr(); }
        const T& operator*() const noexcept { return *GetPtr(); };
    };
}

namespace HAL {
    constexpr size_t InternalSize_Adapter   = 64;
    constexpr size_t InternalSize_Instance  = 64; 
    constexpr size_t InternalSize_Device    = 64;
    constexpr size_t InternalSize_SwapChain = 64;
    constexpr size_t InternalSize_Fence     = 64;
    constexpr size_t InternalSize_Compiler  = 64;
    constexpr size_t InternalSize_CommandQueue = 64;
    constexpr size_t InternalSize_CommandAllocator = 64;
    constexpr size_t InternalSize_CommandList = 64;
}


namespace HAL {
    class Instance;
    class Adapter;
    class Device;
    class SwapChain;
    class Fence;

    class CommandQueue;
    class TransferCommandQueue;
    class ComputeCommandQueue;
    class GraphicsCommandQueue;

    class CommandAllocator;
    class TransferCommandAllocator;
    class ComputeCommandAllocator;
    class GraphicsCommandAllocator;
    
    class CommandList;
    class TransferCommandList;
    class ComputeCommandList;
    class GraphicsCommandList;

    class Compiler;
}

namespace vk {
    class Instance;
    class PhysicalDevice;
    class Device;
    class SwapChainKHR;
    class Queue;
    class CommandPool;
    class CommandBuffer;   
}
