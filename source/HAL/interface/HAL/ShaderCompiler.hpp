#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    enum class ShaderModel {
        SM_6_5
    };
    
    enum class ShaderStage {
        Vertex,
        Hull,
        Domain,
        Geometry,
        Pixel,
        Compute     
    };

    struct ShaderCompilerCreateInfo {
        ShaderModel ShaderModelVersion;
        bool        IsDebugMode;
    };

    class ShaderCompiler {
    public:
        class Internal;
    public:
        ShaderCompiler(ShaderCompilerCreateInfo const& createInfo);

        auto CompileFromString(std::wstring const& data, std::wstring const& entryPoint, ShaderStage target, std::vector<std::wstring> const& defines) const -> std::optional<std::vector<uint8_t>>;
         
        auto CompileFromFile(std::wstring const& path, std::wstring const& entryPoint, ShaderStage target, std::vector<std::wstring> const& defines) const -> std::optional<std::vector<uint8_t>>;
             
    private:    
        Internal_Ptr<Internal, InternalSize_Compiler> m_pInternal;   
    };

}