#pragma once

#include <utility>
#include <optional>
#include <cinttypes>
#include <type_traits>
#include <vector>
#include <string>


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
        explicit Internal_Ptr(Args&&... args) noexcept { new (GetPtr()) T(std::forward<Args>(args)...);  }
    
        Internal_Ptr(Internal_Ptr const& rhs) noexcept { new (GetPtr()) T(*rhs); }

        Internal_Ptr(Internal_Ptr&& rhs) noexcept { new (GetPtr()) T(std::move(*rhs)); }      

        ~Internal_Ptr() noexcept {
            Validate<sizeof(T), alignof(T)>();
            GetPtr()->~T();
        }
           
        Internal_Ptr& operator=(Internal_Ptr const& rhs)  {
            *GetPtr() = *rhs;
            return *this;
        }
            
        Internal_Ptr& operator=(Internal_Ptr&& rhs) noexcept { 
            *GetPtr() = std::move(*rhs);
            return *this;
        }
        
        T* operator->() noexcept { return GetPtr(); }

        const T* operator->() const noexcept { return GetPtr(); }
        
        T& operator*() noexcept { return *GetPtr(); }

        const T& operator*() const noexcept { return *GetPtr(); }
    };
}

namespace HAL {


#ifdef _DEBUG

    constexpr size_t InternalSize_Adapter   = 2496;
    constexpr size_t InternalSize_Instance  = 128; 
    constexpr size_t InternalSize_Device    = 200;
    constexpr size_t InternalSize_SwapChain = 376;
    constexpr size_t InternalSize_Fence     = 40;
    constexpr size_t InternalSize_Compiler  = 64;
    constexpr size_t InternalSize_CommandQueue = 8;
    constexpr size_t InternalSize_CommandAllocator = 32;
    constexpr size_t InternalSize_CommandList = 64;
#else
    constexpr size_t InternalSize_Adapter   = 2480;
    constexpr size_t InternalSize_Instance  = 104; 
    constexpr size_t InternalSize_Device    = 176;
    constexpr size_t InternalSize_SwapChain = 336;
    constexpr size_t InternalSize_Fence     = 40;
    constexpr size_t InternalSize_Compiler  = 64;
    constexpr size_t InternalSize_CommandQueue = 8;
    constexpr size_t InternalSize_CommandAllocator = 32;
    constexpr size_t InternalSize_CommandList = 64;
#endif

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
    class ShaderCompiler;
   
}

namespace vk {
    class Instance;
    class PhysicalDevice;
    class Device;
    class SwapchainKHR;
    class Queue;
    class CommandPool;
    class CommandBuffer;   
    class PipelineCache;
    class Semaphore;
    class ImageView;
    enum class Format;
}
