#pragma once

#include <HAL/Device.hpp>
#include <HAL/Pipeline.hpp>
#include <vulkan/vulkan_decl.h>
#include "ShaderModule.hpp"
#include "DescriptorTableLayoutImpl.hpp"

namespace HAL {


    class Pipeline {
    public:
       using DescriptorTableMap = std::unordered_map<uint32_t, DescriptorTableLayout>;
    public:
        Pipeline(Device const& device, HAL::ArrayView<ShaderBytecode> byteCodes, vk::PipelineBindPoint bindPoint);
        
        auto GetDescripiptorTable(uint32_t index) const -> DescriptorTableLayout const&;

        auto GetShaderModule(uint32_t index) const -> ShaderModule const& { return m_ShaderModules.at(index); };

        auto GetVkPiplineLayout() const -> vk::PipelineLayout { return *m_pPipelineLayout; }

        auto GetVkBindPoint() const -> vk::PipelineBindPoint { return m_BindPoint; }

    private:
        vk::UniquePipelineLayout  m_pPipelineLayout = {};
        DescriptorTableMap        m_PipelineTables = {};
        std::vector<ShaderModule> m_ShaderModules = {};
        vk::PipelineBindPoint     m_BindPoint = {};
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
