#include <vulkan/vulkan_decl.h>
#include <imgui/imgui_impl_vulkan.h>

#include <dxc/dxcapi.use.h>
#include <fmt/core.h>
#include <fmt/format.h>

static constexpr uint32_t VertexShader_SPIRV[] = {
    0x07230203, 0x00010300, 0x000e0000, 0x0000002a, 0x00000000, 0x00020011, 0x00000001, 0x00020011,
    0x00000009, 0x00020011, 0x00001154, 0x0009000a, 0x5f565053, 0x474f4f47, 0x685f454c, 0x5f6c736c,
    0x636e7566, 0x6e6f6974, 0x74696c61, 0x00003179, 0x0007000a, 0x5f565053, 0x474f4f47, 0x755f454c,
    0x5f726573, 0x65707974, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x000b000f, 0x00000000,
    0x00000001, 0x614d5356, 0x00006e69, 0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000006,
    0x00000007, 0x00030003, 0x00000005, 0x0000028a, 0x000a0005, 0x00000008, 0x65707974, 0x7375502e,
    0x6e6f4368, 0x6e617473, 0x6d492e74, 0x50697547, 0x6d617261, 0x00000000, 0x00050006, 0x00000008,
    0x00000000, 0x6c616353, 0x00000065, 0x00060006, 0x00000008, 0x00000001, 0x6e617254, 0x74616c73,
    0x00000065, 0x00060005, 0x00000009, 0x68737550, 0x736e6f43, 0x746e6174, 0x00000073, 0x00060005,
    0x00000002, 0x762e6e69, 0x502e7261, 0x5449534f, 0x004e4f49, 0x00060005, 0x00000003, 0x762e6e69,
    0x542e7261, 0x4f435845, 0x0044524f, 0x00060005, 0x00000004, 0x762e6e69, 0x432e7261, 0x524f4c4f,
    0x00000000, 0x00070005, 0x00000005, 0x2e74756f, 0x2e726176, 0x43584554, 0x44524f4f, 0x00000000,
    0x00060005, 0x00000006, 0x2e74756f, 0x2e726176, 0x4f4c4f43, 0x00000052, 0x00040005, 0x00000001,
    0x614d5356, 0x00006e69, 0x00061600, 0x00000002, 0x00001603, 0x49534f50, 0x4e4f4954, 0x00000000,
    0x00061600, 0x00000003, 0x00001603, 0x43584554, 0x44524f4f, 0x00000000, 0x00051600, 0x00000004,
    0x00001603, 0x4f4c4f43, 0x00000052, 0x00061600, 0x00000005, 0x00001603, 0x43584554, 0x44524f4f,
    0x00000000, 0x00051600, 0x00000006, 0x00001603, 0x4f4c4f43, 0x00000052, 0x00040047, 0x00000007,
    0x0000000b, 0x00000000, 0x00061600, 0x00000007, 0x00001603, 0x505f5653, 0x7469736f, 0x006e6f69,
    0x00040047, 0x00000002, 0x0000001e, 0x00000000, 0x00040047, 0x00000003, 0x0000001e, 0x00000001,
    0x00040047, 0x00000004, 0x0000001e, 0x00000002, 0x00040047, 0x00000005, 0x0000001e, 0x00000000,
    0x00040047, 0x00000006, 0x0000001e, 0x00000001, 0x00050048, 0x00000008, 0x00000000, 0x00000023,
    0x00000000, 0x00050048, 0x00000008, 0x00000001, 0x00000023, 0x00000008, 0x00030047, 0x00000008,
    0x00000002, 0x00040015, 0x0000000a, 0x00000020, 0x00000001, 0x0004002b, 0x0000000a, 0x0000000b,
    0x00000001, 0x0004002b, 0x0000000a, 0x0000000c, 0x00000000, 0x00030016, 0x0000000d, 0x00000020,
    0x0004002b, 0x0000000d, 0x0000000e, 0x00000000, 0x0004002b, 0x0000000d, 0x0000000f, 0x3f800000,
    0x00040017, 0x00000010, 0x0000000d, 0x00000002, 0x0004001e, 0x00000008, 0x00000010, 0x00000010,
    0x00040020, 0x00000011, 0x00000009, 0x00000008, 0x00040020, 0x00000012, 0x00000001, 0x00000010,
    0x00030016, 0x00000013, 0x00000010, 0x00040017, 0x00000014, 0x00000013, 0x00000004, 0x00040020,
    0x00000015, 0x00000001, 0x00000014, 0x00040020, 0x00000016, 0x00000003, 0x00000010, 0x00040020,
    0x00000017, 0x00000003, 0x00000014, 0x00040017, 0x00000018, 0x0000000d, 0x00000004, 0x00040020,
    0x00000019, 0x00000003, 0x00000018, 0x00020013, 0x0000001a, 0x00030021, 0x0000001b, 0x0000001a,
    0x00040020, 0x0000001c, 0x00000009, 0x00000010, 0x0004003b, 0x00000011, 0x00000009, 0x00000009,
    0x0004003b, 0x00000012, 0x00000002, 0x00000001, 0x0004003b, 0x00000012, 0x00000003, 0x00000001,
    0x0004003b, 0x00000015, 0x00000004, 0x00000001, 0x0004003b, 0x00000016, 0x00000005, 0x00000003,
    0x0004003b, 0x00000017, 0x00000006, 0x00000003, 0x0004003b, 0x00000019, 0x00000007, 0x00000003,
    0x00050036, 0x0000001a, 0x00000001, 0x00000000, 0x0000001b, 0x000200f8, 0x0000001d, 0x0004003d,
    0x00000010, 0x0000001e, 0x00000002, 0x0004003d, 0x00000010, 0x0000001f, 0x00000003, 0x0004003d,
    0x00000014, 0x00000020, 0x00000004, 0x00050041, 0x0000001c, 0x00000021, 0x00000009, 0x0000000c,
    0x0004003d, 0x00000010, 0x00000022, 0x00000021, 0x00050085, 0x00000010, 0x00000023, 0x0000001e,
    0x00000022, 0x00050041, 0x0000001c, 0x00000024, 0x00000009, 0x0000000b, 0x0004003d, 0x00000010,
    0x00000025, 0x00000024, 0x00050081, 0x00000010, 0x00000026, 0x00000023, 0x00000025, 0x00050051,
    0x0000000d, 0x00000027, 0x00000026, 0x00000000, 0x00050051, 0x0000000d, 0x00000028, 0x00000026,
    0x00000001, 0x00070050, 0x00000018, 0x00000029, 0x00000027, 0x00000028, 0x0000000e, 0x0000000f,
    0x0003003e, 0x00000005, 0x0000001f, 0x0003003e, 0x00000006, 0x00000020, 0x0003003e, 0x00000007,
    0x00000029, 0x000100fd, 0x00010038
};


static constexpr uint32_t FragmentShader_SPIRV[] = {
    0x07230203, 0x00010300, 0x000e0000, 0x0000003d, 0x00000000, 0x00020011, 0x00000001, 0x00020011,
    0x00000009, 0x00020011, 0x00001154, 0x0009000a, 0x5f565053, 0x474f4f47, 0x685f454c, 0x5f6c736c,
    0x636e7566, 0x6e6f6974, 0x74696c61, 0x00003179, 0x0007000a, 0x5f565053, 0x474f4f47, 0x755f454c,
    0x5f726573, 0x65707974, 0x00000000, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0008000f, 0x00000004, 0x00000002, 0x614d5350,
    0x00006e69, 0x00000003, 0x00000004, 0x00000005, 0x00030010, 0x00000002, 0x00000007, 0x00030003,
    0x00000005, 0x0000028a, 0x00060005, 0x00000006, 0x65707974, 0x2e64322e, 0x67616d69, 0x00000065,
    0x00060005, 0x00000007, 0x706d6153, 0x5464656c, 0x75747865, 0x00006572, 0x00060005, 0x00000008,
    0x65707974, 0x6d61732e, 0x72656c70, 0x00000000, 0x00060005, 0x00000009, 0x706d6153, 0x4c72656c,
    0x61656e69, 0x00000072, 0x00060005, 0x00000003, 0x762e6e69, 0x542e7261, 0x4f435845, 0x0044524f,
    0x00060005, 0x00000004, 0x762e6e69, 0x432e7261, 0x524f4c4f, 0x00000000, 0x00070005, 0x00000005,
    0x2e74756f, 0x2e726176, 0x545f5653, 0x65677261, 0x00003074, 0x00040005, 0x00000002, 0x614d5350,
    0x00006e69, 0x00070005, 0x0000000a, 0x65707974, 0x6d61732e, 0x64656c70, 0x616d692e, 0x00006567,
    0x00061600, 0x00000003, 0x00001603, 0x43584554, 0x44524f4f, 0x00000000, 0x00051600, 0x00000004,
    0x00001603, 0x4f4c4f43, 0x00000052, 0x00061600, 0x00000005, 0x00001603, 0x545f5653, 0x65677261,
    0x00003074, 0x00040047, 0x00000003, 0x0000001e, 0x00000000, 0x00040047, 0x00000004, 0x0000001e,
    0x00000001, 0x00040047, 0x00000005, 0x0000001e, 0x00000000, 0x00040047, 0x00000007, 0x00000022,
    0x00000000, 0x00040047, 0x00000007, 0x00000021, 0x00000000, 0x00040047, 0x00000009, 0x00000022,
    0x00000000, 0x00040047, 0x00000009, 0x00000021, 0x00000001, 0x00061600, 0x00000007, 0x00001604,
    0x74786574, 0x32657275, 0x00000064, 0x00030016, 0x0000000b, 0x00000010, 0x0004002b, 0x0000000b,
    0x0000000c, 0x00000400, 0x00040017, 0x0000000d, 0x0000000b, 0x00000003, 0x0006002c, 0x0000000d,
    0x0000000e, 0x0000000c, 0x0000000c, 0x0000000c, 0x0004002b, 0x0000000b, 0x0000000f, 0x0000292d,
    0x0006002c, 0x0000000d, 0x00000010, 0x0000000f, 0x0000000f, 0x0000000f, 0x0004002b, 0x0000000b,
    0x00000011, 0x00003c00, 0x0004002b, 0x0000000b, 0x00000012, 0x00003c38, 0x0004002b, 0x0000000b,
    0x00000013, 0x00002aac, 0x0006002c, 0x0000000d, 0x00000014, 0x00000013, 0x00000013, 0x00000013,
    0x0004002b, 0x0000000b, 0x00000015, 0x000040cd, 0x0006002c, 0x0000000d, 0x00000016, 0x00000015,
    0x00000015, 0x00000015, 0x0004002b, 0x0000000b, 0x00000017, 0x00004a76, 0x00030016, 0x00000018,
    0x00000020, 0x00040017, 0x00000019, 0x00000018, 0x00000002, 0x00090019, 0x00000006, 0x00000018,
    0x00000001, 0x00000002, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00040020, 0x0000001a,
    0x00000000, 0x00000006, 0x0002001a, 0x00000008, 0x00040020, 0x0000001b, 0x00000000, 0x00000008,
    0x00040020, 0x0000001c, 0x00000001, 0x00000019, 0x00040017, 0x0000001d, 0x0000000b, 0x00000004,
    0x00040020, 0x0000001e, 0x00000001, 0x0000001d, 0x00040020, 0x0000001f, 0x00000003, 0x0000001d,
    0x00020013, 0x00000020, 0x00030021, 0x00000021, 0x00000020, 0x0003001b, 0x0000000a, 0x00000006,
    0x00040017, 0x00000022, 0x00000018, 0x00000004, 0x00020014, 0x00000023, 0x00040017, 0x00000024,
    0x00000023, 0x00000003, 0x0004003b, 0x0000001a, 0x00000007, 0x00000000, 0x0004003b, 0x0000001b,
    0x00000009, 0x00000000, 0x0004003b, 0x0000001c, 0x00000003, 0x00000001, 0x0004003b, 0x0000001e,
    0x00000004, 0x00000001, 0x0004003b, 0x0000001f, 0x00000005, 0x00000003, 0x00050036, 0x00000020,
    0x00000002, 0x00000000, 0x00000021, 0x000200f8, 0x00000025, 0x0004003d, 0x00000019, 0x00000026,
    0x00000003, 0x0004003d, 0x0000001d, 0x00000027, 0x00000004, 0x0008004f, 0x0000000d, 0x00000028,
    0x00000027, 0x00000027, 0x00000000, 0x00000001, 0x00000002, 0x0007000c, 0x0000000d, 0x00000029,
    0x00000001, 0x00000028, 0x0000000e, 0x00000028, 0x000500ba, 0x00000024, 0x0000002a, 0x00000029,
    0x00000010, 0x00050088, 0x0000000b, 0x0000002b, 0x00000011, 0x00000012, 0x0005008e, 0x0000000d,
    0x0000002c, 0x00000029, 0x0000002b, 0x00050081, 0x0000000d, 0x0000002d, 0x0000002c, 0x00000014,
    0x0007000c, 0x0000000d, 0x0000002e, 0x00000001, 0x0000001a, 0x0000002d, 0x00000016, 0x00050088,
    0x0000000b, 0x0000002f, 0x00000011, 0x00000017, 0x0005008e, 0x0000000d, 0x00000030, 0x00000029,
    0x0000002f, 0x000600a9, 0x0000000d, 0x00000031, 0x0000002a, 0x0000002e, 0x00000030, 0x00050051,
    0x0000000b, 0x00000032, 0x00000027, 0x00000003, 0x00050051, 0x0000000b, 0x00000033, 0x00000031,
    0x00000000, 0x00050051, 0x0000000b, 0x00000034, 0x00000031, 0x00000001, 0x00050051, 0x0000000b,
    0x00000035, 0x00000031, 0x00000002, 0x00070050, 0x0000001d, 0x00000036, 0x00000033, 0x00000034,
    0x00000035, 0x00000032, 0x0004003d, 0x00000006, 0x00000037, 0x00000007, 0x0004003d, 0x00000008,
    0x00000038, 0x00000009, 0x00050056, 0x0000000a, 0x00000039, 0x00000037, 0x00000038, 0x00060057,
    0x00000022, 0x0000003a, 0x00000039, 0x00000026, 0x00000000, 0x00040073, 0x0000001d, 0x0000003b,
    0x0000003a, 0x00050085, 0x0000001d, 0x0000003c, 0x00000036, 0x0000003b, 0x0003003e, 0x00000005,
    0x0000003c, 0x000100fd, 0x00010038
};




struct ImGui_ImplVulkanDesc {
    vk::Device         Device;
    vk::PhysicalDevice Adapter;
    vk::RenderPass     RenderPass;
    vma::Allocator     Allocator;
    uint32_t           FrameInFlight;
};

class ImGui_ImplVulkan {
public:
    ImGui_ImplVulkan(ImGui_ImplVulkanDesc const& desc) : m_Desc(desc) {
        this->CreateDescriptorSetLayout(m_Desc.Device);
        this->CreatePipelineLayout(m_Desc.Device);
        this->CreatePipeline(m_Desc.Device, m_Desc.RenderPass);
        this->CreateDescriptorSet(m_Desc.Device);
        this->CreateFontsTexture(m_Desc.Device, m_Desc.Adapter, m_Desc.Allocator);
        this->CreateFontSampler(m_Desc.Device);
        this->WriteDescriptorSet(m_Desc.Device);
        m_PerFrame.resize(m_Desc.FrameInFlight);
    }

    auto DrawDataUpdate(uint32_t frameIndex) -> void {
        assert(frameIndex < std::size(m_PerFrame));
        ImDrawData* pImGuiDrawData = ImGui::GetDrawData();

        if (pImGuiDrawData->TotalVtxCount > 0) {
            const uint64_t vertexBufferSize = pImGuiDrawData->TotalVtxCount * sizeof(ImDrawVert);
            const uint64_t indexBufferSize = pImGuiDrawData->TotalIdxCount * sizeof(ImDrawIdx);
            PerFrame& frameDesc = m_PerFrame[frameIndex];

            if (!frameDesc.VertexBuffer.Buffer || (frameDesc.VertexBuffer.BufferInfo.size < vertexBufferSize)) {
                frameDesc.VertexBuffer.Buffer.reset();
                frameDesc.VertexBuffer.Allocation.reset();

                vk::BufferCreateInfo bufferCI = {
                    .size = vertexBufferSize,
                    .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                    .sharingMode = vk::SharingMode::eExclusive,
                };

                vma::AllocationCreateInfo allocationCI = {
                    .flags = vma::AllocationCreateFlagBits::eMapped,
                    .usage = vma::MemoryUsage::eCpuToGpu,
                };

                vma::AllocationInfo allocationInfo = {};
                auto [pBuffer, pAllocation] = m_Desc.Allocator.createBufferUnique(bufferCI, allocationCI, allocationInfo);
                vkx::setDebugName(m_Desc.Device, *pBuffer, fmt::format("VertexBuffer[{0}] ImGui", frameIndex));
                frameDesc.VertexBuffer.Allocation = std::move(pAllocation);
                frameDesc.VertexBuffer.AllocationInfo = allocationInfo;
                frameDesc.VertexBuffer.Buffer = std::move(pBuffer);
                frameDesc.VertexBuffer.BufferInfo = bufferCI;
            }

            if (!frameDesc.IndexBuffer.Buffer || (frameDesc.IndexBuffer.BufferInfo.size < indexBufferSize)) {
                frameDesc.IndexBuffer.Buffer.reset();
                frameDesc.IndexBuffer.Allocation.reset();

                vk::BufferCreateInfo bufferCI = {
                    .size = indexBufferSize,
                    .usage = vk::BufferUsageFlagBits::eIndexBuffer,
                    .sharingMode = vk::SharingMode::eExclusive,
                };

                vma::AllocationCreateInfo allocationCI = {
                    .flags = vma::AllocationCreateFlagBits::eMapped,
                    .usage = vma::MemoryUsage::eCpuToGpu,
                };

                vma::AllocationInfo allocationInfo = {};
                auto [pBuffer, pAllocation] = m_Desc.Allocator.createBufferUnique(bufferCI, allocationCI, allocationInfo);
                vkx::setDebugName(m_Desc.Device, *pBuffer, fmt::format("IndexBuffer[{0}] ImGui", frameIndex));
                frameDesc.IndexBuffer.Allocation = std::move(pAllocation);
                frameDesc.IndexBuffer.AllocationInfo = allocationInfo;
                frameDesc.IndexBuffer.Buffer = std::move(pBuffer);
                frameDesc.IndexBuffer.BufferInfo = bufferCI;
            }

            auto pVtxData = static_cast<ImDrawVert*>(frameDesc.VertexBuffer.AllocationInfo.pMappedData);
            auto pIdxData = static_cast<ImDrawIdx*>(frameDesc.IndexBuffer.AllocationInfo.pMappedData);

            for (int32_t index = 0; index < pImGuiDrawData->CmdListsCount; index++) {
                const ImDrawList* pCmdList = pImGuiDrawData->CmdLists[index];
                std::memcpy(pVtxData, pCmdList->VtxBuffer.Data, pCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
                std::memcpy(pIdxData, pCmdList->IdxBuffer.Data, pCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                pVtxData += pCmdList->VtxBuffer.Size;
                pIdxData += pCmdList->IdxBuffer.Size;
            }
        }

    }

    auto DrawFrame(vk::CommandBuffer commandBuffer, uint32_t frameIndex) -> void {
        assert(frameIndex < std::size(m_PerFrame));
        ImDrawData* pImGuiDrawData = ImGui::GetDrawData();

        const int32_t frameBufferWidth = static_cast<int32_t>(pImGuiDrawData->DisplaySize.x * pImGuiDrawData->FramebufferScale.x);
        const int32_t frameBufferHeight = static_cast<int32_t>(pImGuiDrawData->DisplaySize.y * pImGuiDrawData->FramebufferScale.y);

        if (frameBufferWidth > 0 && frameBufferHeight > 0) {
            int32_t vertexBufferOffset = 0;
            int32_t indexBufferOffset  = 0;
           
            this->SetupRenderState(pImGuiDrawData, commandBuffer, frameIndex, frameBufferWidth, frameBufferHeight);
            for (int32_t i = 0; i < pImGuiDrawData->CmdListsCount; i++) {
                const ImDrawList* pCmdList = pImGuiDrawData->CmdLists[i];
                for (int32_t j = 0; j < pCmdList->CmdBuffer.Size; j++) {
                    const ImDrawCmd* pCmd = &pCmdList->CmdBuffer[j];
                    if (pCmd->UserCallback != nullptr) {
                        if (pCmd->UserCallback == ImDrawCallback_ResetRenderState)
                            this->SetupRenderState(pImGuiDrawData, commandBuffer, frameIndex, frameBufferWidth, frameBufferHeight);
                        else
                            pCmd->UserCallback(pCmdList, pCmd);
                    }
                    else {
                        ImVec4 clipRect = {};
                        clipRect.x = (pCmd->ClipRect.x - pImGuiDrawData->DisplayPos.x) * pImGuiDrawData->FramebufferScale.x;
                        clipRect.y = (pCmd->ClipRect.y - pImGuiDrawData->DisplayPos.y) * pImGuiDrawData->FramebufferScale.y;
                        clipRect.z = (pCmd->ClipRect.z - pImGuiDrawData->DisplayPos.x) * pImGuiDrawData->FramebufferScale.x;
                        clipRect.w = (pCmd->ClipRect.w - pImGuiDrawData->DisplayPos.y) * pImGuiDrawData->FramebufferScale.y;

                        if (clipRect.x < frameBufferWidth && clipRect.y < frameBufferHeight && clipRect.z >= 0.0f && clipRect.w >= 0.0f) {
                            clipRect.x = std::max(clipRect.x, 0.0f);
                            clipRect.y = std::max(clipRect.y, 0.0f);
                            clipRect.z = std::max(clipRect.z - clipRect.x, 0.0f);
                            clipRect.w = std::max(clipRect.w - clipRect.y, 0.0f);

                            vk::Rect2D scissor = {
                                .offset = { static_cast<int32_t>(clipRect.x),  static_cast<int32_t>(clipRect.y) },
                                .extent = { static_cast<uint32_t>(clipRect.z), static_cast<uint32_t>(clipRect.w) }
                            };
                            commandBuffer.setScissor(0, { scissor });
                            commandBuffer.drawIndexed(pCmd->ElemCount, 1, pCmd->IdxOffset + indexBufferOffset, pCmd->VtxOffset + vertexBufferOffset, 0);
                        }
                    }
                }
                vertexBufferOffset += pCmdList->VtxBuffer.Size;
                indexBufferOffset += pCmdList->IdxBuffer.Size;
            }
        }
    }

private:
    auto CreateFontsTexture(vk::Device device, vk::PhysicalDevice adapter, vma::Allocator allocator) -> void {
        ImGuiIO& imGuiIO = ImGui::GetIO();

        int32_t  textureWidth = 0;
        int32_t  textureHeight = 0;
        uint8_t* pData = nullptr;
        imGuiIO.Fonts->GetTexDataAsRGBA32(&pData, &textureWidth, &textureHeight);

        {
            vk::ImageCreateInfo imageCI = {
                .imageType = vk::ImageType::e2D,
                .format = vk::Format::eR8G8B8A8Unorm,
                .extent = {.width = static_cast<uint32_t>(textureWidth), .height = static_cast<uint32_t>(textureHeight), .depth = 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = vk::SampleCountFlagBits::e1,
                .tiling = vk::ImageTiling::eOptimal,
                .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                .initialLayout = vk::ImageLayout::eUndefined
            };

            vma::AllocationCreateInfo allocationCI = {
                .usage = vma::MemoryUsage::eGpuOnly
            };

            std::tie(m_pImage, m_pImageAllocation) = allocator.createImageUnique(imageCI, allocationCI);
            vkx::setDebugName(device, *m_pImage, "Font ImGui");
        }

        {
            vk::ImageViewCreateInfo imageViewCI = {
                .image = *m_pImage,
                .viewType = vk::ImageViewType::e2D,
                .format = vk::Format::eR8G8B8A8Unorm,
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS,
                }
            };

            m_pImageView = device.createImageViewUnique(imageViewCI);
            vkx::setDebugName(device, *m_pImageView, "Font ImGui");
        }

        vk::UniqueBuffer pBuffer;
        vma::UniqueAllocation pBufferAllocation;
        {
            vk::BufferCreateInfo bufferCI = {
                .size = 4 * textureWidth * textureHeight * sizeof(uint8_t),
                .usage = vk::BufferUsageFlagBits::eTransferSrc
            };

            vma::AllocationCreateInfo allocationCI = {
                .flags = vma::AllocationCreateFlagBits::eMapped,
                .usage = vma::MemoryUsage::eCpuOnly
            };

            vma::AllocationInfo allocationInfo = {};
            std::tie(pBuffer, pBufferAllocation) = allocator.createBufferUnique(bufferCI, allocationCI, allocationInfo);
            vkx::setDebugName(device, *pBuffer, "StagingBuffer ImGui");
            std::memcpy(allocationInfo.pMappedData, pData, 4 * sizeof(uint8_t) * textureWidth * textureHeight);
        }

        auto queueFamilyProperties = adapter.getQueueFamilyProperties();

        uint32_t indexQueueFamily = vkx::getIndexQueueFamilyGraphicsInfo(queueFamilyProperties)->queueIndex;
        vk::UniqueCommandPool pCmdPool;
        vk::UniqueCommandBuffer pCmdBuffer;
        {
            vk::CommandPoolCreateInfo commandBufferPoolCreateInfo = {
                .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex = indexQueueFamily
            };
            pCmdPool = device.createCommandPoolUnique(commandBufferPoolCreateInfo);
            vkx::setDebugName(device, *pCmdPool, "Update ImGui");

            vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {
                .commandPool = *pCmdPool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1,
            };
            pCmdBuffer = std::move(device.allocateCommandBuffersUnique(commandBufferAllocateInfo).front());
            vkx::setDebugName(device, *pCmdBuffer, "Update ImGui");
        }

        vkx::ImageTransition imageTransitionTransfer = {
             .image = *m_pImage,
             .oldLayout = vk::ImageLayout::eUndefined,
             .newLayout = vk::ImageLayout::eTransferDstOptimal,    
             .srcStages = vk::PipelineStageFlagBits::eHost,
             .dstStages = vk::PipelineStageFlagBits::eTransfer,
             .enabledShaderStages = {},
             .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,.levelCount = VK_REMAINING_MIP_LEVELS, .layerCount = VK_REMAINING_ARRAY_LAYERS }
        };
        
        vkx::ImageTransition imageTransitionSampled = {
             .image = *m_pImage,
             .oldLayout = vk::ImageLayout::eTransferDstOptimal,
             .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,   
             .srcStages = vk::PipelineStageFlagBits::eTransfer,
             .dstStages = vk::PipelineStageFlagBits::eFragmentShader,
             .enabledShaderStages = {},
             .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = VK_REMAINING_MIP_LEVELS, .layerCount = VK_REMAINING_ARRAY_LAYERS }
        };

        vk::BufferImageCopy bufferCopyRegion = {
            .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .layerCount = 1},
            .imageExtent = {.width = static_cast<uint32_t>(textureWidth), .height = static_cast<uint32_t>(textureHeight), .depth = 1 }
        };

        vk::UniqueFence pFence = device.createFenceUnique(vk::FenceCreateInfo{});  
        pCmdBuffer->begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
        vkx::imageTransition(pCmdBuffer.get(), imageTransitionTransfer);
        pCmdBuffer->copyBufferToImage(*pBuffer, *m_pImage, vk::ImageLayout::eTransferDstOptimal, { bufferCopyRegion });
        vkx::imageTransition(pCmdBuffer.get(), imageTransitionSampled);
        pCmdBuffer->end();

        vk::Queue queue = device.getQueue(indexQueueFamily, 0);

        vk::SubmitInfo submitDesc = {
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = pCmdBuffer.getAddressOf(),
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };

        queue.submit({ submitDesc }, *pFence);
        std::ignore = device.waitForFences({ *pFence }, true, std::numeric_limits<uint64_t>::max());
    }

    auto CreateFontSampler(vk::Device device) -> void {
        vk::SamplerCreateInfo samplerCreateInfo = {
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eNearest,
            .addressModeU = vk::SamplerAddressMode::eRepeat,
            .addressModeV = vk::SamplerAddressMode::eRepeat,
            .addressModeW = vk::SamplerAddressMode::eRepeat
        };
        m_pSampler = device.createSamplerUnique(samplerCreateInfo);
        vkx::setDebugName(device, *m_pSampler, "Linear ImGui");
    }

    auto CreateDescriptorSetLayout(vk::Device device) -> void {
        vk::DescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
            vk::DescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            },
            vk::DescriptorSetLayoutBinding{
                .binding = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            }
        };

        vk::DescriptorSetLayoutCreateInfo descritporSetLayoutCreateInfo = {
              .bindingCount = _countof(descriptorSetLayoutBindings),
              .pBindings = descriptorSetLayoutBindings
        };
        m_pDescriptorSetLayout = device.createDescriptorSetLayoutUnique(descritporSetLayoutCreateInfo);
        vkx::setDebugName(device, *m_pDescriptorSetLayout, "ImGui");
    }

    auto CreatePipelineLayout(vk::Device device) -> void {
        vk::DescriptorSetLayout descriptorSetLayouts[] = {
            *m_pDescriptorSetLayout
        };

        vk::PushConstantRange pushConstantRange[] = {
            vk::PushConstantRange{.stageFlags = vk::ShaderStageFlagBits::eVertex, .offset = 0, .size = sizeof(ImVec4) }
        };

        vk::PipelineLayoutCreateInfo pipelineLayoutCI = {
            .setLayoutCount = _countof(descriptorSetLayouts),
            .pSetLayouts = descriptorSetLayouts,
            .pushConstantRangeCount = _countof(pushConstantRange),
            .pPushConstantRanges = pushConstantRange,
        };

        m_pGraphicsPipelineLayout = device.createPipelineLayoutUnique(pipelineLayoutCI);
        vkx::setDebugName(device, *m_pGraphicsPipelineLayout, "ImGui");
    }

    auto CreatePipeline(vk::Device device, vk::RenderPass renderPass) -> void {

        vk::UniqueShaderModule vs = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo{ .codeSize = sizeof(VertexShader_SPIRV),   .pCode = VertexShader_SPIRV});
        vk::UniqueShaderModule fs = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo{ .codeSize = sizeof(FragmentShader_SPIRV), .pCode = FragmentShader_SPIRV});
        vkx::setDebugName(device, *vs, "[VS] ImGui");
        vkx::setDebugName(device, *fs, "[FS] ImGui");

        vk::PipelineShaderStageCreateInfo shaderStagesCI[] = {
            vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eVertex,   .module = *vs, .pName = "VSMain" },
            vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = *fs, .pName = "PSMain" },
        };

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStageCI = {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = false
        };

        vk::VertexInputBindingDescription vertexInputBindings[] = {
            vk::VertexInputBindingDescription{.binding = 0, .stride = sizeof(ImDrawVert), .inputRate = vk::VertexInputRate::eVertex }
        };

        vk::VertexInputAttributeDescription vertexInputAttributes[] = {
            vk::VertexInputAttributeDescription{.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat,  .offset = offsetof(ImDrawVert, pos) },
            vk::VertexInputAttributeDescription{.location = 1, .binding = 0, .format = vk::Format::eR32G32Sfloat,  .offset = offsetof(ImDrawVert, uv)  },
            vk::VertexInputAttributeDescription{.location = 2, .binding = 0, .format = vk::Format::eR8G8B8A8Unorm, .offset = offsetof(ImDrawVert, col) }
        };

        vk::PipelineVertexInputStateCreateInfo vertexInputStateCI = {
            .vertexBindingDescriptionCount = _countof(vertexInputBindings),
            .pVertexBindingDescriptions = vertexInputBindings,
            .vertexAttributeDescriptionCount = _countof(vertexInputAttributes),
            .pVertexAttributeDescriptions = vertexInputAttributes
        };
   
        vk::PipelineDepthStencilStateCreateInfo depthStencilStateCI = {
            .depthTestEnable = false,
            .depthWriteEnable = false,
            .depthCompareOp = vk::CompareOp::eLessOrEqual,
            .stencilTestEnable = false
        };

        vk::PipelineMultisampleStateCreateInfo multisampleStateCI = {
            .rasterizationSamples = vk::SampleCountFlagBits::e1
        };

        vk::PipelineColorBlendAttachmentState colorBlendAttachmentStates[] = {
            vk::PipelineColorBlendAttachmentState{
                .blendEnable = true,
                .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
                .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
                .colorBlendOp = vk::BlendOp::eAdd,
                .srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
                .dstAlphaBlendFactor = vk::BlendFactor::eZero,
                .alphaBlendOp = vk::BlendOp::eAdd,
                .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
            }
        };

        vk::PipelineColorBlendStateCreateInfo colorBlendStateCI = {
            .logicOpEnable = false,
            .attachmentCount = _countof(colorBlendAttachmentStates),
            .pAttachments = colorBlendAttachmentStates,
        };

        vk::PipelineRasterizationStateCreateInfo rasterizationStateCI = {
            .depthClampEnable = false,
            .rasterizerDiscardEnable = false,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eNone,
            .frontFace = vk::FrontFace::eClockwise,
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

        vk::PipelineViewportStateCreateInfo viewportStateCI = {
            .viewportCount = 1,
            .scissorCount = 1
        };

        vk::GraphicsPipelineCreateInfo pipelineCI = {
            .stageCount = _countof(shaderStagesCI),
            .pStages = shaderStagesCI,
            .pVertexInputState = &vertexInputStateCI,
            .pInputAssemblyState = &inputAssemblyStageCI,
            .pViewportState = &viewportStateCI,
            .pRasterizationState = &rasterizationStateCI,
            .pMultisampleState = &multisampleStateCI,
            .pDepthStencilState = &depthStencilStateCI,
            .pColorBlendState = &colorBlendStateCI,
            .pDynamicState = &dynamicStateCI,
            .layout = *m_pGraphicsPipelineLayout,
            .renderPass = renderPass,
        };

        std::tie(std::ignore, m_pGraphicsPipeline) = device.createGraphicsPipelineUnique(nullptr, pipelineCI).asTuple();
        vkx::setDebugName(device, *m_pGraphicsPipeline, "ImGui");
    }

    auto CreateDescriptorSet(vk::Device device) -> void {
        vk::DescriptorPoolSize descriptorPoolSize[] = {
            vk::DescriptorPoolSize {
                .type = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1
            }
        };
        vk::DescriptorPoolCreateInfo descriptorPoolCI = {
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1,
            .poolSizeCount = _countof(descriptorPoolSize),
            .pPoolSizes = descriptorPoolSize
        };
        m_pDescriptorPool = device.createDescriptorPoolUnique(descriptorPoolCI);
        vkx::setDebugName(device, *m_pDescriptorPool, "ImGui");

        vk::DescriptorSetLayout descriptorSetLayouts[] = {
            *m_pDescriptorSetLayout
        };

        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo = {
            .descriptorPool = *m_pDescriptorPool,
            .descriptorSetCount = _countof(descriptorSetLayouts),
            .pSetLayouts = descriptorSetLayouts
        };
        m_pDescriptorSet = std::move(device.allocateDescriptorSetsUnique(descriptorSetAllocateInfo).front());
        vkx::setDebugName(device, *m_pDescriptorSet, "ImGui");
    }

    auto WriteDescriptorSet(vk::Device device) -> void {
        vk::DescriptorImageInfo descriptorImageInfo = {
            .imageView = *m_pImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::DescriptorImageInfo descriptorSamplerInfo = {
            .sampler = *m_pSampler
        };

        vk::WriteDescriptorSet writeImageDescriptorSet = {
            .dstSet = *m_pDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .pImageInfo = &descriptorImageInfo,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        };

        vk::WriteDescriptorSet writeSamplerDescriptorSet = {
            .dstSet = *m_pDescriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampler,
            .pImageInfo = &descriptorSamplerInfo,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        };

        device.updateDescriptorSets({ writeImageDescriptorSet, writeSamplerDescriptorSet }, {});
    }

    auto SetupRenderState(ImDrawData* pDrawData, vk::CommandBuffer commandBuffer, uint32_t frameIndex, uint32_t width, uint32_t height) -> void {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pGraphicsPipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pGraphicsPipelineLayout, 0, *m_pDescriptorSet, {});

        if (pDrawData->TotalVtxCount > 0) {
            PerFrame& frameDesc = m_PerFrame[frameIndex];
            commandBuffer.bindVertexBuffers(0, { *frameDesc.VertexBuffer.Buffer }, { 0 });
            commandBuffer.bindIndexBuffer(*frameDesc.IndexBuffer.Buffer, 0, vk::IndexType::eUint32);
        }

        {
            vk::Viewport viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(width),
                .height = static_cast<float>(height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f
            };
            commandBuffer.setViewport(0, { viewport });
        }

        {      
            struct {
                ImVec2 scale;
                ImVec2 translate;
            } pushContants;

            pushContants.scale = { +2.0f / pDrawData->DisplaySize.x, +2.0f / pDrawData->DisplaySize.y };;
            pushContants.translate = { -1.0f - pDrawData->DisplayPos.x * pushContants.scale.x, -1.0f - pDrawData->DisplayPos.y * pushContants.scale.y };
            commandBuffer.pushConstants(*m_pGraphicsPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(pushContants), &pushContants);
        }
    }

private:
    struct GeometryBuffer {
        vma::UniqueAllocation Allocation;
        vma::AllocationInfo   AllocationInfo;
        vk::UniqueBuffer      Buffer;
        vk::BufferCreateInfo  BufferInfo;
    };

    struct PerFrame {
        GeometryBuffer VertexBuffer;
        GeometryBuffer IndexBuffer;
    };

    ImGui_ImplVulkanDesc          m_Desc;

    vk::UniqueImage               m_pImage;
    vk::UniqueImageView           m_pImageView;
    vma::UniqueAllocation         m_pImageAllocation;
    vk::UniqueSampler             m_pSampler;
    std::vector<PerFrame>         m_PerFrame;

    vk::UniqueDescriptorPool      m_pDescriptorPool = {};
    vk::UniqueDescriptorSetLayout m_pDescriptorSetLayout;
    vk::UniqueDescriptorSet       m_pDescriptorSet;
    vk::UniquePipelineLayout      m_pGraphicsPipelineLayout;
    vk::UniquePipeline            m_pGraphicsPipeline;
};

ImGui_ImplVulkan* g_pImGuiImplementVulkan;

void ImGui_ImplVulkan_Init(vk::Device device, vk::PhysicalDevice adapter, vma::Allocator allocator, vk::RenderPass renderPass, uint32_t countFrameInFlight) {
    g_pImGuiImplementVulkan = new ImGui_ImplVulkan(ImGui_ImplVulkanDesc{ .Device = device, .Adapter = adapter,  .RenderPass = renderPass, .Allocator = allocator, .FrameInFlight = countFrameInFlight });
}

void ImGui_ImplVulkan_Shutdown() {
    delete g_pImGuiImplementVulkan;
}

void ImGui_ImplVulkan_NewFrame(vk::CommandBuffer commandBuffer, uint32_t frameIndex) {
    g_pImGuiImplementVulkan->DrawDataUpdate(frameIndex);
    g_pImGuiImplementVulkan->DrawFrame(commandBuffer, frameIndex);
}