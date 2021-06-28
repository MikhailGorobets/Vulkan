#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    enum class ShaderModel {
        SM_6_5
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

        ~ShaderCompiler();

        auto CompileFromString(std::wstring_view data, std::wstring_view entryPoint, ShaderStage target, std::optional<std::span<std::wstring_view>> defines = std::nullopt) const -> std::optional<std::vector<uint8_t>>;

        auto CompileFromFile(std::wstring_view path, std::wstring_view entryPoint, ShaderStage target, std::optional<std::span<std::wstring_view>> defines = std::nullopt) const -> std::optional<std::vector<uint8_t>>;

    private:
        InternalPtr<Internal, InternalSize_ShaderCompiler> m_pInternal;
    };
}