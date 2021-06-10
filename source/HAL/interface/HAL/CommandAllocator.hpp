#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    class CommandAllocator: NonCopyable {
    public:
        class Internal;
    protected:
        CommandAllocator(Device const& device, uint32_t indexQueueFamily);
    public:
        CommandAllocator(CommandAllocator&&) noexcept;

        CommandAllocator& operator=(CommandAllocator&&) noexcept;
 
        ~CommandAllocator();  
    
        auto GetVkCommandPool() const -> vk::CommandPool;

    protected:     
        InternalPtr<Internal, InternalSize_CommandAllocator> m_pInternal;   
    };
    
    class TransferCommandAllocator: public CommandAllocator {
    public:
        TransferCommandAllocator(Device const& device);       
    };

    class ComputeCommandAllocator: public CommandAllocator {
    public:
        ComputeCommandAllocator(Device const& device);
    };

    class GraphicsCommandAllocator: public CommandAllocator {
    public:
        GraphicsCommandAllocator(Device const& device);
    };
}
