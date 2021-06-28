#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    class DescriptorTableLayout {
    public:
        class Internal;
    public:  
        DescriptorTableLayout(Device const& device, std::span<const PipelineResource> resources);
        
        DescriptorTableLayout(DescriptorTableLayout&&);

        DescriptorTableLayout& operator=(DescriptorTableLayout&&);

        ~DescriptorTableLayout();

        auto GetPipelineResource(uint32_t index) const -> PipelineResource const&;

        auto GetVkDescriptorSetLayout() const -> vk::DescriptorSetLayout;

    private:
        InternalPtr<Internal, InternalSize_DescriptorTableLayout> m_pInternal;
    };
}
