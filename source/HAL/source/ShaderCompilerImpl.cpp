#include <Windows.h>
#include "../include/ShaderCompilerImpl.hpp"
#include <fmt/format.h>

namespace HAL {

    ShaderCompiler::Internal::Internal(ShaderCompilerCreateInfo const& createInfo) {
        ThrowIfFailed(m_DxcLoader.Initialize());
        ThrowIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcUtils, &m_pDxcUtils));
        ThrowIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcCompiler, &m_pDxcCompiler));
        ThrowIfFailed(m_pDxcUtils->CreateDefaultIncludeHandler(&m_pDxcIncludeHandler));
        m_IsDebug = createInfo.IsDebugMode;
        m_ShaderModel = createInfo.ShaderModelVersion;
    }

    auto ShaderCompiler::Internal::CompileFromString(std::wstring_view data, std::wstring_view entryPoint, ShaderStage target, std::optional<std::span<std::wstring_view>> defines) const -> std::optional<std::vector<uint8_t>> {
        ComPtr<IDxcBlobEncoding> pDxcSource;
        ThrowIfFailed(m_pDxcUtils->CreateBlob(std::data(data), static_cast<uint32_t>(std::size(data)), CP_UTF8, &pDxcSource));
        return CompileShaderBlob(pDxcSource, entryPoint, target, defines.value_or(std::span<std::wstring_view>{}));
    }

    auto ShaderCompiler::Internal::CompileFromFile(std::wstring_view path, std::wstring_view entryPoint, ShaderStage target, std::optional<std::span<std::wstring_view>> defines) const -> std::optional<std::vector<uint8_t>> {
        ComPtr<IDxcBlobEncoding> pDxcSource;
        ThrowIfFailed(m_pDxcUtils->LoadFile(path.data(), nullptr, &pDxcSource));
        return CompileShaderBlob(pDxcSource, entryPoint, target, defines.value_or(std::span<std::wstring_view>{}));
    }

    auto ShaderCompiler::Internal::CompileShaderBlob(ComPtr<IDxcBlobEncoding> pDxcBlob, std::wstring_view entryPoint, ShaderStage target, std::span<std::wstring_view> defines) const -> std::optional<std::vector<uint8_t>> {
        auto const GetVkShaderStage = [](ShaderStage target) -> std::optional<std::wstring_view> {
            switch (target) {
                case ShaderStage::Vertex:   return L"vs";
                case ShaderStage::Geometry: return L"gs";
                case ShaderStage::Hull:     return L"hs";
                case ShaderStage::Domain:   return L"ds";
                case ShaderStage::Fragment: return L"ps";
                case ShaderStage::Compute:  return L"cs";
                default: return std::nullopt;
            }
        };

        auto const GetShaderModel = [](ShaderModel target) -> std::optional<std::wstring_view> {
            switch (target) {
                case ShaderModel::SM_6_5: return L"6_5";
                default: return std::nullopt;
            }
        };

        const std::wstring shaderProfile = fmt::format(L"{0}_{1}", *GetVkShaderStage(target), *GetShaderModel(m_ShaderModel));
        const std::wstring shaderVersion = std::to_wstring(2018);

        std::vector<const wchar_t*> dxcArguments;
        dxcArguments.insert(std::end(dxcArguments), {L"-E",  entryPoint.data()});
        dxcArguments.insert(std::end(dxcArguments), {L"-T",  shaderProfile.c_str()});
        dxcArguments.insert(std::end(dxcArguments), {L"-HV", shaderVersion.c_str()});

        dxcArguments.insert(std::end(dxcArguments), {L"-spirv", L"-fspv-reflect", L"-fspv-target-env=vulkan1.2", L"-enable-16bit-types"});
        if (m_IsDebug)
            dxcArguments.insert(std::end(dxcArguments), {L"-fspv-debug=file", L"-fspv-debug=source",  L"-fspv-debug=line",  L"-fspv-debug=tool"});

        for (auto const& e : defines)
            dxcArguments.insert(std::end(dxcArguments), {L"-D", e.data()});

        DxcText dxcBuffer = {};
        dxcBuffer.Ptr = pDxcBlob->GetBufferPointer();
        dxcBuffer.Size = pDxcBlob->GetBufferSize();

        ComPtr<IDxcResult> pDxcCompileResult;
        if (auto result = m_pDxcCompiler->Compile(&dxcBuffer, std::data(dxcArguments), static_cast<uint32_t>(std::size(dxcArguments)), m_pDxcIncludeHandler, IID_PPV_ARGS(&pDxcCompileResult)); SUCCEEDED(result)) {

            ComPtr<IDxcBlobUtf8> pDxcErrors;
            ThrowIfFailed(pDxcCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pDxcErrors), nullptr));
            if (pDxcErrors && pDxcErrors->GetStringLength() > 0)
                fmt::print("Error: {}\n", static_cast<const char*>(pDxcErrors->GetBufferPointer()));

            ComPtr<IDxcBlob> pDxcShaderCode;
            ThrowIfFailed(pDxcCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pDxcShaderCode), nullptr));
            return std::vector<uint8_t>(static_cast<uint8_t*>(pDxcShaderCode->GetBufferPointer()), static_cast<uint8_t*>(pDxcShaderCode->GetBufferPointer()) + pDxcShaderCode->GetBufferSize());
        } else {
            ComPtr<IDxcBlobUtf8> pDxcErrors;
            ThrowIfFailed(pDxcCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pDxcErrors), nullptr));
            if (pDxcErrors && pDxcErrors->GetStringLength() > 0)
                fmt::print("Error: {}\n", static_cast<const char*>(pDxcErrors->GetBufferPointer()));
            return std::nullopt;
        }
    }
}

namespace HAL {
    ShaderCompiler::ShaderCompiler(ShaderCompilerCreateInfo const& createInfo): m_pInternal(createInfo) {}

    ShaderCompiler::~ShaderCompiler() = default;

    auto ShaderCompiler::CompileFromString(std::wstring_view data, std::wstring_view entryPoint, ShaderStage target, std::optional<std::span<std::wstring_view>> defines) const -> std::optional<std::vector<uint8_t>> {
        return m_pInternal->CompileFromString(data, entryPoint, target, defines);
    }

    auto ShaderCompiler::CompileFromFile(std::wstring_view path, std::wstring_view entryPoint, ShaderStage target, std::optional<std::span<std::wstring_view>> defines) const -> std::optional<std::vector<uint8_t>> {
        return m_pInternal->CompileFromFile(path, entryPoint, target, defines);
    }
}