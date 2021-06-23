#pragma once


#include <HAL/InternalPtr.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    class ComputePipeline : NonCopyable {
    public:
        class Internal;
    public:
        ComputePipeline(Device const& pDevice, ComputePipelineCreateInfo const& createInfo);

        ComputePipeline(ComputePipeline&&) noexcept;

        ComputePipeline& operator=(ComputePipeline&&) noexcept;

        ~ComputePipeline();

    private:
        InternalPtr<Internal, InternalSize_Pipeline> m_pInternal;
    };

    class GraphicsPipeline : NonCopyable {
    public:
        class Internal;
    public:
        GraphicsPipeline(Device const& pDevice, GraphicsPipelineCreateInfo const& createInfo);

        GraphicsPipeline(ComputePipeline&&) noexcept;

        GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept;

        ~GraphicsPipeline();

    private:
        InternalPtr<Internal, InternalSize_Pipeline> m_pInternal;
    };

}