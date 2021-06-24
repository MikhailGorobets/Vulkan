#pragma once

#include <HAL/RenderPass.hpp>
#include <HAL/CommandList.hpp>
#include <vulkan/vulkan_decl.h>
#include <spirv_hlsl.hpp>

namespace HAL {

    class ShaderModule {
    public:
        struct PipelineResourceHash {
            std::size_t operator()(PipelineResource const& key) const noexcept {
                std::size_t h1 = std::hash<uint32_t>{}(key.SetID);
                std::size_t h2 = std::hash<uint32_t>{}(key.BindingID);
                return h1 ^ (h2 << 1);
            }
        };

        struct PipelineResourceEqual {
            bool operator()(PipelineResource const& lhs, PipelineResource const& rhs) const noexcept {
                return lhs.SetID == rhs.SetID && lhs.BindingID == rhs.BindingID && lhs.DescriptorCount == rhs.DescriptorCount && lhs.DescriptorType == rhs.DescriptorType;
            }
        };

        using StagePipelineResources = std::unordered_set<PipelineResource, PipelineResourceHash, PipelineResourceEqual>;

    public:
        ShaderModule(Device const& device, ShaderBytecode const& code);

        auto GetShaderStage() const -> vk::ShaderStageFlagBits { return m_ShaderStage; }

        auto GetShadeModule() const -> vk::ShaderModule { return m_pShaderModule.get(); }

        auto GetEntryPoint() const -> std::string const& { return m_EntryPoint; }

        auto GetResources() const -> StagePipelineResources const& { return m_DescriptorsSets; }

    private:
        auto GetShaderStage(spv::ExecutionModel executionModel) const -> std::optional<vk::ShaderStageFlagBits>;

        auto ReflectPipelineResources(spirv_cross::CompilerHLSL const& compiler) -> StagePipelineResources;

    private:
        vk::UniqueShaderModule  m_pShaderModule;
        StagePipelineResources  m_DescriptorsSets;
        vk::ShaderStageFlagBits m_ShaderStage;
        std::string             m_EntryPoint;
    };
}