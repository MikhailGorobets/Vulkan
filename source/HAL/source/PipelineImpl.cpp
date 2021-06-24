#include "..\include\PipelineImpl.hpp"

namespace HAL {

    static auto MergePipelineResources(ShaderModule::StagePipelineResources const& resources0, ShaderModule::StagePipelineResources const& resources1) {
        ShaderModule::StagePipelineResources result(64);
        auto SubMergeResources = [](ShaderModule::StagePipelineResources const& iterable, ShaderModule::StagePipelineResources& out) -> void {
            for (auto& binding : iterable) {
                if (auto value = out.find(binding); value != std::end(out)) {
                    *(vk::ShaderStageFlags*)(&value->Stages) |= binding.Stages; //TODO
                } else {
                    out.insert(binding);
                }
            }
        };
        SubMergeResources(resources0, result);
        SubMergeResources(resources1, result);
        return result;
    }

    static auto CreateDescriptorSetLayouts(HAL::Device const& device, std::vector<std::vector<PipelineResource>> const& pipelineResources) {
        std::vector<vk::UniqueDescriptorSetLayout> setLayouts;

        for (auto const& bindings : pipelineResources) {
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
            std::vector<vk::DescriptorBindingFlags> descriptorBindingFlags;

            for (auto const& binding : bindings) {
                descriptorSetLayoutBindings.emplace_back(binding.BindingID, binding.DescriptorType, binding.DescriptorCount, binding.Stages);
                descriptorBindingFlags.push_back(binding.DescriptorCount > 0 ? vk::DescriptorBindingFlags{} : vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::ePartiallyBound);
            }

            if (std::size(descriptorSetLayoutBindings) > 0) {

                vk::DescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlagsCI = {
                    .bindingCount = static_cast<uint32_t>(std::size(descriptorBindingFlags)),
                    .pBindingFlags = std::data(descriptorBindingFlags)
                };

                vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {
                    .pNext = &descriptorSetLayoutBindingFlagsCI,
                    .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
                    .pBindings = descriptorSetLayoutBindings.data()
                };
                setLayouts.push_back(device.GetVkDevice().createDescriptorSetLayoutUnique(descriptorSetLayoutCI));
            }
        }
        return setLayouts;
    }

    static auto CreatePipelineLayout(Device const& device, std::vector<vk::UniqueDescriptorSetLayout> const& descriptorSetLayouts) -> vk::UniquePipelineLayout {
        auto rawDescriptorSetLayouts = vk::uniqueToRaw(descriptorSetLayouts);
        vk::PipelineLayoutCreateInfo pipelineLayoutCI = {
            .setLayoutCount = static_cast<uint32_t>(std::size(rawDescriptorSetLayouts)),
            .pSetLayouts = std::data(rawDescriptorSetLayouts)
        };
        return device.GetVkDevice().createPipelineLayoutUnique(pipelineLayoutCI);
    }

    static auto CreateSetBindings(ShaderModule::StagePipelineResources resources) -> std::vector<std::vector<PipelineResource>> { 
        std::vector<std::vector<PipelineResource>> pipelineResources;
        for (auto const& resource : resources) {
            if (pipelineResources.size() <= resource.SetID)
                pipelineResources.resize(resource.SetID + 1ull);
            pipelineResources[resource.SetID].emplace_back(resource.SetID, resource.BindingID, resource.DescriptorCount, resource.DescriptorType, resource.Stages);
        }
        return pipelineResources;
    }

    Pipeline::Pipeline(Device const& device, HAL::ArrayProxy<ShaderBytecode> const& byteCodes, vk::PipelineBindPoint bindPoint) {
        for (auto const& code : byteCodes)
            m_ShaderModules.push_back(ShaderModule(device, code));

        ShaderModule::StagePipelineResources mergedPipelineResources = {};
        for (auto const& shaderModule : m_ShaderModules)
            mergedPipelineResources = MergePipelineResources(mergedPipelineResources, shaderModule.GetResources());

        m_pPipelineResources = CreateSetBindings(mergedPipelineResources);
        m_DescriptorSetLaytouts = CreateDescriptorSetLayouts(device, m_pPipelineResources);
        m_pPipelineLayout = CreatePipelineLayout(device, m_DescriptorSetLaytouts);
        m_BindPoint = bindPoint;
    }

    auto Pipeline::GetDescripiptorSetLayout(uint32_t slot) const -> std::optional<vk::DescriptorSetLayout> {
        return slot < std::size(m_DescriptorSetLaytouts) ? m_DescriptorSetLaytouts[slot].get() : std::optional<vk::DescriptorSetLayout>{};
    }
}

namespace HAL {

    ComputePipeline::ComputePipeline(Device const& device, ComputePipelineCreateInfo const& createInfo): m_pInternal(device, createInfo) {}

    ComputePipeline::~ComputePipeline() = default;

    GraphicsPipeline::GraphicsPipeline(Device const& device, GraphicsPipelineCreateInfo const& createInfo): m_pInternal(device, createInfo) {}

    GraphicsPipeline::~GraphicsPipeline() = default;
}