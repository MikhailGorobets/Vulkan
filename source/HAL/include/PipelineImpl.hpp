#pragma once

#include <HAL/Device.hpp>
#include <HAL/Pipeline.hpp>
#include <vulkan/vulkan_decl.h>
#include "ShaderModule.hpp"

namespace HAL {

    class Pipeline {
    public:
        Pipeline(Device const& device, HAL::ArrayView<ShaderBytecode> const& byteCodes, vk::PipelineBindPoint bindPoint);

        auto GetLayout() const -> vk::PipelineLayout { return *m_pPipelineLayout; }

        auto GetDescripiptorSetLayout(uint32_t slot) const->std::optional<vk::DescriptorSetLayout>;;

        auto GetBindPoint() const -> vk::PipelineBindPoint { return m_BindPoint; }

        auto GetShaderModule(uint32_t index) const -> ShaderModule const& { return m_ShaderModules[index]; };
        
    private:
        vk::UniquePipelineLayout                   m_pPipelineLayout;
        std::vector<vk::UniqueDescriptorSetLayout> m_DescriptorSetLaytouts;
        std::vector<std::vector<PipelineResource>> m_pPipelineResources;
        std::vector<ShaderModule>                  m_ShaderModules;
        vk::PipelineBindPoint                      m_BindPoint;
    };

    class ComputePipeline::Internal: public Pipeline {
    public:
        Internal(Device const& device, ComputePipelineCreateInfo const& createInfo): Pipeline(device, {createInfo.CS}, vk::PipelineBindPoint::eCompute) {}
    };

    class GraphicsPipeline::Internal: public Pipeline {
    public:
        Internal(Device const& device, GraphicsPipelineCreateInfo const& createInfo): Pipeline(device, {createInfo.VS, createInfo.PS}, vk::PipelineBindPoint::eGraphics) {}
    };
}
