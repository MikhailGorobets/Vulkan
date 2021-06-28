#include "../include/DescriptorTableLayoutImpl.hpp"

namespace HAL {

    DescriptorTableLayout::Internal::Internal(Device const& device, std::span<const PipelineResource> resources) {
        for (auto const& resource : resources)
            m_PipelineResources.emplace(resource.BindingID, resource);

        std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
        std::vector<vk::DescriptorBindingFlags> descriptorBindingFlags;

        for (auto const& resource : resources) {
            descriptorSetLayoutBindings.emplace_back(resource.BindingID, resource.DescriptorType, resource.DescriptorCount, resource.Stages);
            descriptorBindingFlags.push_back(resource.DescriptorCount > 0 ? vk::DescriptorBindingFlags{} : vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::ePartiallyBound);
        }
     
        vk::DescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlagsCI = {
            .bindingCount = static_cast<uint32_t>(std::size(descriptorBindingFlags)),
            .pBindingFlags = std::data(descriptorBindingFlags)
        };

        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {
            .pNext = &descriptorSetLayoutBindingFlagsCI,
            .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
            .pBindings = descriptorSetLayoutBindings.data()
        };
        m_pDescritptorSetLayout = device.GetVkDevice().createDescriptorSetLayoutUnique(descriptorSetLayoutCI);      
    }

    auto DescriptorTableLayout::Internal::GetPipelineResource(uint32_t slotID) const -> PipelineResource const& { return m_PipelineResources.at(slotID); }

    auto DescriptorTableLayout::Internal::GetVkDescriptorSetLayout() const -> vk::DescriptorSetLayout { return m_pDescritptorSetLayout.get(); }

}

namespace HAL {

    DescriptorTableLayout::DescriptorTableLayout(Device const& device, std::span<const PipelineResource> resources) : m_pInternal(device, resources) {}

    DescriptorTableLayout::DescriptorTableLayout(DescriptorTableLayout&&) = default;

    DescriptorTableLayout& DescriptorTableLayout::operator=(DescriptorTableLayout&&) = default;

    DescriptorTableLayout::~DescriptorTableLayout() = default;

    auto DescriptorTableLayout::GetPipelineResource(uint32_t slotID) const -> PipelineResource const& {
        return m_pInternal->GetPipelineResource(slotID);
    }

    auto DescriptorTableLayout::GetVkDescriptorSetLayout() const -> vk::DescriptorSetLayout {
        return m_pInternal->GetVkDescriptorSetLayout();
    }
}
