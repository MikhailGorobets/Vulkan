#ifndef AMD_VULKAN_MEMORY_ALLOCATOR_HPP
#define AMD_VULKAN_MEMORY_ALLOCATOR_HPP

#include "vk_mem_alloc.h"
#include "vulkan.hpp"

#if !defined(VMA_HPP_NAMESPACE)
#define VMA_HPP_NAMESPACE vma
#endif

#define VMA_HPP_NAMESPACE_STRING VULKAN_HPP_STRINGIFY(VMA_HPP_NAMESPACE)

namespace VMA_HPP_NAMESPACE {
  class Allocation;
  class Allocator;
  class DefragmentationContext;
  class Pool;
  class NoParent;

  struct AllocationCreateInfo;
  struct AllocationInfo;
  struct AllocatorCreateInfo;
  struct DefragmentationInfo2;
  struct DefragmentationStats;
  struct DeviceMemoryCallbacks;
  struct PoolCreateInfo;
  struct PoolStats;
  struct RecordSettings;
  struct StatInfo;
  struct Stats;
  struct Budget;
  struct VulkanFunctions;
 

  template <typename OwnerType>
  class ObjectDestroy {
  public:
  	ObjectDestroy() = default;
  
  	ObjectDestroy(OwnerType owner) VULKAN_HPP_NOEXCEPT : m_Owner(owner) {}
  
    VULKAN_HPP_NODISCARD OwnerType getOwner() const VULKAN_HPP_NOEXCEPT { return m_Owner; }
  protected:
  	template <typename T>
  	void destroy(T t) VULKAN_HPP_NOEXCEPT {
        VULKAN_HPP_ASSERT(m_Owner);
  		m_Owner.destroy(t);
  	}
  private:
  	OwnerType m_Owner = {};
  };

  template <typename OwnerType>
  class ObjectFree {
  public:
      ObjectFree() = default;

      ObjectFree(OwnerType owner) VULKAN_HPP_NOEXCEPT : m_Owner(owner) {}
      
      VULKAN_HPP_NODISCARD OwnerType getOwner() const VULKAN_HPP_NOEXCEPT { return m_Owner; }
  protected:
      template <typename T>
      void destroy(T t) VULKAN_HPP_NOEXCEPT {
          VULKAN_HPP_ASSERT(m_Owner);
          m_Owner.free(t);
      }
  private:
      OwnerType  m_Owner = {};
  };

  template<>
  class ObjectDestroy<NoParent> {
  public:
  	ObjectDestroy() = default;
  protected:
  	template <typename T>
  	void destroy(T t) noexcept {
  		t.destroy();
  	}
  };

  template<typename T>
  class UniqueHandleTraits                         { public: using Deleter = ObjectDestroy<T>; };
  template<>
  class UniqueHandleTraits<Allocator>              { public: using Deleter = ObjectDestroy<NoParent>; };
  template<>
  class UniqueHandleTraits<Allocation>             { public: using Deleter = ObjectFree<Allocator>; };
  template<>
  class UniqueHandleTraits<Pool>                   { public: using Deleter = ObjectDestroy<Allocator>; };




  template <typename Type>
  class UniqueHandle : public UniqueHandleTraits<Type>::Deleter {
  private:
  	using Deleter = typename UniqueHandleTraits<Type>::Deleter;
  public:
	UniqueHandle()
		: Deleter()
		, m_Value() {}

	UniqueHandle(Type const& value, Deleter const& deleter = Deleter()) noexcept
		: Deleter(deleter)
		, m_Value(value) {}

	UniqueHandle(UniqueHandle const&) = delete;

	UniqueHandle(UniqueHandle&& other) noexcept
		: Deleter(std::move(static_cast<Deleter&>(other)))
		, m_Value(other.release()) {}

	~UniqueHandle() noexcept {
		if (m_Value)
			this->destroy(m_Value);
	}

	UniqueHandle& operator=(UniqueHandle const&) = delete;

	UniqueHandle& operator=(UniqueHandle&& other) noexcept {
		reset(other.release());
		*static_cast<Deleter*>(this) = std::move(static_cast<Deleter&>(other));
		return *this;
	}

	explicit operator bool() const noexcept {
		return m_Value.operator bool();
	}

	Type const* getAddressOf() const noexcept {
		return &m_Value;
	}

	Type* getAddressOf() noexcept {
		return &m_Value;
	}

	Type const* operator->() const noexcept {
		return &m_Value;
	}

	Type* operator->() noexcept {
		return &m_Value;
	}

	Type const& operator*() const noexcept {
		return m_Value;
	}

	Type& operator*() noexcept {
		return m_Value;
	}

	const Type& get() const noexcept {
		return m_Value;
	}

	Type& get() noexcept {
		return m_Value;
	}

	void reset(Type const& value = Type()) noexcept {
		if (m_Value != value) {
			if (m_Value)
				this->destroy(m_Value);
			m_Value = value;
		}
	}

	Type release() noexcept {
		Type value = m_Value;
		m_Value = nullptr;
		return value;
	}

	void swap(UniqueHandle<Type>& rhs) noexcept {
		std::swap(m_Value, rhs.m_value);
		std::swap(static_cast<Deleter&>(*this), static_cast<Deleter&>(rhs));
	}
	private:
		Type    m_Value;
   };

  using UniqueAllocator              = UniqueHandle<Allocator>;
  using UniqueAllocation             = UniqueHandle<Allocation>;
  using UniquePool                   = UniqueHandle<Pool>;
 

  enum class MemoryUsage
  {
    eUnknown = VMA_MEMORY_USAGE_UNKNOWN,
    eGpuOnly = VMA_MEMORY_USAGE_GPU_ONLY,
    eCpuOnly = VMA_MEMORY_USAGE_CPU_ONLY,
    eCpuToGpu = VMA_MEMORY_USAGE_CPU_TO_GPU,
    eGpuToCpu = VMA_MEMORY_USAGE_GPU_TO_CPU,
    eCpuCopy = VMA_MEMORY_USAGE_CPU_COPY,
    eGpuLazilyAllocated = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED
  };

  VULKAN_HPP_INLINE std::string to_string( MemoryUsage value )
  {
    switch ( value )
    {
      case MemoryUsage::eUnknown : return "Unknown";
      case MemoryUsage::eGpuOnly : return "GpuOnly";
      case MemoryUsage::eCpuOnly : return "CpuOnly";
      case MemoryUsage::eCpuToGpu : return "CpuToGpu";
      case MemoryUsage::eGpuToCpu : return "GpuToCpu";
      case MemoryUsage::eCpuCopy : return "CpuCopy";
      case MemoryUsage::eGpuLazilyAllocated : return "GpuLazilyAllocated";
      default: return "invalid";
    }
  }

  enum class AllocationCreateFlagBits : VmaAllocationCreateFlags
  {
    eDedicatedMemory = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    eNeverAllocate = VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT,
    eMapped = VMA_ALLOCATION_CREATE_MAPPED_BIT,
    eCanBecomeLost = VMA_ALLOCATION_CREATE_CAN_BECOME_LOST_BIT,
    eCanMakeOtherLost = VMA_ALLOCATION_CREATE_CAN_MAKE_OTHER_LOST_BIT,
    eUserDataCopyString = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT,
    eUpperAddress = VMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT,
    eDontBind = VMA_ALLOCATION_CREATE_DONT_BIND_BIT,
    eWithinBudget = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
    eStrategyBestFit = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT,
    eStrategyWorstFit = VMA_ALLOCATION_CREATE_STRATEGY_WORST_FIT_BIT,
    eStrategyFirstFit = VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT,
    eStrategyMinMemory = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT,
    eStrategyMinTime = VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT,
    eStrategyMinFragmentation = VMA_ALLOCATION_CREATE_STRATEGY_MIN_FRAGMENTATION_BIT,
    eStrategyMask = VMA_ALLOCATION_CREATE_STRATEGY_MASK
  };

  VULKAN_HPP_INLINE std::string to_string( AllocationCreateFlagBits value )
  {
    switch ( value )
    {
      case AllocationCreateFlagBits::eDedicatedMemory : return "DedicatedMemory";
      case AllocationCreateFlagBits::eNeverAllocate : return "NeverAllocate";
      case AllocationCreateFlagBits::eMapped : return "Mapped";
      case AllocationCreateFlagBits::eCanBecomeLost : return "CanBecomeLost";
      case AllocationCreateFlagBits::eCanMakeOtherLost : return "CanMakeOtherLost";
      case AllocationCreateFlagBits::eUserDataCopyString : return "UserDataCopyString";
      case AllocationCreateFlagBits::eUpperAddress : return "UpperAddress";
      case AllocationCreateFlagBits::eDontBind : return "DontBind";
      case AllocationCreateFlagBits::eWithinBudget : return "WithinBudget";
      case AllocationCreateFlagBits::eStrategyBestFit : return "StrategyBestFit";
      case AllocationCreateFlagBits::eStrategyWorstFit : return "StrategyWorstFit";
      case AllocationCreateFlagBits::eStrategyFirstFit : return "StrategyFirstFit";
      default: return "invalid";
    }
  }

  using AllocationCreateFlags = VULKAN_HPP_NAMESPACE::Flags<AllocationCreateFlagBits>;

  VULKAN_HPP_INLINE AllocationCreateFlags operator|( AllocationCreateFlagBits bit0, AllocationCreateFlagBits bit1 )
  {
    return AllocationCreateFlags( bit0 ) | bit1;
  }

  VULKAN_HPP_INLINE AllocationCreateFlags operator~( AllocationCreateFlagBits bits )
  {
    return ~( AllocationCreateFlags( bits ) );
  }

  VULKAN_HPP_INLINE std::string to_string( AllocationCreateFlags value  )
  {
    if ( !value ) return "{}";
    std::string result;

    if ( value & AllocationCreateFlagBits::eDedicatedMemory ) result += "DedicatedMemory | ";
    if ( value & AllocationCreateFlagBits::eNeverAllocate ) result += "NeverAllocate | ";
    if ( value & AllocationCreateFlagBits::eMapped ) result += "Mapped | ";
    if ( value & AllocationCreateFlagBits::eCanBecomeLost ) result += "CanBecomeLost | ";
    if ( value & AllocationCreateFlagBits::eCanMakeOtherLost ) result += "CanMakeOtherLost | ";
    if ( value & AllocationCreateFlagBits::eUserDataCopyString ) result += "UserDataCopyString | ";
    if ( value & AllocationCreateFlagBits::eUpperAddress ) result += "UpperAddress | ";
    if ( value & AllocationCreateFlagBits::eDontBind ) result += "DontBind | ";
    if ( value & AllocationCreateFlagBits::eWithinBudget ) result += "WithinBudget | ";
    if ( value & AllocationCreateFlagBits::eStrategyBestFit ) result += "StrategyBestFit | ";
    if ( value & AllocationCreateFlagBits::eStrategyWorstFit ) result += "StrategyWorstFit | ";
    if ( value & AllocationCreateFlagBits::eStrategyFirstFit ) result += "StrategyFirstFit | ";
    if ( value & AllocationCreateFlagBits::eStrategyMinMemory ) result += "StrategyMinMemory | ";
    if ( value & AllocationCreateFlagBits::eStrategyMinTime ) result += "StrategyMinTime | ";
    if ( value & AllocationCreateFlagBits::eStrategyMinFragmentation ) result += "StrategyMinFragmentation | ";
    return "{ " + result.substr(0, result.size() - 3) + " }";
  }

  enum class AllocatorCreateFlagBits : VmaAllocatorCreateFlags
  {
    eExternallySynchronized = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
    eKhrDedicatedAllocation = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT,
    eKhrBindMemory2 = VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT,
    eExtMemoryBudget = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
    eAmdDeviceCoherentMemory = VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT,
    eBufferDeviceAdress = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT
  };

  VULKAN_HPP_INLINE std::string to_string( AllocatorCreateFlagBits value )
  {
    switch ( value )
    {
      case AllocatorCreateFlagBits::eExternallySynchronized : return "ExternallySynchronized";
      case AllocatorCreateFlagBits::eKhrDedicatedAllocation : return "KhrDedicatedAllocation";
      case AllocatorCreateFlagBits::eKhrBindMemory2 : return "KhrBindMemory2";
      case AllocatorCreateFlagBits::eExtMemoryBudget : return "ExtMemoryBudget";
      case AllocatorCreateFlagBits::eAmdDeviceCoherentMemory: return "AmdDeviceCoherentMemory";
      case AllocatorCreateFlagBits::eBufferDeviceAdress: return "BufferDeviceAdress";
      default: return "invalid";
    }
  }

  using AllocatorCreateFlags = VULKAN_HPP_NAMESPACE::Flags<AllocatorCreateFlagBits>;

  VULKAN_HPP_INLINE AllocatorCreateFlags operator|( AllocatorCreateFlagBits bit0, AllocatorCreateFlagBits bit1 )
  {
    return AllocatorCreateFlags( bit0 ) | bit1;
  }

  VULKAN_HPP_INLINE AllocatorCreateFlags operator~( AllocatorCreateFlagBits bits )
  {
    return ~( AllocatorCreateFlags( bits ) );
  }

  VULKAN_HPP_INLINE std::string to_string( AllocatorCreateFlags value  )
  {
    if ( !value ) return "{}";
    std::string result;

    if ( value & AllocatorCreateFlagBits::eExternallySynchronized ) result += "ExternallySynchronized | ";
    if ( value & AllocatorCreateFlagBits::eKhrDedicatedAllocation ) result += "KhrDedicatedAllocation | ";
    if ( value & AllocatorCreateFlagBits::eKhrBindMemory2 ) result += "KhrBindMemory2 | ";
    if ( value & AllocatorCreateFlagBits::eExtMemoryBudget ) result += "ExtMemoryBudget | ";
    return "{ " + result.substr(0, result.size() - 3) + " }";
  }

  enum class DefragmentationFlagBits : VmaDefragmentationFlags
  {};

  VULKAN_HPP_INLINE std::string to_string( DefragmentationFlagBits )
  {
    return "(void)";
  }

  using DefragmentationFlags = VULKAN_HPP_NAMESPACE::Flags<DefragmentationFlagBits>;

  VULKAN_HPP_INLINE std::string to_string( DefragmentationFlags  )
  {
    return "{}";
  }

  enum class PoolCreateFlagBits : VmaPoolCreateFlags
  {
    eIgnoreBufferImageGranularity = VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
    eLinearAlgorithm = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
    eBuddyAlgorithm = VMA_POOL_CREATE_BUDDY_ALGORITHM_BIT,
    eAlgorithmMask = VMA_POOL_CREATE_ALGORITHM_MASK
  };

  VULKAN_HPP_INLINE std::string to_string( PoolCreateFlagBits value )
  {
    switch ( value )
    {
      case PoolCreateFlagBits::eIgnoreBufferImageGranularity : return "IgnoreBufferImageGranularity";
      case PoolCreateFlagBits::eLinearAlgorithm : return "LinearAlgorithm";
      case PoolCreateFlagBits::eBuddyAlgorithm : return "BuddyAlgorithm";
      case PoolCreateFlagBits::eAlgorithmMask : return "AlgorithmMask";
      default: return "invalid";
    }
  }

  using PoolCreateFlags = VULKAN_HPP_NAMESPACE::Flags<PoolCreateFlagBits>;

  VULKAN_HPP_INLINE PoolCreateFlags operator|( PoolCreateFlagBits bit0, PoolCreateFlagBits bit1 )
  {
    return PoolCreateFlags( bit0 ) | bit1;
  }

  VULKAN_HPP_INLINE PoolCreateFlags operator~( PoolCreateFlagBits bits )
  {
    return ~( PoolCreateFlags( bits ) );
  }

  VULKAN_HPP_INLINE std::string to_string( PoolCreateFlags value  )
  {
    if ( !value ) return "{}";
    std::string result;

    if ( value & PoolCreateFlagBits::eIgnoreBufferImageGranularity ) result += "IgnoreBufferImageGranularity | ";
    if ( value & PoolCreateFlagBits::eLinearAlgorithm ) result += "LinearAlgorithm | ";
    if ( value & PoolCreateFlagBits::eBuddyAlgorithm ) result += "BuddyAlgorithm | ";
    return "{ " + result.substr(0, result.size() - 3) + " }";
  }

  enum class RecordFlagBits : VmaRecordFlags
  {
    eFlushAfterCall = VMA_RECORD_FLUSH_AFTER_CALL_BIT
  };

  VULKAN_HPP_INLINE std::string to_string( RecordFlagBits value )
  {
    switch ( value )
    {
      case RecordFlagBits::eFlushAfterCall : return "FlushAfterCall";
      default: return "invalid";
    }
  }

  using RecordFlags = VULKAN_HPP_NAMESPACE::Flags<RecordFlagBits>;

  VULKAN_HPP_INLINE RecordFlags operator|( RecordFlagBits bit0, RecordFlagBits bit1 )
  {
    return RecordFlags( bit0 ) | bit1;
  }

  VULKAN_HPP_INLINE RecordFlags operator~( RecordFlagBits bits )
  {
    return ~( RecordFlags( bits ) );
  }

  VULKAN_HPP_INLINE std::string to_string( RecordFlags value  )
  {
    if ( !value ) return "{}";
    std::string result;

    if ( value & RecordFlagBits::eFlushAfterCall ) result += "FlushAfterCall | ";
    return "{ " + result.substr(0, result.size() - 3) + " }";
  }

  class Allocator
  {
  public:
    using CType = VmaAllocator;

  public:
    VULKAN_HPP_CONSTEXPR Allocator()
      : m_allocator(VK_NULL_HANDLE)
    {}

    VULKAN_HPP_CONSTEXPR Allocator( std::nullptr_t )
      : m_allocator(VK_NULL_HANDLE)
    {}

    VULKAN_HPP_TYPESAFE_EXPLICIT Allocator( VmaAllocator allocator )
      : m_allocator( allocator )
    {}

#if defined(VULKAN_HPP_TYPESAFE_CONVERSION)
    Allocator & operator=(VmaAllocator allocator)
    {
      m_allocator = allocator;
      return *this; 
    }
#endif

    Allocator & operator=( std::nullptr_t )
    {
      m_allocator = VK_NULL_HANDLE;
      return *this;
    }

    bool operator==( Allocator const & rhs ) const
    {
      return m_allocator == rhs.m_allocator;
    }

    bool operator!=(Allocator const & rhs ) const
    {
      return m_allocator != rhs.m_allocator;
    }

    bool operator<(Allocator const & rhs ) const
    {
      return m_allocator < rhs.m_allocator;
    }

    VULKAN_HPP_NAMESPACE::Result allocateMemory( const VULKAN_HPP_NAMESPACE::MemoryRequirements* pVkMemoryRequirements, const AllocationCreateInfo* pCreateInfo, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<Allocation>::type allocateMemory( const VULKAN_HPP_NAMESPACE::MemoryRequirements & vkMemoryRequirements, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo = nullptr ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    VULKAN_HPP_NAMESPACE::Result allocateMemoryForBuffer( VULKAN_HPP_NAMESPACE::Buffer buffer, const AllocationCreateInfo* pCreateInfo, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<Allocation>::type allocateMemoryForBuffer( VULKAN_HPP_NAMESPACE::Buffer buffer, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo = nullptr ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    VULKAN_HPP_NAMESPACE::Result allocateMemoryForImage( VULKAN_HPP_NAMESPACE::Image image, const AllocationCreateInfo* pCreateInfo, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<Allocation>::type allocateMemoryForImage( VULKAN_HPP_NAMESPACE::Image image, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo = nullptr ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    VULKAN_HPP_NAMESPACE::Result allocateMemoryPages( const VULKAN_HPP_NAMESPACE::MemoryRequirements* pVkMemoryRequirements, const AllocationCreateInfo* pCreateInfo, size_t allocationCount, Allocation* pAllocations, AllocationInfo* pAllocationInfos ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    template<typename VectorAllocator = std::allocator<AllocationInfo>>
    typename VULKAN_HPP_NAMESPACE::ResultValueType<std::vector<AllocationInfo,VectorAllocator>>::type allocateMemoryPages( const VULKAN_HPP_NAMESPACE::MemoryRequirements & vkMemoryRequirements, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::ArrayProxy<Allocation> allocations ) const;
    template<typename VectorAllocator = std::allocator<AllocationInfo>>
    typename VULKAN_HPP_NAMESPACE::ResultValueType<std::vector<AllocationInfo,VectorAllocator>>::type allocateMemoryPages( const VULKAN_HPP_NAMESPACE::MemoryRequirements & vkMemoryRequirements, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::ArrayProxy<Allocation> allocations, Allocator const& vectorAllocator ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::Result bindBufferMemory( Allocation allocation, VULKAN_HPP_NAMESPACE::Buffer buffer ) const;
#else
    VULKAN_HPP_NAMESPACE::ResultValueType<void>::type bindBufferMemory( Allocation allocation, VULKAN_HPP_NAMESPACE::Buffer buffer ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::Result bindBufferMemory2( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize allocationLocalOffset, VULKAN_HPP_NAMESPACE::Buffer buffer, const void* pNext ) const;
#else
    VULKAN_HPP_NAMESPACE::ResultValueType<void>::type bindBufferMemory2( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize allocationLocalOffset, VULKAN_HPP_NAMESPACE::Buffer buffer, const void* pNext ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::Result bindImageMemory( Allocation allocation, VULKAN_HPP_NAMESPACE::Image image ) const;
#else
    VULKAN_HPP_NAMESPACE::ResultValueType<void>::type bindImageMemory( Allocation allocation, VULKAN_HPP_NAMESPACE::Image image ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::Result bindImageMemory2( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize allocationLocalOffset, VULKAN_HPP_NAMESPACE::Image image, const void* pNext ) const;
#else
    VULKAN_HPP_NAMESPACE::ResultValueType<void>::type bindImageMemory2( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize allocationLocalOffset, VULKAN_HPP_NAMESPACE::Image image, const void* pNext ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void calculateStats( Stats* pStats ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    Stats calculateStats() const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void getBudget( Budget* pBudget ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    Budget getBudget() const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::Result checkCorruption( uint32_t memoryTypeBits ) const;
#else
    VULKAN_HPP_NAMESPACE::ResultValueType<void>::type checkCorruption( uint32_t memoryTypeBits ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::Result checkPoolCorruption( Pool pool ) const;
#else
    VULKAN_HPP_NAMESPACE::ResultValueType<void>::type checkPoolCorruption( Pool pool ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
    void getPoolName( Pool pool, const char** ppName ) const;
#else
    const char* getPoolName( Pool pool ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  void setPoolName( Pool pool, const char* pName ) const;
#else
  void setPoolName( Pool pool, const char* pName ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    VULKAN_HPP_NAMESPACE::Result createBuffer( const VULKAN_HPP_NAMESPACE::BufferCreateInfo* pBufferCreateInfo, const AllocationCreateInfo* pAllocationCreateInfo, VULKAN_HPP_NAMESPACE::Buffer* pBuffer, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<std::pair<VULKAN_HPP_NAMESPACE::Buffer, vma::Allocation>>::type createBuffer( const VULKAN_HPP_NAMESPACE::BufferCreateInfo & bufferCreateInfo, const AllocationCreateInfo & allocationCreateInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo = nullptr ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifndef VULKAN_HPP_NO_SMART_HANDLE
    template<typename Dispatch = VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
    VULKAN_HPP_NAMESPACE::ResultValueType<std::pair<VULKAN_HPP_NAMESPACE::UniqueBuffer, vma::UniqueAllocation>>::type createBufferUnique(const VULKAN_HPP_NAMESPACE::BufferCreateInfo& bufferCreateInfo, const AllocationCreateInfo& allocationCreateInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo VULKAN_HPP_DEFAULT_ARGUMENT_NULLPTR_ASSIGNMENT, Dispatch const& d VULKAN_HPP_DEFAULT_DISPATCHER_ASSIGNMENT) const;
#endif /*VULKAN_HPP_NO_SMART_HANDLE*/

    VULKAN_HPP_NAMESPACE::Result createImage( const VULKAN_HPP_NAMESPACE::ImageCreateInfo* pImageCreateInfo, const AllocationCreateInfo* pAllocationCreateInfo, VULKAN_HPP_NAMESPACE::Image* pImage, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<std::pair<VULKAN_HPP_NAMESPACE::Image, vma::Allocation>>::type createImage( const VULKAN_HPP_NAMESPACE::ImageCreateInfo & imageCreateInfo, const AllocationCreateInfo & allocationCreateInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo = nullptr ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifndef VULKAN_HPP_NO_SMART_HANDLE
    template<typename Dispatch = VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
    VULKAN_HPP_NAMESPACE::ResultValueType<std::pair<VULKAN_HPP_NAMESPACE::UniqueImage, vma::UniqueAllocation>>::type createImageUnique(const VULKAN_HPP_NAMESPACE::ImageCreateInfo& imageCreateInfo, const AllocationCreateInfo& allocationCreateInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo VULKAN_HPP_DEFAULT_ARGUMENT_NULLPTR_ASSIGNMENT, Dispatch const& d VULKAN_HPP_DEFAULT_DISPATCHER_ASSIGNMENT) const;
#endif /*VULKAN_HPP_NO_SMART_HANDLE*/

    void createLostAllocation( Allocation* pAllocation ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    Allocation createLostAllocation() const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    VULKAN_HPP_NAMESPACE::Result createPool(const PoolCreateInfo* pCreateInfo, Pool* pPool ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<Pool>::type createPool( const PoolCreateInfo & createInfo ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifndef VULKAN_HPP_NO_SMART_HANDLE
    VULKAN_HPP_NAMESPACE::ResultValueType<UniqueHandle<Pool>>::type createPoolUnique(const PoolCreateInfo& createInfo) const;
#endif /*VULKAN_HPP_NO_SMART_HANDLE*/

    VULKAN_HPP_NAMESPACE::Result defragmentationBegin( const DefragmentationInfo2* pInfo, DefragmentationStats* pStats, DefragmentationContext* pContext ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<DefragmentationContext>::type defragmentationBegin( const DefragmentationInfo2 & info, VULKAN_HPP_NAMESPACE::Optional<DefragmentationStats> stats = nullptr ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::Result defragmentationEnd( DefragmentationContext context ) const;
#else
    VULKAN_HPP_NAMESPACE::ResultValueType<void>::type defragmentationEnd( DefragmentationContext context ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void destroy() const;

    void destroyBuffer( VULKAN_HPP_NAMESPACE::Buffer buffer, Allocation allocation ) const;

    void destroyImage( VULKAN_HPP_NAMESPACE::Image image, Allocation allocation ) const;

    void destroyPool(Pool pool) const;

    void destroy(Pool pool) const;

    VULKAN_HPP_NAMESPACE::Result findMemoryTypeIndex( uint32_t memoryTypeBits, const AllocationCreateInfo* pAllocationCreateInfo, uint32_t* pMemoryTypeIndex ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<uint32_t>::type findMemoryTypeIndex( uint32_t memoryTypeBits, const AllocationCreateInfo & allocationCreateInfo ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    VULKAN_HPP_NAMESPACE::Result findMemoryTypeIndexForBufferInfo( const VULKAN_HPP_NAMESPACE::BufferCreateInfo* pBufferCreateInfo, const AllocationCreateInfo* pAllocationCreateInfo, uint32_t* pMemoryTypeIndex ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<uint32_t>::type findMemoryTypeIndexForBufferInfo( const VULKAN_HPP_NAMESPACE::BufferCreateInfo & bufferCreateInfo, const AllocationCreateInfo & allocationCreateInfo ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    VULKAN_HPP_NAMESPACE::Result findMemoryTypeIndexForImageInfo( const VULKAN_HPP_NAMESPACE::ImageCreateInfo* pImageCreateInfo, const AllocationCreateInfo* pAllocationCreateInfo, uint32_t* pMemoryTypeIndex ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<uint32_t>::type findMemoryTypeIndexForImageInfo( const VULKAN_HPP_NAMESPACE::ImageCreateInfo & imageCreateInfo, const AllocationCreateInfo & allocationCreateInfo ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void flushAllocation( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize offset, VULKAN_HPP_NAMESPACE::DeviceSize size ) const;

    void freeMemory( Allocation allocation ) const;

    void freeMemoryPages( size_t allocationCount, Allocation* pAllocations ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    void freeMemoryPages( VULKAN_HPP_NAMESPACE::ArrayProxy<Allocation> allocations ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void free(Allocation allocation) const;

    void getAllocationInfo( Allocation allocation, AllocationInfo* pAllocationInfo ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    AllocationInfo getAllocationInfo( Allocation allocation ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void getMemoryTypeProperties( uint32_t memoryTypeIndex, VULKAN_HPP_NAMESPACE::MemoryPropertyFlags* pFlags ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::MemoryPropertyFlags getMemoryTypeProperties( uint32_t memoryTypeIndex ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void getPoolStats( Pool pool, PoolStats* pPoolStats ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    PoolStats getPoolStats( Pool pool ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void invalidateAllocation( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize offset, VULKAN_HPP_NAMESPACE::DeviceSize size ) const;

    void makePoolAllocationsLost( Pool pool, size_t* pLostAllocationCount ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    size_t makePoolAllocationsLost( Pool pool ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    VULKAN_HPP_NAMESPACE::Result mapMemory( Allocation allocation, void** ppData ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    VULKAN_HPP_NAMESPACE::ResultValueType<void*>::type mapMemory( Allocation allocation ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void setAllocationUserData( Allocation allocation, void* pUserData ) const;
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
    void setAllocationUserData( Allocation allocation ) const;
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

    void setCurrentFrameIndex( uint32_t frameIndex ) const;

    VULKAN_HPP_NAMESPACE::Bool32 touchAllocation( Allocation allocation ) const;

    void unmapMemory( Allocation allocation ) const;

    VULKAN_HPP_TYPESAFE_EXPLICIT operator VmaAllocator() const
    {
      return m_allocator;
    }

    explicit operator bool() const
    {
      return m_allocator != VK_NULL_HANDLE;
    }

    bool operator!() const
    {
      return m_allocator == VK_NULL_HANDLE;
    }

  private:
    VmaAllocator m_allocator;
  };
  static_assert( sizeof( Allocator ) == sizeof( VmaAllocator ), "handle and wrapper have different size!" );

  class Pool
  {
  public:
    using CType = VmaPool;

  public:
    VULKAN_HPP_CONSTEXPR Pool()
      : m_pool(VK_NULL_HANDLE)
    {}

    VULKAN_HPP_CONSTEXPR Pool( std::nullptr_t )
      : m_pool(VK_NULL_HANDLE)
    {}

    VULKAN_HPP_TYPESAFE_EXPLICIT Pool( VmaPool pool )
      : m_pool( pool )
    {}

#if defined(VULKAN_HPP_TYPESAFE_CONVERSION)
    Pool & operator=(VmaPool pool)
    {
      m_pool = pool;
      return *this; 
    }
#endif

    Pool & operator=( std::nullptr_t )
    {
      m_pool = VK_NULL_HANDLE;
      return *this;
    }

    bool operator==( Pool const & rhs ) const
    {
      return m_pool == rhs.m_pool;
    }

    bool operator!=(Pool const & rhs ) const
    {
      return m_pool != rhs.m_pool;
    }

    bool operator<(Pool const & rhs ) const
    {
      return m_pool < rhs.m_pool;
    }

    VULKAN_HPP_TYPESAFE_EXPLICIT operator VmaPool() const
    {
      return m_pool;
    }

    explicit operator bool() const
    {
      return m_pool != VK_NULL_HANDLE;
    }

    bool operator!() const
    {
      return m_pool == VK_NULL_HANDLE;
    }

  private:
    VmaPool m_pool;
  };
  static_assert( sizeof( Pool ) == sizeof( VmaPool ), "handle and wrapper have different size!" );
   
  class Allocation
  {
  public:
    using CType = VmaAllocation;

  public:
    VULKAN_HPP_CONSTEXPR Allocation()
      : m_allocation(VK_NULL_HANDLE)
    {}

    VULKAN_HPP_CONSTEXPR Allocation( std::nullptr_t )
      : m_allocation(VK_NULL_HANDLE)
    {}

    VULKAN_HPP_TYPESAFE_EXPLICIT Allocation( VmaAllocation allocation )
      : m_allocation( allocation )
    {}

#if defined(VULKAN_HPP_TYPESAFE_CONVERSION)
    Allocation & operator=(VmaAllocation allocation)
    {
      m_allocation = allocation;
      return *this; 
    }
#endif

    Allocation & operator=( std::nullptr_t )
    {
      m_allocation = VK_NULL_HANDLE;
      return *this;
    }

    bool operator==( Allocation const & rhs ) const
    {
      return m_allocation == rhs.m_allocation;
    }

    bool operator!=(Allocation const & rhs ) const
    {
      return m_allocation != rhs.m_allocation;
    }

    bool operator<(Allocation const & rhs ) const
    {
      return m_allocation < rhs.m_allocation;
    }

    VULKAN_HPP_TYPESAFE_EXPLICIT operator VmaAllocation() const
    {
      return m_allocation;
    }

    explicit operator bool() const
    {
      return m_allocation != VK_NULL_HANDLE;
    }

    bool operator!() const
    {
      return m_allocation == VK_NULL_HANDLE;
    }

  private:
    VmaAllocation m_allocation;
  };
  static_assert( sizeof( Allocation ) == sizeof( VmaAllocation ), "handle and wrapper have different size!" );

  class DefragmentationContext
  {
  public:
    using CType = VmaDefragmentationContext;

  public:
    VULKAN_HPP_CONSTEXPR DefragmentationContext()
      : m_defragmentationContext(VK_NULL_HANDLE)
    {}

    VULKAN_HPP_CONSTEXPR DefragmentationContext( std::nullptr_t )
      : m_defragmentationContext(VK_NULL_HANDLE)
    {}

    VULKAN_HPP_TYPESAFE_EXPLICIT DefragmentationContext( VmaDefragmentationContext defragmentationContext )
      : m_defragmentationContext( defragmentationContext )
    {}

#if defined(VULKAN_HPP_TYPESAFE_CONVERSION)
    DefragmentationContext & operator=(VmaDefragmentationContext defragmentationContext)
    {
      m_defragmentationContext = defragmentationContext;
      return *this; 
    }
#endif

    DefragmentationContext & operator=( std::nullptr_t )
    {
      m_defragmentationContext = VK_NULL_HANDLE;
      return *this;
    }

    bool operator==( DefragmentationContext const & rhs ) const
    {
      return m_defragmentationContext == rhs.m_defragmentationContext;
    }

    bool operator!=(DefragmentationContext const & rhs ) const
    {
      return m_defragmentationContext != rhs.m_defragmentationContext;
    }

    bool operator<(DefragmentationContext const & rhs ) const
    {
      return m_defragmentationContext < rhs.m_defragmentationContext;
    }

    VULKAN_HPP_TYPESAFE_EXPLICIT operator VmaDefragmentationContext() const
    {
      return m_defragmentationContext;
    }

    explicit operator bool() const
    {
      return m_defragmentationContext != VK_NULL_HANDLE;
    }

    bool operator!() const
    {
      return m_defragmentationContext == VK_NULL_HANDLE;
    }

  private:
    VmaDefragmentationContext m_defragmentationContext;
  };
  static_assert( sizeof( DefragmentationContext ) == sizeof( VmaDefragmentationContext ), "handle and wrapper have different size!" );

  VULKAN_HPP_NAMESPACE::Result createAllocator( const AllocatorCreateInfo* pCreateInfo, Allocator* pAllocator );
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_NAMESPACE::ResultValueType<Allocator>::type createAllocator( const AllocatorCreateInfo & createInfo );
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/
#ifndef VULKAN_HPP_NO_SMART_HANDLE
  VULKAN_HPP_NAMESPACE::ResultValueType<UniqueHandle<Allocator>>::type createAllocatorUnique(const AllocatorCreateInfo& createInfo);
#endif /*VULKAN_HPP_NO_SMART_HANDLE*/

  struct AllocationCreateInfo
  { 
    AllocationCreateInfo& operator=( VmaAllocationCreateInfo const & rhs ) VULKAN_HPP_NOEXCEPT
    {
      *reinterpret_cast<VmaAllocationCreateInfo*>(this) = rhs;
      return *this;
    }

    operator VmaAllocationCreateInfo const&() const VULKAN_HPP_NOEXCEPT
    {
      return *reinterpret_cast<const VmaAllocationCreateInfo*>( this );
    }

    operator VmaAllocationCreateInfo &() VULKAN_HPP_NOEXCEPT
    {
      return *reinterpret_cast<VmaAllocationCreateInfo*>( this );
    }

    auto operator<=>(AllocationCreateInfo const&) const = default;

  public:
    AllocationCreateFlags flags;
    MemoryUsage usage;
    VULKAN_HPP_NAMESPACE::MemoryPropertyFlags requiredFlags;
    VULKAN_HPP_NAMESPACE::MemoryPropertyFlags preferredFlags;
    uint32_t memoryTypeBits;
    Pool pool;
    void* pUserData;
  };
  static_assert( sizeof( AllocationCreateInfo ) == sizeof( VmaAllocationCreateInfo ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<AllocationCreateInfo>::value, "struct wrapper is not a standard layout!" );

  struct AllocationInfo
  {  
    AllocationInfo& operator=( VmaAllocationInfo const & rhs )
    {
      *reinterpret_cast<VmaAllocationInfo*>(this) = rhs;
      return *this;
    }

    operator VmaAllocationInfo const&() const
    {
      return *reinterpret_cast<const VmaAllocationInfo*>( this );
    }

    operator VmaAllocationInfo &()
    {
      return *reinterpret_cast<VmaAllocationInfo*>( this );
    }

    auto operator<=>(AllocationInfo const&) const = default;

  public:
    uint32_t memoryType;
    VULKAN_HPP_NAMESPACE::DeviceMemory deviceMemory;
    VULKAN_HPP_NAMESPACE::DeviceSize offset;
    VULKAN_HPP_NAMESPACE::DeviceSize size;
    void* pMappedData;
    void* pUserData;
  };
  static_assert( sizeof( AllocationInfo ) == sizeof( VmaAllocationInfo ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<AllocationInfo>::value, "struct wrapper is not a standard layout!" );

  struct DeviceMemoryCallbacks
  {
    DeviceMemoryCallbacks& operator=( VmaDeviceMemoryCallbacks const & rhs )
    {
      *reinterpret_cast<VmaDeviceMemoryCallbacks*>(this) = rhs;
      return *this;
    }

    operator VmaDeviceMemoryCallbacks const&() const
    {
      return *reinterpret_cast<const VmaDeviceMemoryCallbacks*>( this );
    }

    operator VmaDeviceMemoryCallbacks &()
    {
      return *reinterpret_cast<VmaDeviceMemoryCallbacks*>( this );
    }

    auto operator<=>(DeviceMemoryCallbacks const&) const = default;

  public:
    PFN_vmaAllocateDeviceMemoryFunction pfnAllocate;
    PFN_vmaFreeDeviceMemoryFunction pfnFree;
    void* pUserData;
  };
  static_assert( sizeof( DeviceMemoryCallbacks ) == sizeof( VmaDeviceMemoryCallbacks ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<DeviceMemoryCallbacks>::value, "struct wrapper is not a standard layout!" );

  struct VulkanFunctions
  {

    VulkanFunctions& operator=( VmaVulkanFunctions const & rhs )
    {
      *reinterpret_cast<VmaVulkanFunctions*>(this) = rhs;
      return *this;
    }

    operator VmaVulkanFunctions const&() const
    {
      return *reinterpret_cast<const VmaVulkanFunctions*>( this );
    }

    operator VmaVulkanFunctions &()
    {
      return *reinterpret_cast<VmaVulkanFunctions*>( this );
    }

    auto operator<=>(VulkanFunctions const&) const = default;

  public:
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    PFN_vkAllocateMemory vkAllocateMemory;
    PFN_vkFreeMemory vkFreeMemory;
    PFN_vkMapMemory vkMapMemory;
    PFN_vkUnmapMemory vkUnmapMemory;
    PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
    PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
    PFN_vkBindBufferMemory vkBindBufferMemory;
    PFN_vkBindImageMemory vkBindImageMemory;
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
    PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
    PFN_vkCreateBuffer vkCreateBuffer;
    PFN_vkDestroyBuffer vkDestroyBuffer;
    PFN_vkCreateImage vkCreateImage;
    PFN_vkDestroyImage vkDestroyImage;
    PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
    PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR;
    PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR;
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
    PFN_vkBindBufferMemory2KHR vkBindBufferMemory2KHR;
    PFN_vkBindImageMemory2KHR vkBindImageMemory2KHR;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
    PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR;
#endif
  };
  static_assert( sizeof( VulkanFunctions ) == sizeof( VmaVulkanFunctions ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<VulkanFunctions>::value, "struct wrapper is not a standard layout!" );

  struct RecordSettings
  {
    RecordSettings& operator=( VmaRecordSettings const & rhs )
    {
      *reinterpret_cast<VmaRecordSettings*>(this) = rhs;
      return *this;
    }

    operator VmaRecordSettings const&() const
    {
      return *reinterpret_cast<const VmaRecordSettings*>( this );
    }

    operator VmaRecordSettings &()
    {
      return *reinterpret_cast<VmaRecordSettings*>( this );
    }

    auto operator<=>(RecordSettings const&) const = default;

  public:
    RecordFlags flags;
    const char* pFilePath;
  };
  static_assert( sizeof( RecordSettings ) == sizeof( VmaRecordSettings ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<RecordSettings>::value, "struct wrapper is not a standard layout!" );

  struct AllocatorCreateInfo
  {
    AllocatorCreateInfo& operator=( VmaAllocatorCreateInfo const & rhs )
    {
      *reinterpret_cast<VmaAllocatorCreateInfo*>(this) = rhs;
      return *this;
    }

    operator VmaAllocatorCreateInfo const&() const
    {
      return *reinterpret_cast<const VmaAllocatorCreateInfo*>( this );
    }

    operator VmaAllocatorCreateInfo &()
    {
      return *reinterpret_cast<VmaAllocatorCreateInfo*>( this );
    }

    auto operator<=>(AllocatorCreateInfo const&) const = default;

  public:
    AllocatorCreateFlags flags;
    VULKAN_HPP_NAMESPACE::PhysicalDevice physicalDevice;
    VULKAN_HPP_NAMESPACE::Device device;
    VULKAN_HPP_NAMESPACE::DeviceSize preferredLargeHeapBlockSize;
    const VULKAN_HPP_NAMESPACE::AllocationCallbacks* pAllocationCallbacks;
    const DeviceMemoryCallbacks* pDeviceMemoryCallbacks;
    uint32_t frameInUseCount;
    const VULKAN_HPP_NAMESPACE::DeviceSize* pHeapSizeLimit;
    const VulkanFunctions* pVulkanFunctions;
    const RecordSettings* pRecordSettings;
    VULKAN_HPP_NAMESPACE::Instance instance;
    uint32_t vulkanApiVersion;
  };
  static_assert( sizeof( AllocatorCreateInfo ) == sizeof( VmaAllocatorCreateInfo ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<AllocatorCreateInfo>::value, "struct wrapper is not a standard layout!" );

  struct DefragmentationInfo2
  {
  
    DefragmentationInfo2& operator=( VmaDefragmentationInfo2 const & rhs )
    {
      *reinterpret_cast<VmaDefragmentationInfo2*>(this) = rhs;
      return *this;
    }

    operator VmaDefragmentationInfo2 const&() const
    {
      return *reinterpret_cast<const VmaDefragmentationInfo2*>( this );
    }

    operator VmaDefragmentationInfo2 &()
    {
      return *reinterpret_cast<VmaDefragmentationInfo2*>( this );
    }

    auto operator<=>(DefragmentationInfo2 const&) const = default;

  public:
    DefragmentationFlags flags;
    uint32_t allocationCount;
    Allocation* pAllocations;
    VULKAN_HPP_NAMESPACE::Bool32* pAllocationsChanged;
    uint32_t poolCount;
    Pool* pPools;
    VULKAN_HPP_NAMESPACE::DeviceSize maxCpuBytesToMove;
    uint32_t maxCpuAllocationsToMove;
    VULKAN_HPP_NAMESPACE::DeviceSize maxGpuBytesToMove;
    uint32_t maxGpuAllocationsToMove;
    VULKAN_HPP_NAMESPACE::CommandBuffer commandBuffer;
  };
  static_assert( sizeof( DefragmentationInfo2 ) == sizeof( VmaDefragmentationInfo2 ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<DefragmentationInfo2>::value, "struct wrapper is not a standard layout!" );

  struct DefragmentationStats
  {
    DefragmentationStats& operator=( VmaDefragmentationStats const & rhs )
    {
      *reinterpret_cast<VmaDefragmentationStats*>(this) = rhs;
      return *this;
    }

    operator VmaDefragmentationStats const&() const
    {
      return *reinterpret_cast<const VmaDefragmentationStats*>( this );
    }

    operator VmaDefragmentationStats &()
    {
      return *reinterpret_cast<VmaDefragmentationStats*>( this );
    }

    auto operator<=>(DefragmentationStats const&) const = default;

  public:
    VULKAN_HPP_NAMESPACE::DeviceSize bytesMoved;
    VULKAN_HPP_NAMESPACE::DeviceSize bytesFreed;
    uint32_t allocationsMoved;
    uint32_t deviceMemoryBlocksFreed;
  };
  static_assert( sizeof( DefragmentationStats ) == sizeof( VmaDefragmentationStats ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<DefragmentationStats>::value, "struct wrapper is not a standard layout!" );

  struct PoolCreateInfo
  {
    PoolCreateInfo& operator=( VmaPoolCreateInfo const & rhs )
    {
      *reinterpret_cast<VmaPoolCreateInfo*>(this) = rhs;
      return *this;
    }

    operator VmaPoolCreateInfo const&() const
    {
      return *reinterpret_cast<const VmaPoolCreateInfo*>( this );
    }

    operator VmaPoolCreateInfo &()
    {
      return *reinterpret_cast<VmaPoolCreateInfo*>( this );
    }

    auto operator<=>(PoolCreateInfo const&) const = default;

  public:
    uint32_t memoryTypeIndex;
    PoolCreateFlags flags;
    VULKAN_HPP_NAMESPACE::DeviceSize blockSize;
    size_t minBlockCount;
    size_t maxBlockCount;
    uint32_t frameInUseCount;
  };
  static_assert( sizeof( PoolCreateInfo ) == sizeof( VmaPoolCreateInfo ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<PoolCreateInfo>::value, "struct wrapper is not a standard layout!" );

  struct PoolStats
  {
   
    PoolStats& operator=( VmaPoolStats const & rhs )
    {
      *reinterpret_cast<VmaPoolStats*>(this) = rhs;
      return *this;
    }

    operator VmaPoolStats const&() const
    {
      return *reinterpret_cast<const VmaPoolStats*>( this );
    }

    operator VmaPoolStats &()
    {
      return *reinterpret_cast<VmaPoolStats*>( this );
    }

    auto operator<=>(PoolStats const&) const = default;

  public:
    VULKAN_HPP_NAMESPACE::DeviceSize size;
    VULKAN_HPP_NAMESPACE::DeviceSize unusedSize;
    size_t allocationCount;
    size_t unusedRangeCount;
    VULKAN_HPP_NAMESPACE::DeviceSize unusedRangeSizeMax;
    size_t blockCount;
  };
  static_assert( sizeof( PoolStats ) == sizeof( VmaPoolStats ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<PoolStats>::value, "struct wrapper is not a standard layout!" );

  struct StatInfo
  {
   
    StatInfo& operator=( VmaStatInfo const & rhs )
    {
      *reinterpret_cast<VmaStatInfo*>(this) = rhs;
      return *this;
    }

    operator VmaStatInfo const&() const
    {
      return *reinterpret_cast<const VmaStatInfo*>( this );
    }

    operator VmaStatInfo &()
    {
      return *reinterpret_cast<VmaStatInfo*>( this );
    }

    auto operator<=>(StatInfo const&) const = default;

  public:
    uint32_t blockCount;
    uint32_t allocationCount;
    uint32_t unusedRangeCount;
    VULKAN_HPP_NAMESPACE::DeviceSize usedBytes;
    VULKAN_HPP_NAMESPACE::DeviceSize unusedBytes;
    VULKAN_HPP_NAMESPACE::DeviceSize allocationSizeMin;
    VULKAN_HPP_NAMESPACE::DeviceSize allocationSizeAvg;
    VULKAN_HPP_NAMESPACE::DeviceSize allocationSizeMax;
    VULKAN_HPP_NAMESPACE::DeviceSize unusedRangeSizeMin;
    VULKAN_HPP_NAMESPACE::DeviceSize unusedRangeSizeAvg;
    VULKAN_HPP_NAMESPACE::DeviceSize unusedRangeSizeMax;
  };
  static_assert( sizeof( StatInfo ) == sizeof( VmaStatInfo ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<StatInfo>::value, "struct wrapper is not a standard layout!" );

  struct Stats
  {

    Stats& operator=( VmaStats const & rhs )
    {
      *reinterpret_cast<VmaStats*>(this) = rhs;
      return *this;
    }

    operator VmaStats const&() const
    {
      return *reinterpret_cast<const VmaStats*>( this );
    }

    operator VmaStats &()
    {
      return *reinterpret_cast<VmaStats*>( this );
    }

    auto operator<=>(Stats const&) const = default;

  public:
    StatInfo memoryType[VK_MAX_MEMORY_TYPES];
    StatInfo memoryHeap[VK_MAX_MEMORY_HEAPS];
    StatInfo total;
  };
  static_assert( sizeof( Stats ) == sizeof( VmaStats ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<Stats>::value, "struct wrapper is not a standard layout!" );
  
  struct Budget
  {
   
    Budget& operator=( VmaBudget const & rhs )
    {
      *reinterpret_cast<VmaBudget*>(this) = rhs;
      return *this;
    }
    

    operator VmaBudget const&() const
    {
      return *reinterpret_cast<const VmaBudget*>( this );
    }

    operator VmaBudget &()
    {
      return *reinterpret_cast<VmaBudget*>( this );
    }

    auto operator<=>(Budget const&) const = default;
  public:
      VULKAN_HPP_NAMESPACE::DeviceSize blockBytes;
      VULKAN_HPP_NAMESPACE::DeviceSize allocationBytes;
      VULKAN_HPP_NAMESPACE::DeviceSize usage;
      VULKAN_HPP_NAMESPACE::DeviceSize budget;
  };
  static_assert( sizeof( Budget ) == sizeof( VmaBudget ), "struct and wrapper have different size!" );
  static_assert( std::is_standard_layout<Budget>::value, "struct wrapper is not a standard layout!" );

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result createAllocator( const AllocatorCreateInfo* pCreateInfo, Allocator* pAllocator)
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCreateAllocator( reinterpret_cast<const VmaAllocatorCreateInfo*>( pCreateInfo ), reinterpret_cast<VmaAllocator*>( pAllocator ) ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<Allocator>::type createAllocator( const AllocatorCreateInfo & createInfo )
  {
    Allocator allocator;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCreateAllocator( reinterpret_cast<const VmaAllocatorCreateInfo*>( &createInfo ), reinterpret_cast<VmaAllocator*>( &allocator ) ) );
    return createResultValue( result, allocator, VMA_HPP_NAMESPACE_STRING"::createAllocator" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifndef VULKAN_HPP_NO_SMART_HANDLE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<UniqueHandle<Allocator>>::type createAllocatorUnique(const AllocatorCreateInfo& createInfo)
  {
      return UniqueAllocator(createAllocator(createInfo), ObjectDestroy<NoParent>());
  }
#endif /*VULKAN_HPP_NO_SMART_HANDLE*/


  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::allocateMemory( const VULKAN_HPP_NAMESPACE::MemoryRequirements* pVkMemoryRequirements, const AllocationCreateInfo* pCreateInfo, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaAllocateMemory( m_allocator, reinterpret_cast<const VkMemoryRequirements*>( pVkMemoryRequirements ), reinterpret_cast<const VmaAllocationCreateInfo*>( pCreateInfo ), reinterpret_cast<VmaAllocation*>( pAllocation ), reinterpret_cast<VmaAllocationInfo*>( pAllocationInfo ) ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<Allocation>::type Allocator::allocateMemory( const VULKAN_HPP_NAMESPACE::MemoryRequirements & vkMemoryRequirements, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo ) const
  {
    Allocation allocation;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaAllocateMemory( m_allocator, reinterpret_cast<const VkMemoryRequirements*>( &vkMemoryRequirements ), reinterpret_cast<const VmaAllocationCreateInfo*>( &createInfo ), reinterpret_cast<VmaAllocation*>( &allocation ), reinterpret_cast<VmaAllocationInfo*>( static_cast<AllocationInfo*>( allocationInfo ) ) ) );
    return createResultValue( result, allocation, VMA_HPP_NAMESPACE_STRING"::Allocator::allocateMemory" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::allocateMemoryForBuffer( VULKAN_HPP_NAMESPACE::Buffer buffer, const AllocationCreateInfo* pCreateInfo, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaAllocateMemoryForBuffer( m_allocator, static_cast<VkBuffer>( buffer ), reinterpret_cast<const VmaAllocationCreateInfo*>( pCreateInfo ), reinterpret_cast<VmaAllocation*>( pAllocation ), reinterpret_cast<VmaAllocationInfo*>( pAllocationInfo ) ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<Allocation>::type Allocator::allocateMemoryForBuffer( VULKAN_HPP_NAMESPACE::Buffer buffer, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo ) const
  {
    Allocation allocation;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaAllocateMemoryForBuffer( m_allocator, static_cast<VkBuffer>( buffer ), reinterpret_cast<const VmaAllocationCreateInfo*>( &createInfo ), reinterpret_cast<VmaAllocation*>( &allocation ), reinterpret_cast<VmaAllocationInfo*>( static_cast<AllocationInfo*>( allocationInfo ) ) ) );
    return createResultValue( result, allocation, VMA_HPP_NAMESPACE_STRING"::Allocator::allocateMemoryForBuffer" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::allocateMemoryForImage( VULKAN_HPP_NAMESPACE::Image image, const AllocationCreateInfo* pCreateInfo, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaAllocateMemoryForImage( m_allocator, static_cast<VkImage>( image ), reinterpret_cast<const VmaAllocationCreateInfo*>( pCreateInfo ), reinterpret_cast<VmaAllocation*>( pAllocation ), reinterpret_cast<VmaAllocationInfo*>( pAllocationInfo ) ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<Allocation>::type Allocator::allocateMemoryForImage( VULKAN_HPP_NAMESPACE::Image image, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo ) const
  {
    Allocation allocation;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaAllocateMemoryForImage( m_allocator, static_cast<VkImage>( image ), reinterpret_cast<const VmaAllocationCreateInfo*>( &createInfo ), reinterpret_cast<VmaAllocation*>( &allocation ), reinterpret_cast<VmaAllocationInfo*>( static_cast<AllocationInfo*>( allocationInfo ) ) ) );
    return createResultValue( result, allocation, VMA_HPP_NAMESPACE_STRING"::Allocator::allocateMemoryForImage" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::allocateMemoryPages( const VULKAN_HPP_NAMESPACE::MemoryRequirements* pVkMemoryRequirements, const AllocationCreateInfo* pCreateInfo, size_t allocationCount, Allocation* pAllocations, AllocationInfo* pAllocationInfos ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaAllocateMemoryPages( m_allocator, reinterpret_cast<const VkMemoryRequirements*>( pVkMemoryRequirements ), reinterpret_cast<const VmaAllocationCreateInfo*>( pCreateInfo ), allocationCount, reinterpret_cast<VmaAllocation*>( pAllocations ), reinterpret_cast<VmaAllocationInfo*>( pAllocationInfos ) ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  template<typename VectorAllocator>
  VULKAN_HPP_INLINE typename VULKAN_HPP_NAMESPACE::ResultValueType<std::vector<AllocationInfo,VectorAllocator>>::type Allocator::allocateMemoryPages( const VULKAN_HPP_NAMESPACE::MemoryRequirements & vkMemoryRequirements, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::ArrayProxy<Allocation> allocations ) const
  {
    std::vector<AllocationInfo,VectorAllocator> allocationInfos( allocations.size() );
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaAllocateMemoryPages( m_allocator, reinterpret_cast<const VkMemoryRequirements*>( &vkMemoryRequirements ), reinterpret_cast<const VmaAllocationCreateInfo*>( &createInfo ), allocations.size() , reinterpret_cast<VmaAllocation*>( allocations.data() ), reinterpret_cast<VmaAllocationInfo*>( allocationInfos.data() ) ) );
    return createResultValue( result, allocationInfos, VMA_HPP_NAMESPACE_STRING"::Allocator::allocateMemoryPages" );
  }
  template<typename VectorAllocator>
  VULKAN_HPP_INLINE typename VULKAN_HPP_NAMESPACE::ResultValueType<std::vector<AllocationInfo,VectorAllocator>>::type Allocator::allocateMemoryPages( const VULKAN_HPP_NAMESPACE::MemoryRequirements & vkMemoryRequirements, const AllocationCreateInfo & createInfo, VULKAN_HPP_NAMESPACE::ArrayProxy<Allocation> allocations, Allocator const& vectorAllocator ) const
  {
    std::vector<AllocationInfo,VectorAllocator> allocationInfos( allocations.size(), vectorAllocator );
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaAllocateMemoryPages( m_allocator, reinterpret_cast<const VkMemoryRequirements*>( &vkMemoryRequirements ), reinterpret_cast<const VmaAllocationCreateInfo*>( &createInfo ), allocations.size() , reinterpret_cast<VmaAllocation*>( allocations.data() ), reinterpret_cast<VmaAllocationInfo*>( allocationInfos.data() ) ) );
    return createResultValue( result, allocationInfos, VMA_HPP_NAMESPACE_STRING"::Allocator::allocateMemoryPages" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::bindBufferMemory( Allocation allocation, VULKAN_HPP_NAMESPACE::Buffer buffer ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaBindBufferMemory( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkBuffer>( buffer ) ) );
  }
#else
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<void>::type Allocator::bindBufferMemory( Allocation allocation, VULKAN_HPP_NAMESPACE::Buffer buffer ) const
  {
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaBindBufferMemory( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkBuffer>( buffer ) ) );
    return createResultValue( result, VMA_HPP_NAMESPACE_STRING"::Allocator::bindBufferMemory" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::bindBufferMemory2( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize allocationLocalOffset, VULKAN_HPP_NAMESPACE::Buffer buffer, const void* pNext ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaBindBufferMemory2( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkDeviceSize>( allocationLocalOffset ), static_cast<VkBuffer>( buffer ), pNext ) );
  }
#else
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<void>::type Allocator::bindBufferMemory2( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize allocationLocalOffset, VULKAN_HPP_NAMESPACE::Buffer buffer, const void* pNext ) const
  {
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaBindBufferMemory2( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkDeviceSize>( allocationLocalOffset ), static_cast<VkBuffer>( buffer ), pNext ) );
    return createResultValue( result, VMA_HPP_NAMESPACE_STRING"::Allocator::bindBufferMemory2" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::bindImageMemory( Allocation allocation, VULKAN_HPP_NAMESPACE::Image image ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaBindImageMemory( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkImage>( image ) ) );
  }
#else
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<void>::type Allocator::bindImageMemory( Allocation allocation, VULKAN_HPP_NAMESPACE::Image image ) const
  {
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaBindImageMemory( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkImage>( image ) ) );
    return createResultValue( result, VMA_HPP_NAMESPACE_STRING"::Allocator::bindImageMemory" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result bindImageMemory2( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize allocationLocalOffset, VULKAN_HPP_NAMESPACE::Image image, const void* pNext ) const;
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaBindImageMemory2( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkDeviceSize>( allocationLocalOffset ), static_cast<VkImage>( image ), pNext ) );
  }
#else
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<void>::type Allocator::bindImageMemory2( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize allocationLocalOffset, VULKAN_HPP_NAMESPACE::Image image, const void* pNext ) const
  {
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaBindImageMemory2( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkDeviceSize>( allocationLocalOffset ), static_cast<VkImage>( image ), pNext ) );
    return createResultValue( result, VMA_HPP_NAMESPACE_STRING"::Allocator::bindImageMemory2" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE void Allocator::calculateStats( Stats* pStats ) const
  {
    ::vmaCalculateStats( m_allocator, reinterpret_cast<VmaStats*>( pStats ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE Stats Allocator::calculateStats() const
  {
    Stats stats;
    ::vmaCalculateStats( m_allocator, reinterpret_cast<VmaStats*>( &stats ) );
    return stats;
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE void Allocator::getBudget( Budget* pBudget ) const
  {
    ::vmaGetBudget( m_allocator, reinterpret_cast<VmaBudget*>( pBudget ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE Budget Allocator::getBudget() const
  {
    Budget budget;
    ::vmaGetBudget( m_allocator, reinterpret_cast<VmaBudget*>( &budget ) );
    return budget;
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::checkCorruption( uint32_t memoryTypeBits ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCheckCorruption( m_allocator, memoryTypeBits ) );
  }
#else
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<void>::type Allocator::checkCorruption( uint32_t memoryTypeBits ) const
  {
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCheckCorruption( m_allocator, memoryTypeBits ) );
    return createResultValue( result, VMA_HPP_NAMESPACE_STRING"::Allocator::checkCorruption" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::checkPoolCorruption( Pool pool ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCheckPoolCorruption( m_allocator, static_cast<VmaPool>( pool ) ) );
  }
#else
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<void>::type Allocator::checkPoolCorruption( Pool pool ) const
  {
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCheckPoolCorruption( m_allocator, static_cast<VmaPool>( pool ) ) );
    return createResultValue( result, VMA_HPP_NAMESPACE_STRING"::Allocator::checkPoolCorruption" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::getPoolName( Pool pool, const char** ppName ) const
  {
    ::vmaGetPoolName( m_allocator, static_cast<VmaPool>( pool ), ppName );
  }
#else
  VULKAN_HPP_INLINE const char* Allocator::getPoolName( Pool pool ) const
  {
    const char* pName = nullptr;
    ::vmaGetPoolName( m_allocator, static_cast<VmaPool>( pool ), &pName );
    return pName;
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::setPoolName( Pool pool, const char* pName ) const
  {
    ::vmaSetPoolName( m_allocator, static_cast<VmaPool>( pool ), pName );
  }
#else
  VULKAN_HPP_INLINE void Allocator::setPoolName( Pool pool, const char* pName ) const
  {
    ::vmaSetPoolName( m_allocator, static_cast<VmaPool>( pool ), pName );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::createBuffer( const VULKAN_HPP_NAMESPACE::BufferCreateInfo* pBufferCreateInfo, const AllocationCreateInfo* pAllocationCreateInfo, VULKAN_HPP_NAMESPACE::Buffer* pBuffer, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCreateBuffer( m_allocator, reinterpret_cast<const VkBufferCreateInfo*>( pBufferCreateInfo ), reinterpret_cast<const VmaAllocationCreateInfo*>( pAllocationCreateInfo ), reinterpret_cast<VkBuffer*>( pBuffer ), reinterpret_cast<VmaAllocation*>( pAllocation ), reinterpret_cast<VmaAllocationInfo*>( pAllocationInfo ) ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<std::pair<VULKAN_HPP_NAMESPACE::Buffer, vma::Allocation>>::type Allocator::createBuffer( const VULKAN_HPP_NAMESPACE::BufferCreateInfo & bufferCreateInfo, const AllocationCreateInfo & allocationCreateInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo ) const
  {
    VULKAN_HPP_NAMESPACE::Buffer buffer;
    vma::Allocation allocation;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCreateBuffer( m_allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferCreateInfo ), reinterpret_cast<const VmaAllocationCreateInfo*>( &allocationCreateInfo ), reinterpret_cast<VkBuffer*>( &buffer ), reinterpret_cast<VmaAllocation*>( &allocation ), reinterpret_cast<VmaAllocationInfo*>( static_cast<AllocationInfo*>( allocationInfo ) ) ) );
    std::pair<VULKAN_HPP_NAMESPACE::Buffer, vma::Allocation> pair = std::make_pair( buffer, allocation );
    return createResultValue( result, pair, VMA_HPP_NAMESPACE_STRING"::Allocator::createBuffer" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifndef VULKAN_HPP_NO_SMART_HANDLE
  template<typename Dispatch>
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<std::pair<VULKAN_HPP_NAMESPACE::UniqueBuffer, vma::UniqueAllocation>>::type Allocator::createBufferUnique(const VULKAN_HPP_NAMESPACE::BufferCreateInfo& bufferCreateInfo, const AllocationCreateInfo& allocationCreateInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo, Dispatch const& d) const
  {
      VULKAN_HPP_NAMESPACE::Buffer buffer;
      vma::Allocation allocation;
      VmaAllocatorInfo allocatorInfo = {};
      vmaGetAllocatorInfo(m_allocator, &allocatorInfo);
      auto result = static_cast<VULKAN_HPP_NAMESPACE::Result>(::vmaCreateBuffer(m_allocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferCreateInfo), reinterpret_cast<const VmaAllocationCreateInfo*>(&allocationCreateInfo), reinterpret_cast<VkBuffer*>(&buffer), reinterpret_cast<VmaAllocation*>(&allocation), reinterpret_cast<VmaAllocationInfo*>(static_cast<AllocationInfo*>(allocationInfo))));
      auto pair = std::make_pair(VULKAN_HPP_NAMESPACE::UniqueBuffer{ buffer, VULKAN_HPP_NAMESPACE::ObjectDestroy<VULKAN_HPP_NAMESPACE::Device, Dispatch>( vk::Device{ allocatorInfo.device }) }, vma::UniqueAllocation{ allocation, ObjectFree<Allocator>(*this) });
      return createResultValue(result, pair, VMA_HPP_NAMESPACE_STRING"::Allocator::createBuffer");
  }
#endif /*VULKAN_HPP_NO_SMART_HANDLE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::createImage( const VULKAN_HPP_NAMESPACE::ImageCreateInfo* pImageCreateInfo, const AllocationCreateInfo* pAllocationCreateInfo, VULKAN_HPP_NAMESPACE::Image* pImage, Allocation* pAllocation, AllocationInfo* pAllocationInfo ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCreateImage( m_allocator, reinterpret_cast<const VkImageCreateInfo*>( pImageCreateInfo ), reinterpret_cast<const VmaAllocationCreateInfo*>( pAllocationCreateInfo ), reinterpret_cast<VkImage*>( pImage ), reinterpret_cast<VmaAllocation*>( pAllocation ), reinterpret_cast<VmaAllocationInfo*>( pAllocationInfo ) ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<std::pair<VULKAN_HPP_NAMESPACE::Image, vma::Allocation>>::type Allocator::createImage( const VULKAN_HPP_NAMESPACE::ImageCreateInfo & imageCreateInfo, const AllocationCreateInfo & allocationCreateInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo ) const
  {
    VULKAN_HPP_NAMESPACE::Image image;
    vma::Allocation allocation;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCreateImage( m_allocator, reinterpret_cast<const VkImageCreateInfo*>( &imageCreateInfo ), reinterpret_cast<const VmaAllocationCreateInfo*>( &allocationCreateInfo ), reinterpret_cast<VkImage*>( &image ), reinterpret_cast<VmaAllocation*>( &allocation ), reinterpret_cast<VmaAllocationInfo*>( static_cast<AllocationInfo*>( allocationInfo ) ) ) );
    std::pair<VULKAN_HPP_NAMESPACE::Image, vma::Allocation> pair = std::make_pair( image, allocation );
    return createResultValue( result, pair, VMA_HPP_NAMESPACE_STRING"::Allocator::createImage" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/
#ifndef VULKAN_HPP_NO_SMART_HANDLE
  template<typename Dispatch>
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<std::pair<VULKAN_HPP_NAMESPACE::UniqueImage, vma::UniqueAllocation>>::type Allocator::createImageUnique(const VULKAN_HPP_NAMESPACE::ImageCreateInfo& imageCreateInfo, const AllocationCreateInfo& allocationCreateInfo, VULKAN_HPP_NAMESPACE::Optional<AllocationInfo> allocationInfo, Dispatch const& d) const
  {
      VULKAN_HPP_NAMESPACE::Image image;
      vma::Allocation allocation;
      VmaAllocatorInfo allocatorInfo = {};
      vmaGetAllocatorInfo(m_allocator, &allocatorInfo);
      auto result = static_cast<VULKAN_HPP_NAMESPACE::Result>(::vmaCreateImage(m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), reinterpret_cast<const VmaAllocationCreateInfo*>(&allocationCreateInfo), reinterpret_cast<VkImage*>(&image), reinterpret_cast<VmaAllocation*>(&allocation), reinterpret_cast<VmaAllocationInfo*>(static_cast<AllocationInfo*>(allocationInfo))));
      auto pair = std::make_pair(VULKAN_HPP_NAMESPACE::UniqueImage{ image, VULKAN_HPP_NAMESPACE::ObjectDestroy<VULKAN_HPP_NAMESPACE::Device, Dispatch>( vk::Device{allocatorInfo.device} ) }, vma::UniqueAllocation{ allocation, ObjectFree<Allocator>(*this) });
      return createResultValue(result, pair, VMA_HPP_NAMESPACE_STRING"::Allocator::createImage");
  }
#endif /*VULKAN_HPP_NO_SMART_HANDLE*/


  VULKAN_HPP_INLINE void Allocator::createLostAllocation( Allocation* pAllocation ) const
  {
    ::vmaCreateLostAllocation( m_allocator, reinterpret_cast<VmaAllocation*>( pAllocation ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE Allocation Allocator::createLostAllocation() const
  {
    Allocation allocation;
    ::vmaCreateLostAllocation( m_allocator, reinterpret_cast<VmaAllocation*>( &allocation ) );
    return allocation;
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::createPool( const PoolCreateInfo* pCreateInfo, Pool* pPool ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCreatePool( m_allocator, reinterpret_cast<const VmaPoolCreateInfo*>( pCreateInfo ), reinterpret_cast<VmaPool*>( pPool ) ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<Pool>::type Allocator::createPool( const PoolCreateInfo & createInfo ) const
  {
    vma::Pool pool;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaCreatePool( m_allocator, reinterpret_cast<const VmaPoolCreateInfo*>( &createInfo ), reinterpret_cast<VmaPool*>( &pool ) ) );
    return createResultValue( result, pool, VMA_HPP_NAMESPACE_STRING"::Allocator::createPool" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifndef VULKAN_HPP_NO_SMART_HANDLE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<UniqueHandle<Pool>>::type Allocator::createPoolUnique(const PoolCreateInfo& createInfo) const
  {
      vma::Pool pool;
      VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>(::vmaCreatePool(m_allocator, reinterpret_cast<const VmaPoolCreateInfo*>(&createInfo), reinterpret_cast<VmaPool*>(&pool)));
      vma::UniquePool handle{ pool, ObjectDestroy<Allocator>(*this) };
      return createResultValue(result, handle, VMA_HPP_NAMESPACE_STRING"::Allocator::createPoolUnique");
  }
#endif /*VULKAN_HPP_NO_SMART_HANDLE*/




  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::defragmentationBegin( const DefragmentationInfo2* pInfo, DefragmentationStats* pStats, DefragmentationContext* pContext ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaDefragmentationBegin( m_allocator, reinterpret_cast<const VmaDefragmentationInfo2*>( pInfo ), reinterpret_cast<VmaDefragmentationStats*>( pStats ), reinterpret_cast<VmaDefragmentationContext*>( pContext ) ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<DefragmentationContext>::type Allocator::defragmentationBegin( const DefragmentationInfo2 & info, VULKAN_HPP_NAMESPACE::Optional<DefragmentationStats> stats ) const
  {
    DefragmentationContext context;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaDefragmentationBegin( m_allocator, reinterpret_cast<const VmaDefragmentationInfo2*>( &info ), reinterpret_cast<VmaDefragmentationStats*>( static_cast<DefragmentationStats*>( stats ) ), reinterpret_cast<VmaDefragmentationContext*>( &context ) ) );
    return createResultValue( result, context, VMA_HPP_NAMESPACE_STRING"::Allocator::defragmentationBegin" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::defragmentationEnd( DefragmentationContext context ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaDefragmentationEnd( m_allocator, static_cast<VmaDefragmentationContext>( context ) ) );
  }
#else
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<void>::type Allocator::defragmentationEnd( DefragmentationContext context ) const
  {
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaDefragmentationEnd( m_allocator, static_cast<VmaDefragmentationContext>( context ) ) );
    return createResultValue( result, VMA_HPP_NAMESPACE_STRING"::Allocator::defragmentationEnd" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::destroy() const
  {
    ::vmaDestroyAllocator( m_allocator );
  }
#else
  VULKAN_HPP_INLINE void Allocator::destroy() const
  {
    ::vmaDestroyAllocator( m_allocator );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::destroyBuffer( VULKAN_HPP_NAMESPACE::Buffer buffer, Allocation allocation ) const
  {
    ::vmaDestroyBuffer( m_allocator, static_cast<VkBuffer>( buffer ), static_cast<VmaAllocation>( allocation ) );
  }
#else
  VULKAN_HPP_INLINE void Allocator::destroyBuffer( VULKAN_HPP_NAMESPACE::Buffer buffer, Allocation allocation ) const
  {
    ::vmaDestroyBuffer( m_allocator, static_cast<VkBuffer>( buffer ), static_cast<VmaAllocation>( allocation ) );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::destroyImage( VULKAN_HPP_NAMESPACE::Image image, Allocation allocation ) const
  {
    ::vmaDestroyImage( m_allocator, static_cast<VkImage>( image ), static_cast<VmaAllocation>( allocation ) );
  }
#else
  VULKAN_HPP_INLINE void Allocator::destroyImage( VULKAN_HPP_NAMESPACE::Image image, Allocation allocation ) const
  {
    ::vmaDestroyImage( m_allocator, static_cast<VkImage>( image ), static_cast<VmaAllocation>( allocation ) );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::destroyPool( Pool pool ) const
  {
    ::vmaDestroyPool( m_allocator, static_cast<VmaPool>( pool ) );
  }
#else
  VULKAN_HPP_INLINE void Allocator::destroyPool( Pool pool ) const
  {
    ::vmaDestroyPool( m_allocator, static_cast<VmaPool>( pool ) );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::findMemoryTypeIndex( uint32_t memoryTypeBits, const AllocationCreateInfo* pAllocationCreateInfo, uint32_t* pMemoryTypeIndex ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaFindMemoryTypeIndex( m_allocator, memoryTypeBits, reinterpret_cast<const VmaAllocationCreateInfo*>( pAllocationCreateInfo ), pMemoryTypeIndex ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<uint32_t>::type Allocator::findMemoryTypeIndex( uint32_t memoryTypeBits, const AllocationCreateInfo & allocationCreateInfo ) const
  {
    uint32_t memoryTypeIndex;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaFindMemoryTypeIndex( m_allocator, memoryTypeBits, reinterpret_cast<const VmaAllocationCreateInfo*>( &allocationCreateInfo ), &memoryTypeIndex ) );
    return createResultValue( result, memoryTypeIndex, VMA_HPP_NAMESPACE_STRING"::Allocator::findMemoryTypeIndex" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::findMemoryTypeIndexForBufferInfo( const VULKAN_HPP_NAMESPACE::BufferCreateInfo* pBufferCreateInfo, const AllocationCreateInfo* pAllocationCreateInfo, uint32_t* pMemoryTypeIndex ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaFindMemoryTypeIndexForBufferInfo( m_allocator, reinterpret_cast<const VkBufferCreateInfo*>( pBufferCreateInfo ), reinterpret_cast<const VmaAllocationCreateInfo*>( pAllocationCreateInfo ), pMemoryTypeIndex ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<uint32_t>::type Allocator::findMemoryTypeIndexForBufferInfo( const VULKAN_HPP_NAMESPACE::BufferCreateInfo & bufferCreateInfo, const AllocationCreateInfo & allocationCreateInfo ) const
  {
    uint32_t memoryTypeIndex;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaFindMemoryTypeIndexForBufferInfo( m_allocator, reinterpret_cast<const VkBufferCreateInfo*>( &bufferCreateInfo ), reinterpret_cast<const VmaAllocationCreateInfo*>( &allocationCreateInfo ), &memoryTypeIndex ) );
    return createResultValue( result, memoryTypeIndex, VMA_HPP_NAMESPACE_STRING"::Allocator::findMemoryTypeIndexForBufferInfo" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::findMemoryTypeIndexForImageInfo( const VULKAN_HPP_NAMESPACE::ImageCreateInfo* pImageCreateInfo, const AllocationCreateInfo* pAllocationCreateInfo, uint32_t* pMemoryTypeIndex ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaFindMemoryTypeIndexForImageInfo( m_allocator, reinterpret_cast<const VkImageCreateInfo*>( pImageCreateInfo ), reinterpret_cast<const VmaAllocationCreateInfo*>( pAllocationCreateInfo ), pMemoryTypeIndex ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<uint32_t>::type Allocator::findMemoryTypeIndexForImageInfo( const VULKAN_HPP_NAMESPACE::ImageCreateInfo & imageCreateInfo, const AllocationCreateInfo & allocationCreateInfo ) const
  {
    uint32_t memoryTypeIndex;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaFindMemoryTypeIndexForImageInfo( m_allocator, reinterpret_cast<const VkImageCreateInfo*>( &imageCreateInfo ), reinterpret_cast<const VmaAllocationCreateInfo*>( &allocationCreateInfo ), &memoryTypeIndex ) );
    return createResultValue( result, memoryTypeIndex, VMA_HPP_NAMESPACE_STRING"::Allocator::findMemoryTypeIndexForImageInfo" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::flushAllocation( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize offset, VULKAN_HPP_NAMESPACE::DeviceSize size ) const
  {
    ::vmaFlushAllocation( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkDeviceSize>( offset ), static_cast<VkDeviceSize>( size ) );
  }
#else
  VULKAN_HPP_INLINE void Allocator::flushAllocation( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize offset, VULKAN_HPP_NAMESPACE::DeviceSize size ) const
  {
    ::vmaFlushAllocation( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkDeviceSize>( offset ), static_cast<VkDeviceSize>( size ) );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::freeMemory( Allocation allocation ) const
  {
    ::vmaFreeMemory( m_allocator, static_cast<VmaAllocation>( allocation ) );
  }
#else
  VULKAN_HPP_INLINE void Allocator::freeMemory( Allocation allocation ) const
  {
    ::vmaFreeMemory( m_allocator, static_cast<VmaAllocation>( allocation ) );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE void Allocator::freeMemoryPages( size_t allocationCount, Allocation* pAllocations ) const
  {
    ::vmaFreeMemoryPages( m_allocator, allocationCount, reinterpret_cast<VmaAllocation*>( pAllocations ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::freeMemoryPages( VULKAN_HPP_NAMESPACE::ArrayProxy<Allocation> allocations ) const
  {
    ::vmaFreeMemoryPages( m_allocator, allocations.size() , reinterpret_cast<VmaAllocation*>( allocations.data() ) );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/


  VULKAN_HPP_INLINE void Allocator::free(Allocation allocation) const
  {
      this->freeMemory(allocation);
  }

  VULKAN_HPP_INLINE void Allocator::getAllocationInfo( Allocation allocation, AllocationInfo* pAllocationInfo ) const
  {
    ::vmaGetAllocationInfo( m_allocator, static_cast<VmaAllocation>( allocation ), reinterpret_cast<VmaAllocationInfo*>( pAllocationInfo ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE AllocationInfo Allocator::getAllocationInfo( Allocation allocation ) const
  {
    AllocationInfo allocationInfo;
    ::vmaGetAllocationInfo( m_allocator, static_cast<VmaAllocation>( allocation ), reinterpret_cast<VmaAllocationInfo*>( &allocationInfo ) );
    return allocationInfo;
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE void Allocator::getMemoryTypeProperties( uint32_t memoryTypeIndex, VULKAN_HPP_NAMESPACE::MemoryPropertyFlags* pFlags ) const
  {
    ::vmaGetMemoryTypeProperties( m_allocator, memoryTypeIndex, reinterpret_cast<VkMemoryPropertyFlags*>( pFlags ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::MemoryPropertyFlags Allocator::getMemoryTypeProperties( uint32_t memoryTypeIndex ) const
  {
    VULKAN_HPP_NAMESPACE::MemoryPropertyFlags flags;
    ::vmaGetMemoryTypeProperties( m_allocator, memoryTypeIndex, reinterpret_cast<VkMemoryPropertyFlags*>( &flags ) );
    return flags;
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE void Allocator::getPoolStats( Pool pool, PoolStats* pPoolStats ) const
  {
    ::vmaGetPoolStats( m_allocator, static_cast<VmaPool>( pool ), reinterpret_cast<VmaPoolStats*>( pPoolStats ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE PoolStats Allocator::getPoolStats( Pool pool ) const
  {
    PoolStats poolStats;
    ::vmaGetPoolStats( m_allocator, static_cast<VmaPool>( pool ), reinterpret_cast<VmaPoolStats*>( &poolStats ) );
    return poolStats;
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::invalidateAllocation( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize offset, VULKAN_HPP_NAMESPACE::DeviceSize size ) const
  {
    ::vmaInvalidateAllocation( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkDeviceSize>( offset ), static_cast<VkDeviceSize>( size ) );
  }
#else
  VULKAN_HPP_INLINE void Allocator::invalidateAllocation( Allocation allocation, VULKAN_HPP_NAMESPACE::DeviceSize offset, VULKAN_HPP_NAMESPACE::DeviceSize size ) const
  {
    ::vmaInvalidateAllocation( m_allocator, static_cast<VmaAllocation>( allocation ), static_cast<VkDeviceSize>( offset ), static_cast<VkDeviceSize>( size ) );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE void Allocator::makePoolAllocationsLost( Pool pool, size_t* pLostAllocationCount ) const
  {
    ::vmaMakePoolAllocationsLost( m_allocator, static_cast<VmaPool>( pool ), pLostAllocationCount );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE size_t Allocator::makePoolAllocationsLost( Pool pool ) const
  {
    size_t lostAllocationCount;
    ::vmaMakePoolAllocationsLost( m_allocator, static_cast<VmaPool>( pool ), &lostAllocationCount );
    return lostAllocationCount;
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Result Allocator::mapMemory( Allocation allocation, void** ppData ) const
  {
    return static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaMapMemory( m_allocator, static_cast<VmaAllocation>( allocation ), ppData ) );
  }
#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::ResultValueType<void*>::type Allocator::mapMemory( Allocation allocation ) const
  {
    void* pData;
    VULKAN_HPP_NAMESPACE::Result result = static_cast<VULKAN_HPP_NAMESPACE::Result>( ::vmaMapMemory( m_allocator, static_cast<VmaAllocation>( allocation ), &pData ) );
    return createResultValue( result, pData, VMA_HPP_NAMESPACE_STRING"::Allocator::mapMemory" );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::setAllocationUserData( Allocation allocation, void* pUserData ) const
  {
    ::vmaSetAllocationUserData( m_allocator, static_cast<VmaAllocation>( allocation ), pUserData );
  }
#else
  VULKAN_HPP_INLINE void Allocator::setAllocationUserData( Allocation allocation, void* pUserData ) const
  {
    ::vmaSetAllocationUserData( m_allocator, static_cast<VmaAllocation>( allocation ), pUserData );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::setCurrentFrameIndex( uint32_t frameIndex ) const
  {
    ::vmaSetCurrentFrameIndex( m_allocator, frameIndex );
  }
#else
  VULKAN_HPP_INLINE void Allocator::setCurrentFrameIndex( uint32_t frameIndex ) const
  {
    ::vmaSetCurrentFrameIndex( m_allocator, frameIndex );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Bool32 Allocator::touchAllocation( Allocation allocation ) const
  {
    return static_cast<Bool32>( ::vmaTouchAllocation( m_allocator, static_cast<VmaAllocation>( allocation ) ) );
  }
#else
  VULKAN_HPP_INLINE VULKAN_HPP_NAMESPACE::Bool32 Allocator::touchAllocation( Allocation allocation ) const
  {
    return ::vmaTouchAllocation( m_allocator, static_cast<VmaAllocation>( allocation ) );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/

#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE
  VULKAN_HPP_INLINE void Allocator::unmapMemory( Allocation allocation ) const
  {
    ::vmaUnmapMemory( m_allocator, static_cast<VmaAllocation>( allocation ) );
  }
#else
  VULKAN_HPP_INLINE void Allocator::unmapMemory( Allocation allocation ) const
  {
    ::vmaUnmapMemory( m_allocator, static_cast<VmaAllocation>( allocation ) );
  }
#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/
}

namespace VULKAN_HPP_NAMESPACE {
  template <> struct FlagTraits<VMA_HPP_NAMESPACE::AllocationCreateFlagBits> {
    enum
    {
      allFlags = VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eDedicatedMemory) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eNeverAllocate) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eMapped) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eCanBecomeLost) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eCanMakeOtherLost) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eUserDataCopyString) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eUpperAddress) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eDontBind)  | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eWithinBudget)  | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eStrategyBestFit) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eStrategyWorstFit) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eStrategyFirstFit) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eStrategyMinMemory) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eStrategyMinTime) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eStrategyMinFragmentation) | VkFlags(VMA_HPP_NAMESPACE::AllocationCreateFlagBits::eStrategyMask)
    };
  };

  template <> struct FlagTraits<VMA_HPP_NAMESPACE::AllocatorCreateFlagBits> {
    enum
    {
      allFlags = VkFlags(VMA_HPP_NAMESPACE::AllocatorCreateFlagBits::eExternallySynchronized) | VkFlags(VMA_HPP_NAMESPACE::AllocatorCreateFlagBits::eKhrDedicatedAllocation) | VkFlags(VMA_HPP_NAMESPACE::AllocatorCreateFlagBits::eKhrBindMemory2) | VkFlags(VMA_HPP_NAMESPACE::AllocatorCreateFlagBits::eExtMemoryBudget )
    };
  };

  template <> struct FlagTraits<VMA_HPP_NAMESPACE::PoolCreateFlagBits> {
    enum
    {
      allFlags = VkFlags(VMA_HPP_NAMESPACE::PoolCreateFlagBits::eIgnoreBufferImageGranularity) | VkFlags(VMA_HPP_NAMESPACE::PoolCreateFlagBits::eLinearAlgorithm) | VkFlags(VMA_HPP_NAMESPACE::PoolCreateFlagBits::eBuddyAlgorithm) | VkFlags(VMA_HPP_NAMESPACE::PoolCreateFlagBits::eAlgorithmMask)
    };
  };

  template <> struct FlagTraits<VMA_HPP_NAMESPACE::RecordFlagBits> {
    enum
    {
      allFlags = VkFlags(VMA_HPP_NAMESPACE::RecordFlagBits::eFlushAfterCall)
    };
  };
}

#endif
