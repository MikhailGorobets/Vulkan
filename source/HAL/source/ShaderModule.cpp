#include "../include/ShaderModule.hpp"
#include "../include/DeviceImpl.hpp"

namespace HAL {

    ShaderModule::ShaderModule(HAL::Device const& device, ShaderBytecode const& code) {
        spirv_cross::CompilerHLSL compiler(reinterpret_cast<const uint32_t*>(code.pData), code.Size / 4);
        m_EntryPoint = compiler.get_entry_points_and_stages()[0].name;
        m_ShaderStage = *GetShaderStage(compiler.get_execution_model());
        m_DescriptorsSets = this->ReflectPipelineResources(compiler);
        m_pShaderModule = device.GetVkDevice().createShaderModuleUnique({ .codeSize = static_cast<uint32_t>(code.Size), .pCode = reinterpret_cast<uint32_t*>(code.pData) });
    }

    auto ShaderModule::GetShaderStage(spv::ExecutionModel executionModel) const -> std::optional<vk::ShaderStageFlagBits> {
        switch (executionModel) {
        case spv::ExecutionModelVertex:
            return vk::ShaderStageFlagBits::eVertex;
        case spv::ExecutionModelTessellationControl:
            return vk::ShaderStageFlagBits::eTessellationControl;
        case spv::ExecutionModelTessellationEvaluation:
            return vk::ShaderStageFlagBits::eTessellationEvaluation;
        case spv::ExecutionModelGeometry:
            return vk::ShaderStageFlagBits::eGeometry;
        case spv::ExecutionModelFragment:
            return vk::ShaderStageFlagBits::eFragment;
        case spv::ExecutionModelGLCompute:
            return vk::ShaderStageFlagBits::eCompute;
        }
        return {};
    }

    auto ShaderModule::ReflectPipelineResources(spirv_cross::CompilerHLSL const& compiler) -> StagePipelineResources {
        StagePipelineResources pipelineResources(32);

        auto ReflectPipelineResourcesForType = [&](vk::DescriptorType type, spirv_cross::SmallVector<spirv_cross::Resource> const& resources) -> void {
            for (auto const& resource : resources) {
                auto const& typeID = compiler.get_type(resource.type_id);

                uint32_t setID = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                uint32_t index = compiler.get_decoration(resource.id, spv::DecorationBinding);
                uint32_t count = typeID.array.empty() ? 1 : typeID.array[0];
                pipelineResources.emplace(setID, index, count, type, *GetShaderStage(compiler.get_execution_model()));
            }
        };

        spirv_cross::ShaderResources resources = compiler.get_shader_resources();
        ReflectPipelineResourcesForType(vk::DescriptorType::eUniformBuffer, resources.uniform_buffers);
        ReflectPipelineResourcesForType(vk::DescriptorType::eStorageBuffer, resources.storage_buffers);
        ReflectPipelineResourcesForType(vk::DescriptorType::eSampledImage, resources.separate_images);
        ReflectPipelineResourcesForType(vk::DescriptorType::eStorageImage, resources.storage_images);
        ReflectPipelineResourcesForType(vk::DescriptorType::eSampler, resources.separate_samplers);
        return pipelineResources;
    }
}


