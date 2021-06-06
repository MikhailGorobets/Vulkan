#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    class CommandAllocator {
    public:
        class Internal;
    protected:
        CommandAllocator(Device const& device);

        auto GetVkCommandPool() const -> vk::CommandPool;
    private:     
        Internal_Ptr<Internal, InternalSize_CommandAllocator> m_pInternal;   
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
