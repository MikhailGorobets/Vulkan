#pragma once

#include <HAL/InternalPtr.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    
    class ComputePipeline: NonCopyable {
    public:
        class Internal;
    public:
        ComputePipeline(Device const& pDevice, ComputePipelineCreateInfo const& createInfo);
    
        ~ComputePipeline();
        
        auto GetDescriptorTableLayout(uint32_t tableID) const -> DescriptorTableLayout const&;
        
        auto GetVkPipelineLayout() const -> vk::PipelineLayout;

    private:
        InternalPtr<Internal, InternalSize_Pipeline> m_pInternal;
    };

    class GraphicsPipeline: NonCopyable {
    public:
        class Internal;
    public:
        GraphicsPipeline(Device const& pDevice, GraphicsPipelineCreateInfo const& createInfo);
           
        ~GraphicsPipeline();
        
        auto GetDescriptorTableLayout(uint32_t tableID) const -> DescriptorTableLayout const&;

    private:
        InternalPtr<Internal, InternalSize_Pipeline> m_pInternal;
    };
}