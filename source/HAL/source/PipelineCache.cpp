#include "..\include\PipelineCache.hpp"

namespace HAL {

    static auto CreateComputePipeline(vk::Device device, vk::PipelineCache cache, ComputePipeline const& pipeline, ComputeState const& state) -> vk::UniquePipeline {
        auto pImplPipeline = reinterpret_cast<const Pipeline*>(&pipeline);

        vk::ComputePipelineCreateInfo pipelineCI = {
            .stage = vk::PipelineShaderStageCreateInfo{
                .stage = pImplPipeline->GetShaderModule(0).GetShaderStage(),
                .module = pImplPipeline->GetShaderModule(0).GetShadeModule(),
                .pName = pImplPipeline->GetShaderModule(0).GetEntryPoint().c_str()
        },
            .layout = pImplPipeline->GetLayout()
        };
        auto [result, vkPipelines] = device.createComputePipelinesUnique(cache, {pipelineCI});
        return std::move(vkPipelines.front());
    }

    static auto CreateGraphicsPipeline(vk::Device device, vk::PipelineCache cache, GraphicsPipeline const& pipeline, RenderPass const& renderPass, GraphicsState const& state) -> vk::UniquePipeline {

        auto pImplPipeline = reinterpret_cast<const Pipeline*>(&pipeline);

        vk::PipelineViewportStateCreateInfo viewportStateCI = {
            .viewportCount = 1,
            .scissorCount = 1
        };

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = false
        };

        vk::PipelineVertexInputStateCreateInfo vertexInputStateCI = {
            .vertexBindingDescriptionCount = 0,
            .vertexAttributeDescriptionCount = 0
        };

        vk::PipelineColorBlendAttachmentState colorBlendAttachments[] = {
            vk::PipelineColorBlendAttachmentState{
                .blendEnable = false,
                .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
            }
        };

        vk::PipelineDepthStencilStateCreateInfo depthStencilStateCI = {
            .depthTestEnable = state.DepthStencilState.DepthEnable,
            .depthWriteEnable = state.DepthStencilState.DepthWrite,
            .depthCompareOp = static_cast<vk::CompareOp>(state.DepthStencilState.DepthFunc),
            .stencilTestEnable = false
        };

        vk::PipelineMultisampleStateCreateInfo multisampleStateCI = {
            .rasterizationSamples = vk::SampleCountFlagBits::e1
        };

        vk::PipelineColorBlendStateCreateInfo colorBlendStateCI = {
            .logicOpEnable = false,
            .attachmentCount = _countof(colorBlendAttachments),
            .pAttachments = colorBlendAttachments,
        };

        vk::PipelineRasterizationStateCreateInfo rasterizationStateCI = {
            .depthClampEnable = false,
            .rasterizerDiscardEnable = false,
            .polygonMode = static_cast<vk::PolygonMode>(state.RasterState.FillMode),
            .cullMode = static_cast<vk::CullModeFlagBits>(state.RasterState.CullMode),
            .frontFace = static_cast<vk::FrontFace>(state.RasterState.FrontFace),
            .lineWidth = 1.0f
        };

        vk::DynamicState dynamicStates[] = {
            vk::DynamicState::eScissor,
            vk::DynamicState::eViewport
        };

        vk::PipelineDynamicStateCreateInfo dynamicStateCI = {
            .dynamicStateCount = _countof(dynamicStates),
            .pDynamicStates = dynamicStates
        };


        vk::GraphicsPipelineCreateInfo pipelineCI = {
            //.stageCount = _countof(shaderStagesCI),
            // .pStages = shaderStagesCI,
            .pVertexInputState = &vertexInputStateCI,
            .pViewportState = &viewportStateCI,
            .pRasterizationState = &rasterizationStateCI,
            .pMultisampleState = &multisampleStateCI,
            .pDepthStencilState = &depthStencilStateCI,
            .pColorBlendState = &colorBlendStateCI,
            .pDynamicState = &dynamicStateCI,
            .layout = pImplPipeline->GetLayout(),
            .renderPass = renderPass.GetVkRenderPass(),
        };

        auto [result, vkPipelines] = device.createGraphicsPipelinesUnique(cache, {pipelineCI});
        return std::move(vkPipelines.front());
    }

    PipelineCache::PipelineCache(Device const& device, PipelineCacheCreateInfo const& createInfo) {
        
        //TODO
        std::unique_ptr<FILE, decltype(&std::fclose)> pFile(std::fopen("PipelineCache", "rb"), std::fclose);
        std::vector<uint8_t> cacheData{};

        if (pFile.get() != nullptr) {
            std::fseek(pFile.get(), 0, SEEK_END);
            size_t size = std::ftell(pFile.get());
            std::fseek(pFile.get(), 0, SEEK_SET);
            cacheData.resize(size);
            std::fread(std::data(cacheData), sizeof(uint8_t), size, pFile.get());
        }

        vk::PipelineCacheCreateInfo pipelineCacheCI = {
            .initialDataSize = std::size(cacheData),
            .pInitialData = std::data(cacheData)
        };

        m_pVkPipelineCache = device.GetVkDevice().createPipelineCacheUnique(pipelineCacheCI);
        vkx::setDebugName(device.GetVkDevice(), *m_pVkPipelineCache, "");
    }

    auto PipelineCache::GetComputePipeline(ComputePipeline const& pipeline, ComputeState const& state) const -> vk::Pipeline {
        auto pImplPipeline = reinterpret_cast<const Pipeline*>(&pipeline);

        vk::Pipeline result;

        ComputePipelineKey key = {.Stage = pImplPipeline->GetShaderModule(0).GetShadeModule()};
        if (auto it = m_ComputePipelineCache.find(key); it == m_ComputePipelineCache.end()) {
            auto vkPipeline = CreateComputePipeline(m_pVkPipelineCache.getOwner(), m_pVkPipelineCache.get(), pipeline, state);
            result = vkPipeline.get();
            m_ComputePipelineCache.emplace(key, std::move(vkPipeline));
        } else {
            result = it->second.get();
        }
        return result;
    }
}