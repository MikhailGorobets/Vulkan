#include "..\interface\HAL\Pipeline.hpp"
#include "..\include\PipelineImpl.hpp"
#include "..\include\DescriptorTableLayoutImpl.hpp"

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

    static auto CreatePipelineLayout(Device const& device, std::vector<vk::DescriptorSetLayout> descriptorSetLayouts) -> vk::UniquePipelineLayout {
        vk::PipelineLayoutCreateInfo pipelineLayoutCI = {
            .setLayoutCount = static_cast<uint32_t>(std::size(descriptorSetLayouts)),
            .pSetLayouts = std::data(descriptorSetLayouts)
        };
        return device.GetVkDevice().createPipelineLayoutUnique(pipelineLayoutCI);
    }

    static auto SeparateResources(ShaderModule::StagePipelineResources resources) -> std::vector<std::vector<PipelineResource>> { 
        std::vector<std::vector<PipelineResource>> pipelineResources;
        for (auto const& resource : resources) {
            if (pipelineResources.size() <= resource.SetID)
                pipelineResources.resize(resource.SetID + 1ull);
            pipelineResources[resource.SetID].emplace_back(resource.SetID, resource.BindingID, resource.DescriptorCount, resource.DescriptorType, resource.Stages);
        }
        return pipelineResources;
    }

    Pipeline::Pipeline(Device const& device, HAL::ArrayView<ShaderBytecode> byteCodes, vk::PipelineBindPoint bindPoint) {
        for (auto const& code : byteCodes)
            m_ShaderModules.push_back(ShaderModule(device, code));

        ShaderModule::StagePipelineResources mergedPipelineResources = {};
        for (auto const& shaderModule : m_ShaderModules)
            mergedPipelineResources = MergePipelineResources(mergedPipelineResources, shaderModule.GetResources());

        for (auto const& separatedSet: SeparateResources(mergedPipelineResources))
            if (!separatedSet.empty())
                m_PipelineTables.emplace(separatedSet.front().SetID, DescriptorTableLayout(device, separatedSet));
        

        std::vector<vk::DescriptorSetLayout> layouts;
        for(auto const& [index, set] : m_PipelineTables)
            layouts.push_back(set.GetVkDescriptorSetLayout());

        m_pPipelineLayout = CreatePipelineLayout(device, layouts);
        m_BindPoint = bindPoint;
    }

    auto Pipeline::GetDescripiptorTable(uint32_t index) const -> DescriptorTableLayout const& {
        return m_PipelineTables.at(index);
    }

}

namespace HAL {

    ComputePipeline::ComputePipeline(Device const& device, ComputePipelineCreateInfo const& createInfo): m_pInternal(device, createInfo) {}

    ComputePipeline::~ComputePipeline() = default;

    auto ComputePipeline::GetDescriptorTableLayout(uint32_t tableID) const -> DescriptorTableLayout const& {
        return m_pInternal->GetDescripiptorTable(tableID);
    }

    GraphicsPipeline::GraphicsPipeline(Device const& device, GraphicsPipelineCreateInfo const& createInfo): m_pInternal(device, createInfo) {}

    GraphicsPipeline::~GraphicsPipeline() = default;
}