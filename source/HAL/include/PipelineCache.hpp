#pragma once


#include <HAL/Pipeline.hpp>
#include <vulkan/vulkan_decl.h>
#include "ShaderModule.hpp"
#include "PipelineImpl.hpp"


namespace HAL {

    class PipelineCache {
    private:
        struct GraphicsStateBitField {
            uint32_t FillMode;
            uint32_t CullMode;
            uint32_t FrontFace;
            uint32_t DepthTestEnable;
            uint32_t DepthWriteEnable;
            uint32_t DepthFunc;
            uint32_t SubpassIndex;
        };

        struct ColorBlendAttachmentBitField {
            uint32_t ColorWriteMask;
        };

        struct GraphicsPipelineKey {

        };

        struct ComputePipelineKey {
            vk::ShaderModule Stage;
            bool operator==(const ComputePipelineKey&) const = default;
        };

        struct GraphicsPipelineKeyHash {
            std::size_t operator()(GraphicsPipelineKey const& key) const noexcept {
                return 0;
            }
        };

        struct ComputePipelineKeyyHash {
            std::size_t operator()(ComputePipelineKey const& key) const noexcept {
                return 0;
            }
        };

        template<typename Key, typename Hash>
        using PipelinesCache = std::unordered_map<Key, vk::UniquePipeline, Hash>;

    public:
        PipelineCache(Device const& device);

        auto CreateComputePipeline(ComputePipeline const& pipeline, ComputeState const& state) const->vk::UniquePipeline;

        auto CreateGraphicsPipeline(GraphicsPipeline const& pipeline, RenderPass const& renderPass, GraphicsState const& state)->vk::UniquePipeline;

        auto GetComputePipeline(ComputePipeline const& pipeline, ComputeState const& state)->vk::Pipeline;

        auto GetGraphicsPipeline(GraphicsPipeline const& pipeline, RenderPass const& renderPass, GraphicsState const& state)->vk::UniquePipeline;

        auto Reset() -> void;

    private:
        std::reference_wrapper<const Device> m_Device;
        PipelinesCache<GraphicsPipelineKey, GraphicsPipelineKeyHash> m_GraphicsPipelineCache;
        PipelinesCache<ComputePipelineKey, ComputePipelineKeyyHash>  m_ComputePipelineCache;
    };

}