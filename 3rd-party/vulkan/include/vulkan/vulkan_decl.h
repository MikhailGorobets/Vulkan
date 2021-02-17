#pragma once

#define VK_NO_PROTOTYPES 
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_mem_alloc.hpp>

#include <spirv/spirv_cross.hpp>
#include <spirv/spirv_parser.hpp>

#include <smolv/smolv.h>
#include <dxc/dxcapi.use.h>

#include <fmt/core.h>
#include <fmt/format.h>
#include <optional>

namespace vkx {

    inline auto getIndexQueueFamilyGraphics(vk::PhysicalDevice device) -> uint32_t {
       auto const queueFamilyProperties = device.getQueueFamilyProperties();
       auto const queueIndex = static_cast<uint32_t>(std::distance(std::begin(queueFamilyProperties), std::find_if(std::begin(queueFamilyProperties), std::end(queueFamilyProperties), [](auto const& e) {
           return e.queueFlags & vk::QueueFlagBits::eGraphics;
       })));
       return queueIndex;
    }

    inline auto getIndexQueueFamilyCompute(vk::PhysicalDevice device) -> uint32_t {
       auto const queueFamilyProperties = device.getQueueFamilyProperties();
       auto const queueIndex = static_cast<uint32_t>(std::distance(std::begin(queueFamilyProperties), std::find_if(std::begin(queueFamilyProperties), std::end(queueFamilyProperties), [](auto const& e) {
           return (e.queueFlags & vk::QueueFlagBits::eCompute) && !(e.queueFlags & vk::QueueFlagBits::eGraphics);
       })));
       return queueIndex;
    }

    inline auto getIndexQueueFamilyTransfer(vk::PhysicalDevice device) -> uint32_t {
        auto const queueFamilyProperties = device.getQueueFamilyProperties();
        auto const queueIndex = static_cast<uint32_t>(std::distance(std::begin(queueFamilyProperties), std::find_if(std::begin(queueFamilyProperties), std::end(queueFamilyProperties), [](auto const& e) {
            return (e.queueFlags & vk::QueueFlagBits::eTransfer) && !(e.queueFlags & vk::QueueFlagBits::eCompute);
        })));
        return queueIndex;
    }

    template<typename T>
    auto setDebugName(vk::Device device, T objectHandle, std::string const& objectName) -> void {
    #ifdef _DEBUG
        auto objectNameCombine = fmt::format("{0}: {1}", vk::to_string(T::objectType), objectName);
        device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT{ .objectType = T::objectType, .objectHandle = reinterpret_cast<uint64_t&>(objectHandle), .pObjectName = objectNameCombine.c_str() });
    #endif
    }

}


namespace vkx {

    inline auto isInstanceLayerAvailable(std::vector<vk::LayerProperties> const& layers, const char* name) -> bool {
        for (auto const& e : layers) {
            if (std::strcmp(e.layerName, name) == 0) {
                return true;
            }
        }
        return false;
    }
    
    inline auto isInstanceExtensionAvailable(std::vector<vk::ExtensionProperties> const& extensions, const char* name) -> bool {
        for (auto const& e : extensions) {
            if (std::strcmp(e.extensionName, name) == 0) {
                return true;
            }
        }
        return false;
    }

    inline auto isDeviceExtensionAvailable(std::vector<vk::ExtensionProperties> const& extensions, const char* name) -> bool {
        for (auto const& e : extensions) {
            if (std::strcmp(e.extensionName, name) == 0) {
                return true;
            }
        }
        return false;
    }
}



namespace vkx {

    class ComException : public std::exception {
    public:
        ComException(HRESULT hr) : result(hr) {}

        const char* what() const override {
            static char s_str[64] = {};
            ::sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<uint32_t>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    inline auto throwIfFailed(HRESULT hr) -> void {
        if (FAILED(hr)) throw ComException(hr);
    }

    template<typename T>
    struct ComPtr {
        ComPtr(T* pComObj = nullptr) : m_pComObj(pComObj) {
            static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");
            if (m_pComObj)
                m_pComObj->AddRef();
        }

        ComPtr(const ComPtr<T>& pComObj) {
            static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");
            m_pComObj = pComObj.m_pComObj;
            if (m_pComObj)
                m_pComObj->AddRef();
        }

        ComPtr(ComPtr<T>&& pComObj) {
            m_pComObj = pComObj.m_pComObj;
            pComObj.m_pComObj = nullptr;
        }

        T* operator=(T* pComObj) {
            if (m_pComObj)
                m_pComObj->Release();

            m_pComObj = pComObj;

            if (m_pComObj)
                m_pComObj->AddRef();

            return m_pComObj;
        }

        T* operator=(const ComPtr<T>& pComObj) {
            if (m_pComObj)
                m_pComObj->Release();

            m_pComObj = pComObj.m_pComObj;

            if (m_pComObj)
                m_pComObj->AddRef();

            return m_pComObj;
        }

        ~ComPtr() {
            if (m_pComObj) {
                m_pComObj->Release();
                m_pComObj = nullptr;
            }
        }

        operator T* () const {
            return m_pComObj;
        }


        T& operator*() const {
            return *m_pComObj;
        }

        T** operator&() {
            assert(nullptr == m_pComObj);
            return &m_pComObj;
        }

        T* operator->() const {
            return m_pComObj;
        }

        bool operator!() const {
            return (nullptr == m_pComObj);
        }

        bool operator<(T* pComObj) const {
            return m_pComObj < pComObj;
        }

        bool operator!=(T* pComObj) const {
            return !operator==(pComObj);
        }

        bool operator==(T* pComObj) const {
            return m_pComObj == pComObj;
        }

        template <typename I>
        HRESULT queryInterface(I** ppInterface) {
            return m_pComObj->QueryInterface(IID_PPV_ARGS(ppInterface));
        }

    protected:
        T* m_pComObj;
    };

    class Compiler;
    class CompileResult {
        friend Compiler;
        CompileResult(std::vector<uint32_t>&& data) : m_Data(data) {}

    public:
        [[nodiscard]] auto data() const -> const uint32_t* {
            return std::data(m_Data);
        }
        [[nodiscard]] auto size() const -> uint32_t {
            return static_cast<uint32_t>(std::size(m_Data) * sizeof(uint32_t));
        }
    private:
        std::vector<uint32_t> m_Data;
    };

    struct CompilerCreateInfo {
        uint32_t shaderModelMajor = 6;
        uint32_t shaderModelMinor = 5;
        uint32_t HLSLVersion = 2018;
    };

    //class CompilerFactory {
    //public:
    //    auto createCompiler(CompilerCreateInfo const& compilerCreateInfo) -> Compiler {
    //        std::scoped_lock<std::mutex> lock(m_Mutex);
    //
    //       // throwIfFailed(m_DxcLoader.Initialize());
    //       // throwIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcUtils, &m_pDxcUtils));
    //       // throwIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcCompiler, &m_pDxcCompiler));
    //       // throwIfFailed(m_pDxcUtils->CreateDefaultIncludeHandler(&m_pDxcIncludeHandler));
    //
    //
    //    }
    //    auto destroyCompiler(Compiler handle) -> void {
    //
    //    }
    //private:
    //    std::mutex m_Mutex;
    //};

    class Compiler {
    public:
        Compiler(CompilerCreateInfo const& compilerCreateInfo) {

            m_CompilerDesc = compilerCreateInfo;
            throwIfFailed(m_DxcLoader.Initialize());
            throwIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcUtils, &m_pDxcUtils));
            throwIfFailed(m_DxcLoader.CreateInstance(CLSID_DxcCompiler, &m_pDxcCompiler));
            throwIfFailed(m_pDxcUtils->CreateDefaultIncludeHandler(&m_pDxcIncludeHandler));

        }

        [[nodiscard]] auto compileFromString(std::wstring const& data, std::wstring const& entryPoint, vk::ShaderStageFlagBits target, std::vector<std::wstring> const& defines) const -> CompileResult {
            ComPtr<IDxcBlobEncoding> pDxcSource;
            throwIfFailed(m_pDxcUtils->CreateBlob(std::data(data), static_cast<uint32_t>(std::size(data)), CP_UTF8, &pDxcSource));
            return compileShaderBlob(pDxcSource, entryPoint, target, defines);
        }

        [[nodiscard]] auto compileFromFile(std::wstring const& path, std::wstring const& entryPoint, vk::ShaderStageFlagBits target, std::vector<std::wstring> const& defines) const -> CompileResult {
            ComPtr<IDxcBlobEncoding> pDxcSource;
            throwIfFailed(m_pDxcUtils->LoadFile(path.c_str(), nullptr, &pDxcSource));
            return compileShaderBlob(pDxcSource, entryPoint, target, defines);
        }

    private:
        [[nodiscard]] auto compileShaderBlob(ComPtr<IDxcBlobEncoding> pDxcBlob, std::wstring const& entryPoint, vk::ShaderStageFlagBits target, std::vector<std::wstring> const& defines) const -> CompileResult {

            auto const getShaderStage = [](vk::ShaderStageFlagBits target) -> std::wstring {
                switch (target) {
                case vk::ShaderStageFlagBits::eVertex:                 return L"vs";
                case vk::ShaderStageFlagBits::eGeometry:               return L"gs";
                case vk::ShaderStageFlagBits::eTessellationControl:    return L"hs";
                case vk::ShaderStageFlagBits::eTessellationEvaluation: return L"ds";
                case vk::ShaderStageFlagBits::eFragment:               return L"ps";
                case vk::ShaderStageFlagBits::eCompute:                return L"cs";
                default: vk::throwResultException(vk::Result::eErrorUnknown, "Unknown shader type");
                }
            };

            const std::wstring shaderProfile = fmt::format(L"{0}_{1}_{2}", getShaderStage(target), m_CompilerDesc.shaderModelMajor, m_CompilerDesc.shaderModelMinor);
            const std::wstring shaderVersion = std::to_wstring(m_CompilerDesc.HLSLVersion);

            std::vector<LPCWSTR> dxcArguments;
            dxcArguments.insert(std::end(dxcArguments), { L"-E",  entryPoint.c_str() });
            dxcArguments.insert(std::end(dxcArguments), { L"-T",  shaderProfile.c_str() });
            dxcArguments.insert(std::end(dxcArguments), { L"-HV", shaderVersion.c_str() });

            dxcArguments.insert(std::end(dxcArguments), { L"-spirv", L"-fspv-reflect", L"-fspv-target-env=vulkan1.1", L"-enable-16bit-types" });
#ifdef  _DEBUG
            dxcArguments.insert(std::end(dxcArguments), { L"-fspv-debug=file", L"-fspv-debug=source",  L"-fspv-debug=line",  L"-fspv-debug=tool" });
#endif

            for (auto const& e : defines)
                dxcArguments.insert(std::end(dxcArguments), { L"-D", e.c_str() });

            DxcText dxcBuffer = {};
            dxcBuffer.Ptr = pDxcBlob->GetBufferPointer();
            dxcBuffer.Size = pDxcBlob->GetBufferSize();

            ComPtr<IDxcResult> pDxcCompileResult;
            throwIfFailed(m_pDxcCompiler->Compile(&dxcBuffer, std::data(dxcArguments), static_cast<uint32_t>(std::size(dxcArguments)), m_pDxcIncludeHandler, IID_PPV_ARGS(&pDxcCompileResult)));

            ComPtr<IDxcBlobUtf8> pDxcErrors;
            throwIfFailed(pDxcCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pDxcErrors), nullptr));
            if (pDxcErrors && pDxcErrors->GetStringLength() > 0)
                fmt::print("{}\n", static_cast<const char*>(pDxcErrors->GetBufferPointer()));


            ComPtr<IDxcBlob> pDxcShaderCode;
            throwIfFailed(pDxcCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pDxcShaderCode), nullptr));

            auto spirv = std::vector<uint32_t>(static_cast<uint32_t*>(pDxcShaderCode->GetBufferPointer()), static_cast<uint32_t*>(pDxcShaderCode->GetBufferPointer()) + pDxcShaderCode->GetBufferSize() / sizeof(uint32_t));
            return CompileResult(std::move(spirv));
        }

    private:
        dxc::DxcDllSupport         m_DxcLoader;
        ComPtr<IDxcUtils>          m_pDxcUtils;
        ComPtr<IDxcCompiler3>      m_pDxcCompiler;
        ComPtr<IDxcIncludeHandler> m_pDxcIncludeHandler;
        CompilerCreateInfo         m_CompilerDesc;
    };
}


namespace vkx {

    struct SpirvShaderResourceAttributes {
        vk::DescriptorType descriptorType;
        uint32_t           descriptorCount;
        std::string        descriptorName;
    };

    struct SpirvShaderStageInputAttributes {

    };

    struct SpirvShaderResources {

    };

}

namespace vkx {

    struct ImageTransition {
        vk::Image         image = {};
        vk::ImageLayout   oldLayout = {};
        vk::ImageLayout   newLayout = {};   
        vk::PipelineStageFlags srcStages = {};
        vk::PipelineStageFlags dstStages = {};
        vk::PipelineStageFlags enabledShaderStages = {};
        vk::ImageSubresourceRange subresourceRange = {};
    };

    struct BufferTransition {
        vk::Buffer      buffer = {};
        vk::AccessFlags srcAccessMask = {};
        vk::AccessFlags dstAccessMask = {};
        vk::PipelineStageFlags srcStages = {};
        vk::PipelineStageFlags dstStages = {};
        vk::PipelineStageFlags enabledShaderStages = {}; 
    };

    void imageTransition(vk::CommandBuffer cmdBuffer, ImageTransition const& translation);

    void bufferTransition(vk::CommandBuffer cmdBuffer, BufferTransition const& translation);

}


namespace vkx {
    class [[nodiscard]] DebugUtilsLabelScoped {
    public:
        DebugUtilsLabelScoped(vk::CommandBuffer& cmdBuffer, std::string const& name, vk::ArrayWrapper1D<float, 4> color): cmdBuffer(cmdBuffer) {
            cmdBuffer.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT{ .pLabelName = name.c_str(), .color = color });
        }
        ~DebugUtilsLabelScoped() {
            cmdBuffer.endDebugUtilsLabelEXT();
        } 
    private:
        vk::CommandBuffer& cmdBuffer;
    };
}