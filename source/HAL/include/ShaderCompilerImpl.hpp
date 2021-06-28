#pragma once

#include <HAL/ShaderCompiler.hpp>
#include <dxc/dxcapi.use.h>
#include "ComPtr.hpp"

#include <vector>
#include <string>

namespace HAL {

    class ShaderCompiler::Internal {
    public:
        Internal(ShaderCompilerCreateInfo const& createInfo);

        auto CompileFromString(std::wstring_view data, std::wstring_view entryPoint, ShaderStage target, std::optional<std::span<std::wstring_view>> defines = std::nullopt) const -> std::optional<std::vector<uint8_t>>;

        auto CompileFromFile(std::wstring_view path, std::wstring_view entryPoint, ShaderStage target, std::optional<std::span<std::wstring_view>> defines = std::nullopt) const -> std::optional<std::vector<uint8_t>>;

        auto CompileShaderBlob(ComPtr<IDxcBlobEncoding> pDxcBlob, std::wstring_view entryPoint, ShaderStage target, std::span<std::wstring_view> defines) const -> std::optional<std::vector<uint8_t>>;

    private:
        dxc::DxcDllSupport         m_DxcLoader;
        ComPtr<IDxcUtils>          m_pDxcUtils;
        ComPtr<IDxcCompiler3>      m_pDxcCompiler;
        ComPtr<IDxcIncludeHandler> m_pDxcIncludeHandler;
        ShaderModel                m_ShaderModel;
        bool                       m_IsDebug;
    };
}
