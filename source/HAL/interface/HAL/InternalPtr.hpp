#pragma once

#include <utility>
#include <optional>
#include <cinttypes>
#include <type_traits>
#include <vector>
#include <string>


//TODO only C-API
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
    class RenderPassCreateInfo;
    enum class Format;
    enum class DescriptorType;
    enum class ShaderStageFlagBits: uint32_t;
}

namespace vma {
    class Allocator;
}


namespace std {
    template<typename T> using observer_ptr = T*; 
}

namespace HAL {
    struct NonCopyable { 
        NonCopyable() = default;
        NonCopyable(NonCopyable const&) = delete;
        NonCopyable& operator=(NonCopyable const&) = delete;
    };
}

namespace HAL {
    template<typename T> using ArrayProxy = std::initializer_list<std::reference_wrapper<const T>>; 
}

namespace HAL {
    template<typename T, size_t Size, size_t Alignment = 8>
    class InternalPtr {
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
        explicit InternalPtr(Args&&... args) noexcept { new (GetPtr()) T(std::forward<Args>(args)...);  }
    
        InternalPtr(InternalPtr const& rhs) noexcept { new (GetPtr()) T(*rhs); }

        InternalPtr(InternalPtr&& rhs) noexcept { new (GetPtr()) T(std::move(*rhs)); }      

        ~InternalPtr() noexcept {          
            Validate<sizeof(T), alignof(T)>();
            GetPtr()->~T();
        }
           
        InternalPtr& operator=(InternalPtr const& rhs)  {
            *GetPtr() = *rhs;
            return *this;
        }
            
        InternalPtr& operator=(InternalPtr&& rhs) noexcept { 
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
    constexpr size_t InternalSize_Adapter = 2632;
    constexpr size_t InternalSize_Instance = 128;
    constexpr size_t InternalSize_Device = 184;
    constexpr size_t InternalSize_SwapChain = 360;
    constexpr size_t InternalSize_Fence = 40;
    constexpr size_t InternalSize_CommandQueue = 8;
    constexpr size_t InternalSize_CommandAllocator = 40;
    constexpr size_t InternalSize_CommandList = 56;
    constexpr size_t InternalSize_RenderPass = 144;
    constexpr size_t InternalSize_ShaderCompiler = 56;
    constexpr size_t InternalSize_Pipeline = 136;
#else
    constexpr size_t InternalSize_Adapter = 2616;
    constexpr size_t InternalSize_Instance = 104;
    constexpr size_t InternalSize_Device = 160;
    constexpr size_t InternalSize_SwapChain = 320;
    constexpr size_t InternalSize_Fence = 40;
    constexpr size_t InternalSize_Compiler = 64;
    constexpr size_t InternalSize_CommandQueue = 8;
    constexpr size_t InternalSize_CommandAllocator = 40;
    constexpr size_t InternalSize_CommandList = 56;
    constexpr size_t InternalSize_RenderPass = 120;
    constexpr size_t InternalSize_ShaderCompiler = 56;
    constexpr size_t InternalSize_Pipeline = 112;
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
    class GraphicsPipeline;
    class ComputePipeline;
    class DescriptorTable;
       
}

namespace HAL {

    enum class Format {

    };

    enum class FillMode {
        Solid,
        Line,
        Point
    };

    enum class CullMode {
        None,
        Front,
        Back,
    };

    enum class ComparisonFunction {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum class BlendFactor {

    };

    enum class BlendFunction {

    };

    enum class FrontFace {
        CounterClockwise,
        Clockwise
    };

    enum class ShaderStage {
        Vertex,
        Hull,
        Domain,
        Geometry,
        Fragment,
        Compute
    };

    struct PipelineResource {
        uint32_t                SetID;
        uint32_t                BindingID;
        uint32_t                DescriptorCount;
        vk::DescriptorType      DescriptorType;
        vk::ShaderStageFlagBits Stages;
    };

    struct RasterizationState {
        FillMode  FillMode;
        CullMode  CullMode;
        FrontFace FrontFace;
    };

    struct DepthStencilState {
        bool               DepthEnable;
        bool               DepthWrite;
        ComparisonFunction DepthFunc;
    };

    struct RenderTargetBlendState {
        bool          BlendEnable;
        BlendFactor   ColorSrcBlend;
        BlendFactor   ColorDstBlend;
        BlendFunction ColorBlendOp;
        BlendFactor   AlphaSrcBlend;
        BlendFactor   AlphaDstBlend;
        BlendFunction AlphaBlendOp;
        uint8_t       WriteMask;
    };

    struct ColorBlendState {
        RenderTargetBlendState RenderTarget[8];
    };

    struct ShaderBytecode {
        uint8_t* pData;
        uint64_t Size;
    };

    struct ComputeState {

    };

    struct GraphicsState {
        RasterizationState RasterState;
        DepthStencilState  DepthStencilState;
        ColorBlendState    BlendState;
    };

    struct ComputePipelineCreateInfo {
        ShaderBytecode CS;
    };

    struct GraphicsPipelineCreateInfo {
        ShaderBytecode VS;
        ShaderBytecode PS;
        ShaderBytecode DS;
        ShaderBytecode HS;
        ShaderBytecode GS;
    };
}
