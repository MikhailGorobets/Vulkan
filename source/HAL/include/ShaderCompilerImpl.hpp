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
        
         auto CompileFromString(std::wstring const& data, std::wstring const& entryPoint, ShaderStage target, std::vector<std::wstring> const& defines) const -> std::optional<std::vector<uint32_t>>;
         
         auto CompileFromFile(std::wstring const& path, std::wstring const& entryPoint, ShaderStage target, std::vector<std::wstring> const& defines) const -> std::optional<std::vector<uint32_t>>;

         auto CompileShaderBlob(ComPtr<IDxcBlobEncoding> pDxcBlob, std::wstring const& entryPoint, ShaderStage target, std::vector<std::wstring> const& defines) const -> std::optional<std::vector<uint32_t>>;

    private:
        dxc::DxcDllSupport         m_DxcLoader;
        ComPtr<IDxcUtils>          m_pDxcUtils;
        ComPtr<IDxcCompiler3>      m_pDxcCompiler;
        ComPtr<IDxcIncludeHandler> m_pDxcIncludeHandler;
        ShaderModel                m_ShaderModel;
        bool                       m_IsDebug;
    };
}
