#include <vulkan/vulkan_decl.h>

#include <dxc/dxcapi.use.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <implot/implot.h>

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <hlslpp/hlsl++.h>

#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>
#include <mutex>


struct ScrollingBuffer {
    int32_t MaxSize = 4096;
    int32_t Offset  = 0;
    std::vector<ImVec2> Data;
    ScrollingBuffer() {
        Data.reserve(MaxSize);
    }
    auto AddPoint(float x, float y) -> void {
        if (std::size(Data) < MaxSize)
            Data.push_back(ImVec2(x, y));
        else {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
    auto ComputeMax(float timeRange) -> float {            
        auto time = 0.0f;
        auto max  = std::numeric_limits<float>::min();

        auto x = this->Iter();
        auto y = this->Iter();
        for (Next(y); time < timeRange && Next(x) && Next(y);) {
            time += x.first->x - y.first->x;
            max = std::max(max, x.first->y);
        }
        return max;
    };

    auto ComputeAverage(float timeRange) -> float {
        auto time = 0.0f;
        auto sum = 0.0f;
        auto count = 0;

        auto x = this->Iter();
        auto y = this->Iter();
        for (Next(y); time < timeRange && Next(x) && Next(y);) {
            time += x.first->x - y.first->x;
            sum  += x.first->y;
            count++;
        }
        return sum / count;
    }

private:
    using Interator = std::pair<std::vector<ImVec2>::iterator, size_t>;

    auto Iter() -> Interator {
        if (Offset > 0)
            return std::make_pair(std::begin(Data) + Offset, 0);
        else          
            return std::make_pair(std::end(Data), 0);
    }

    auto Next(Interator& iter) -> bool {      
        if (iter.second < std::size(Data)) {
            iter.first = iter.first == std::begin(Data) ? std::end(Data) - 1 : iter.first - 1;
            iter.second++;
            return true;
        }   
        return false;
    }
};

class MemoryStatisticGPU {
public:
    enum class HeapType: uint64_t {
        DeviceMemory,
        SystemMemory
    };
    
    struct MemoryFrame {
        double   TimePoint;
        uint64_t MemoryUsed;
    };

    struct MemoryHeapStats {
        uint64_t MemorySize;
        uint64_t MemoryUsed;
        uint32_t MemoryIndex;
        uint32_t MemoryCount;
        HeapType MemoryType;
        std::vector<MemoryFrame> MemoryFrames;
    };
public:
    MemoryStatisticGPU(vk::PhysicalDevice physicalDevice) {
        m_MemoryProperty = physicalDevice.getMemoryProperties();
        for (uint32_t index = 0; index < m_MemoryProperty.memoryHeapCount; index++) {          
            MemoryHeapStats heap = {
                .MemorySize = m_MemoryProperty.memoryHeaps[index].size,
                .MemoryUsed = 0,
                .MemoryIndex = index,
                .MemoryCount = 0,
                .MemoryType = m_MemoryProperty.memoryHeaps[index].flags & vk::MemoryHeapFlagBits::eDeviceLocal ? HeapType::DeviceMemory : HeapType::SystemMemory
            };
            m_GpuMemory.push_back(heap);
        }
        m_TimeStamp = std::chrono::high_resolution_clock::now();
    }

    auto AddMemoryInfo(uint32_t memoryTypeIndex, vk::DeviceSize size) -> void {
        assert(memoryTypeIndex < m_MemoryProperty.memoryTypeCount);
        auto time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - m_TimeStamp).count();
        auto heapIndex = m_MemoryProperty.memoryTypes[memoryTypeIndex].heapIndex;
        m_GpuMemory[heapIndex].MemoryUsed += size;
        m_GpuMemory[heapIndex].MemoryCount++;
        m_GpuMemory[heapIndex].MemoryFrames.push_back({ time, m_GpuMemory[heapIndex].MemoryUsed });
    }
    
    auto RemoveMemoryInfo(uint32_t memoryTypeIndex, vk::DeviceSize size) -> void {
        assert(memoryTypeIndex < m_MemoryProperty.memoryTypeCount);
        auto time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - m_TimeStamp).count();
        auto heapIndex = m_MemoryProperty.memoryTypes[memoryTypeIndex].heapIndex;
        m_GpuMemory[heapIndex].MemoryUsed -= size;
        m_GpuMemory[heapIndex].MemoryCount--;
        m_GpuMemory[heapIndex].MemoryFrames.push_back({ time, m_GpuMemory[heapIndex].MemoryUsed });
    }

    auto GetMemoryStatistic() -> std::vector<MemoryHeapStats> const& {
        return m_GpuMemory;
    }

    static auto GPUAllocate(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* pUserData) -> void {
        if (pUserData) {
            auto pMemoryStat = reinterpret_cast<MemoryStatisticGPU*>(pUserData);
            pMemoryStat->AddMemoryInfo(memoryType, size);
        }
          
        vk::MemoryPropertyFlags memoryInfo = {};
        VmaAllocatorInfo allocatorInfo = {};
        vmaGetMemoryTypeProperties(allocator, memoryType, reinterpret_cast<VkMemoryPropertyFlags*>(&memoryInfo));
        vmaGetAllocatorInfo(allocator, &allocatorInfo);

        vkx::setDebugName(vk::Device(allocatorInfo.device), vk::DeviceMemory(memory), fmt::format("Type: {0} Size: {1} kb", vk::to_string(memoryInfo), (size >> 10)));
        fmt::print("Alloc -> Memory type: {0}, Size: {1} kb \n", vk::to_string(memoryInfo), (size >> 10));
    }
    
    static auto GPUFree(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* pUserData) -> void {
         if (pUserData) {
            auto pMemoryStat = reinterpret_cast<MemoryStatisticGPU*>(pUserData);
            pMemoryStat->RemoveMemoryInfo(memoryType, size);
         }

         vk::MemoryPropertyFlags memoryInfo = {};
         vmaGetMemoryTypeProperties(allocator, memoryType, reinterpret_cast<VkMemoryPropertyFlags*>(&memoryInfo));
         fmt::print("Free -> Memory type: {0}, Size: {1} kb \n", vk::to_string(memoryInfo), (size >> 10));    
    }

private:
    vk::PhysicalDeviceMemoryProperties    m_MemoryProperty;
    std::vector<MemoryHeapStats>          m_GpuMemory;
    std::chrono::steady_clock::time_point m_TimeStamp;
};

class MemoryStatisticCPU {

};


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
        
        [[nodiscard]] auto operator[](size_t index) const -> uint32_t { 
            return this->data()[index];
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



//auto GenerateCPPFromShaderModule(std::string const& fileName, std::wstring const& path, std::wstring const& kernel, vk::ShaderStageFlagBits stage, std::vector<std::wstring> const& defines) {
//    vkx::Compiler compiler(vkx::CompilerCreateInfo{.shaderModelMajor = 6, .shaderModelMinor = 5, .HLSLVersion = 2018});
//    auto const spirv = compiler.compileFromFile(path, kernel, stage, defines);
//
//
//    auto const GetShaderStage = [](vk::ShaderStageFlagBits target) -> const char* {
//        switch (target) {
//        case vk::ShaderStageFlagBits::eVertex:
//            return "VertexShader_SPIRV";
//        case vk::ShaderStageFlagBits::eGeometry:
//            return "GeometryShader_SPIRV";
//        case vk::ShaderStageFlagBits::eTessellationControl:
//            return "TessellationControlhader_SPIRV";
//        case vk::ShaderStageFlagBits::eTessellationEvaluation:
//            return "TessellationEvaluationShader_SPIRV";
//        case vk::ShaderStageFlagBits::eFragment:
//            return "FragmentShader_SPIRV";
//        case vk::ShaderStageFlagBits::eCompute:
//            return "ComputeShader_SPIRV";
//        default:
//            vk::throwResultException(vk::Result::eErrorUnknown, "Unknown shader type");
//        }
//    };
//
//    std::unique_ptr<FILE, decltype(&fclose)> pFile(fopen(fileName.c_str(), "w"), fclose);
// 
//
//    fprintf(pFile.get(), "static constexpr uint32_t %s[] =", GetShaderStage(stage));
//    fprintf(pFile.get(), "\n{", GetShaderStage(stage));
//
//    for (size_t index = 0; index < spirv.size() / 4; index++) {
//        if (index % 8 == 0)
//            fprintf(pFile.get(), "\n    ");
//        fprintf(pFile.get(), "0x%08x,", spirv[index]);      
//    }
// 
//    fprintf(pFile.get(), "\n};\n", GetShaderStage(stage));
//}
//


int main(int argc, char* argv[]) {
    
    auto const WINDOW_TITLE = "Application Vulkan";
    auto const WINDOW_WIDTH  = 1920;
    auto const WINDOW_HEIGHT = 1280;
    auto const BUFFER_COUNT  = 3u;
    auto const FRAMES_IN_FLIGHT = 2u;
 
    
    const char* INSTANCE_EXTENSION[] = {
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,	
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    
    const char* INSTANCE_LAYERS[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    const char* DEVICE_EXTENSION[] = {   
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
    };
  
    struct GLFWScoped {
         GLFWScoped() { 
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
         }
        ~GLFWScoped() { glfwTerminate(); }
    } GLFW;
   
    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> pWindow(glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr), glfwDestroyWindow);
  
    vk::DynamicLoader vulkanDLL;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkanDLL.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
    
    vk::UniqueInstance pInstance; {
              
        std::vector<const char*> instanceLayers;
        std::vector<vk::LayerProperties> instanceLayerProperties = vk::enumerateInstanceLayerProperties();
        for (size_t index = 0; index < _countof(INSTANCE_LAYERS); index++) {
            if (vkx::isInstanceLayerAvailable(instanceLayerProperties, INSTANCE_LAYERS[index])) {
#ifdef _DEBUG
                instanceLayers.push_back(INSTANCE_LAYERS[index]);
#endif
            }
        }

        std::vector<const char*> instanceExtensions;
        std::vector<vk::ExtensionProperties> instanceExtensionProperties = vk::enumerateInstanceExtensionProperties();
        for (size_t index = 0; index < _countof(INSTANCE_EXTENSION); index++) {
            if (vkx::isInstanceExtensionAvailable(instanceExtensionProperties, INSTANCE_EXTENSION[index])) {
                instanceExtensions.push_back(INSTANCE_EXTENSION[index]);
            }
        }

        vk::ApplicationInfo applicationInfo = {
            .pApplicationName = WINDOW_TITLE,
            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
            .pEngineName = "None",
            .engineVersion = VK_MAKE_VERSION(0, 0, 1),
            .apiVersion = VK_API_VERSION_1_2
        };

        vk::InstanceCreateInfo instanceCI = {
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<uint32_t>(std::size(instanceLayers)),
            .ppEnabledLayerNames = std::data(instanceLayers),
            .enabledExtensionCount = static_cast<uint32_t>(std::size(instanceExtensions)),
            .ppEnabledExtensionNames = std::data(instanceExtensions)
        };
        pInstance = vk::createInstanceUnique(instanceCI);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*pInstance);
    }

    vk::PhysicalDevice physicalDevice = pInstance->enumeratePhysicalDevices().front();

    const uint32_t indexQueueFamilyGraphics = vkx::getIndexQueueFamilyGraphics(physicalDevice).value();
    const uint32_t indexQueueFamilyCompute  = vkx::getIndexQueueFamilyCompute(physicalDevice).value();
    const uint32_t indexQueueFamilyTransfer = vkx::getIndexQueueFamilyTransfer(physicalDevice).value();
 
    vk::UniqueDevice pDevice; {

        float queuePriorities[] = { 1.0f };

        vk::DeviceQueueCreateInfo deviceQueueCI[] = {
            vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = indexQueueFamilyGraphics,
                .queueCount = _countof(queuePriorities),
                .pQueuePriorities = queuePriorities
            },
            vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = indexQueueFamilyCompute,
                .queueCount = _countof(queuePriorities),
                .pQueuePriorities = queuePriorities
            },
            vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = indexQueueFamilyTransfer,
                .queueCount = _countof(queuePriorities),
                .pQueuePriorities = queuePriorities
            }
        };
       
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceShaderFloat16Int8Features, vk::PhysicalDeviceImagelessFramebufferFeatures> enabledFeatures = { 
            vk::PhysicalDeviceFeatures2 { 
                .features = vk::PhysicalDeviceFeatures {
                    .shaderInt64 = true,
                    .shaderInt16 = true
                }
            },
            vk::PhysicalDeviceShaderFloat16Int8Features{ 
                .shaderFloat16 = true, 
                .shaderInt8    = true 
            },
            vk::PhysicalDeviceImagelessFramebufferFeatures{ 
                .imagelessFramebuffer = true 
            }
        };
    
        std::vector<const char*> deviceExtensions;
        std::vector<vk::ExtensionProperties> deviceExtensionProperties = physicalDevice.enumerateDeviceExtensionProperties();
        for (size_t index = 0; index < _countof(DEVICE_EXTENSION); index++) {
            if (vkx::isDeviceExtensionAvailable(deviceExtensionProperties, DEVICE_EXTENSION[index])) {
                deviceExtensions.push_back(DEVICE_EXTENSION[index]);
            }
        }

        vk::DeviceCreateInfo deviceCI = {
            .pNext = &enabledFeatures.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = _countof(deviceQueueCI),
            .pQueueCreateInfos = deviceQueueCI,
            .enabledExtensionCount = static_cast<uint32_t>(std::size(DEVICE_EXTENSION)),
            .ppEnabledExtensionNames = std::data(DEVICE_EXTENSION), 
        };

        vk::PhysicalDeviceProperties deviceDesc = physicalDevice.getProperties();
        pDevice = physicalDevice.createDeviceUnique(deviceCI);
         
        vkx::setDebugName(*pDevice, physicalDevice, fmt::format("Name: {0} Type: {1}", deviceDesc.deviceName, vk::to_string(deviceDesc.deviceType)));
        vkx::setDebugName(*pDevice, *pInstance, fmt::format("ApiVersion: {0}.{1}.{2}", VK_VERSION_MAJOR(deviceDesc.apiVersion), VK_VERSION_MINOR(deviceDesc.apiVersion), VK_VERSION_PATCH(deviceDesc.apiVersion)));
        vkx::setDebugName(*pDevice, *pDevice, deviceDesc.deviceName);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*pDevice); 
    }

    MemoryStatisticGPU memoryGPU = { physicalDevice };
    MemoryStatisticCPU memoryCPU;
  
    vma::UniqueAllocator pAllocator; {

        vma::DeviceMemoryCallbacks deviceMemoryCallbacks = {
            .pfnAllocate = MemoryStatisticGPU::GPUAllocate,
            .pfnFree     = MemoryStatisticGPU::GPUFree,
            .pUserData   = &memoryGPU,
        };
        
        vma::VulkanFunctions vulkanFunctions = {
            .vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
            .vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
            .vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
            .vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
            .vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
            .vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
            .vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
            .vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
            .vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
            .vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
            .vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
            .vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
            .vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
            .vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
            .vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
            .vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
            .vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
            .vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2,
            .vkGetImageMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2,
            .vkBindBufferMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2,
            .vkBindImageMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2,
            .vkGetPhysicalDeviceMemoryProperties2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2
        };
        
        vma::AllocatorCreateInfo allocatorCI = {
            .flags = vma::AllocatorCreateFlagBits::eExtMemoryBudget | vma::AllocatorCreateFlagBits::eExternallySynchronized,
            .physicalDevice = physicalDevice,      
            .device = *pDevice,    
            .pDeviceMemoryCallbacks = &deviceMemoryCallbacks,
            .frameInUseCount = FRAMES_IN_FLIGHT,
            .pVulkanFunctions = &vulkanFunctions,
            .instance = *pInstance,    
            .vulkanApiVersion = VK_API_VERSION_1_2,         
        };    
        pAllocator = vma::createAllocatorUnique(allocatorCI);
    }
    
    vk::UniqueRenderPass pRenderPass; {

        vk::AttachmentDescription attachments[] = {
            vk::AttachmentDescription{
                .format = vk::Format::eB8G8R8A8Srgb,
                .samples = vk::SampleCountFlagBits::e1,
                .loadOp = vk::AttachmentLoadOp::eDontCare,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .initialLayout = vk::ImageLayout::eUndefined,
                .finalLayout = vk::ImageLayout::ePresentSrcKHR
            },

            vk::AttachmentDescription{
                .format = vk::Format::eD32SfloatS8Uint,
                .samples = vk::SampleCountFlagBits::e1,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout = vk::ImageLayout::eUndefined,
                .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
            }, 
        };

        vk::AttachmentReference colorAttachmentReferencePass0[] = {
            vk::AttachmentReference{ .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal },
        };

        vk::AttachmentReference depthAttachmentReferencePass0[] = {
            vk::AttachmentReference{ .attachment = 1, .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal }
        };

        vk::SubpassDescription subpassDescriptions[] = {
            vk::SubpassDescription {
                .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
                .colorAttachmentCount = _countof(colorAttachmentReferencePass0),
                .pColorAttachments = colorAttachmentReferencePass0,
                .pDepthStencilAttachment = depthAttachmentReferencePass0
            }        
        };

        vk::RenderPassCreateInfo renderPassCI = {
            .attachmentCount = _countof(attachments),
            .pAttachments = attachments,
            .subpassCount = _countof(subpassDescriptions),
            .pSubpasses = subpassDescriptions         
        };
        pRenderPass = pDevice->createRenderPassUnique(renderPassCI);
        vkx::setDebugName(*pDevice, *pRenderPass, "");
    }

    vk::UniqueDescriptorSetLayout pDescriptorSetLayout; {
        vk::DescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
            vk::DescriptorSetLayoutBinding {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eUniformBufferDynamic,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eAllGraphics
            }   
        };

        vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {
            .bindingCount = _countof(descriptorSetLayoutBindings),
            .pBindings = descriptorSetLayoutBindings
        };

        pDescriptorSetLayout = pDevice->createDescriptorSetLayoutUnique(descriptorSetLayoutCI);
        vkx::setDebugName(*pDevice, *pDescriptorSetLayout, "");
    }

    vk::UniquePipelineLayout pPipelineLayout; {
        vk::PipelineLayoutCreateInfo pipelineLayoutCI = {
            .setLayoutCount = 1,
            .pSetLayouts = pDescriptorSetLayout.getAddressOf(),
            .pushConstantRangeCount = 0
        };
        pPipelineLayout = pDevice->createPipelineLayoutUnique(pipelineLayoutCI);
        vkx::setDebugName(*pDevice, *pPipelineLayout, "");  
    }

    vk::UniquePipelineCache pPipelineCache;{
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

        pPipelineCache = pDevice->createPipelineCacheUnique(pipelineCacheCI);
        vkx::setDebugName(*pDevice, *pPipelineCache, "");
    }
    
    vk::UniquePipeline pPipeline; {
        vkx::Compiler compiler(vkx::CompilerCreateInfo{ .shaderModelMajor = 6, .shaderModelMinor = 5, .HLSLVersion = 2018});

        auto const spirvVS = compiler.compileFromFile(L"content/shaders/WaveFront.hlsl", L"VSMain", vk::ShaderStageFlagBits::eVertex, {});
        auto const spirvFS = compiler.compileFromFile(L"content/shaders/WaveFront.hlsl", L"PSMain", vk::ShaderStageFlagBits::eFragment, {});
     

        vk::UniqueShaderModule vs = pDevice->createShaderModuleUnique(vk::ShaderModuleCreateInfo{ .codeSize = std::size(spirvVS), .pCode = reinterpret_cast<const uint32_t*>(std::data(spirvVS)) });
        vk::UniqueShaderModule fs = pDevice->createShaderModuleUnique(vk::ShaderModuleCreateInfo{ .codeSize = std::size(spirvFS), .pCode = reinterpret_cast<const uint32_t*>(std::data(spirvFS)) });
        vkx::setDebugName(*pDevice, *vs, "[VS] WaveFront");
        vkx::setDebugName(*pDevice, *fs, "[FS] WaveFront");

        vk::PipelineShaderStageCreateInfo shaderStagesCI[] = {
            vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eVertex,   .module = *vs, .pName = "VSMain" },
            vk::PipelineShaderStageCreateInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = *fs, .pName = "PSMain" },
        };

        vk::PipelineViewportStateCreateInfo viewportStateCI = {
            .viewportCount = 1,        
            .scissorCount = 1
        };

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = false
        };

        vk::PipelineVertexInputStateCreateInfo vertexInputStateCI = {
            .vertexBindingDescriptionCount  = 0,
            .vertexAttributeDescriptionCount = 0
        };

        vk::PipelineColorBlendAttachmentState colorBlendAttachments[] = {
            vk::PipelineColorBlendAttachmentState{
                .blendEnable = false,
                .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
            }      
        };

        vk::PipelineDepthStencilStateCreateInfo depthStencilStateCI = {
            .depthTestEnable = true,
            .depthWriteEnable = false,
            .depthCompareOp = vk::CompareOp::eLess,
            .stencilTestEnable = false
        };

        vk::PipelineMultisampleStateCreateInfo multisampleStateCI = {
            .rasterizationSamples = vk::SampleCountFlagBits::e1
        };

        vk::PipelineColorBlendStateCreateInfo colorBlendStateCI ={
            .logicOpEnable = false,
            .attachmentCount = _countof(colorBlendAttachments),
            .pAttachments = colorBlendAttachments,
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
               
        vk::PipelineCreationFeedbackEXT creationFeedback = {};
        vk::PipelineCreationFeedbackEXT stageCreationFeedbacks[] = {
            vk::PipelineCreationFeedbackEXT {},
            vk::PipelineCreationFeedbackEXT {}
        };
 
        vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineCreationFeedbackCreateInfoEXT> pipelineCI = {
            vk::GraphicsPipelineCreateInfo {
                .stageCount = _countof(shaderStagesCI),
                .pStages = shaderStagesCI,
                .pVertexInputState = &vertexInputStateCI,
                .pInputAssemblyState = &inputAssemblyStateCI,
                .pViewportState = &viewportStateCI,
                .pRasterizationState = &rasterizationStateCI,
                .pMultisampleState = &multisampleStateCI,
                .pDepthStencilState = &depthStencilStateCI,
                .pColorBlendState = &colorBlendStateCI,
                .pDynamicState = &dynamicStateCI,
                .layout = *pPipelineLayout,
                .renderPass = *pRenderPass,
            },
            vk::PipelineCreationFeedbackCreateInfoEXT {
                .pPipelineCreationFeedback = &creationFeedback,
                .pipelineStageCreationFeedbackCount = _countof(stageCreationFeedbacks),
                .pPipelineStageCreationFeedbacks = stageCreationFeedbacks
            }
        };

        std::tie(std::ignore, pPipeline) = pDevice->createGraphicsPipelineUnique(*pPipelineCache, pipelineCI.get<vk::GraphicsPipelineCreateInfo>()).asTuple();
        vkx::setDebugName(*pDevice, *pPipeline, "WaveFront");      
        fmt::print("Flags: {} Duration: {}s\n", vk::to_string(creationFeedback.flags), creationFeedback.duration / 1E9f);     
    }
    
    std::vector<vk::UniqueCommandPool> commandPools;
    for (size_t index = 0; index < FRAMES_IN_FLIGHT; index++) {
        vk::CommandPoolCreateInfo commandPoolCI = {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = indexQueueFamilyGraphics
        };
        auto pCommandPool = pDevice->createCommandPoolUnique(commandPoolCI);
        vkx::setDebugName(*pDevice, *pCommandPool, fmt::format("[{0}]", index));
        commandPools.push_back(std::move(pCommandPool));
    }

    std::vector<vk::UniqueCommandBuffer> cmdBuffers;
    for (size_t index = 0; index < FRAMES_IN_FLIGHT; index++) {
        vk::CommandBufferAllocateInfo commandBufferAI = {
            .commandPool = *commandPools[index],
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        auto pCommandBuffer = std::move(pDevice->allocateCommandBuffersUnique(commandBufferAI).front());
        vkx::setDebugName(*pDevice, *pCommandBuffer, fmt::format("[{0}]", index));
        cmdBuffers.push_back(std::move(pCommandBuffer));
    }

    std::vector<vk::UniqueDescriptorPool> descriptorPools;
    for (size_t index = 0; index < FRAMES_IN_FLIGHT; index++) {
        vk::DescriptorPoolSize descriptorPoolSize[] = {
            vk::DescriptorPoolSize {
                .type = vk::DescriptorType::eUniformBufferDynamic,
                .descriptorCount = 1
            }
        };

        vk::DescriptorPoolCreateInfo descriptorPoolCI = {
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1,
            .poolSizeCount = _countof(descriptorPoolSize),
            .pPoolSizes = descriptorPoolSize
        };
        auto pDescriptorPool = pDevice->createDescriptorPoolUnique(descriptorPoolCI);
        vkx::setDebugName(*pDevice, *pDescriptorPool, fmt::format("[{0}]", index));
        descriptorPools.push_back(std::move(pDescriptorPool));
    }

    std::vector<vk::UniqueDescriptorSet> descriptorSets;
    for (size_t index = 0; index < FRAMES_IN_FLIGHT; index++) {
        vk::DescriptorSetLayout descriptorSetLayouts[] = {
            *pDescriptorSetLayout
        };
   
        vk::DescriptorSetAllocateInfo descriptorSetAI = {
            .descriptorPool = *descriptorPools[index],
            .descriptorSetCount = _countof(descriptorSetLayouts),
            .pSetLayouts = descriptorSetLayouts
        };
        auto pDescriptorSet = std::move(pDevice->allocateDescriptorSetsUnique(descriptorSetAI).front());
        vkx::setDebugName(*pDevice, *pDescriptorSet, fmt::format("[{0}]", index));
        descriptorSets.push_back(std::move(pDescriptorSet));
    }
   
    std::vector<std::tuple<vk::UniqueBuffer, vma::UniqueAllocation, vma::AllocationInfo>> uniformBuffers;
    for (size_t index = 0; index < FRAMES_IN_FLIGHT; index++) {   
        vk::BufferCreateInfo bufferCI = {
            .size = 128 << 10,
            .usage = vk::BufferUsageFlagBits::eUniformBuffer,
            .sharingMode = vk::SharingMode::eExclusive,
        };
            
        vma::AllocationCreateInfo allocationCI = {
            .flags = vma::AllocationCreateFlagBits::eMapped,
            .usage = vma::MemoryUsage::eCpuToGpu,
        };
    
        vma::AllocationInfo allocationInfo = {};
        auto [pBuffer, pAllocation] = pAllocator->createBufferUnique(bufferCI, allocationCI, allocationInfo);
    
        vkx::setDebugName(*pDevice, *pBuffer, fmt::format("UniformBuffer[{0}]", index));
        uniformBuffers.emplace_back(std::move(pBuffer), std::move(pAllocation), allocationInfo);
    }
    
    for (size_t index = 0; index < FRAMES_IN_FLIGHT; index++) {
        vk::DescriptorBufferInfo descriptorsBufferInfo[] = {
            vk::DescriptorBufferInfo { .buffer = *std::get<0>(uniformBuffers[index]), .offset = 0, .range = 16 * sizeof(float) }
        };
    
        vk::WriteDescriptorSet writeDescriptorSet = {
            .dstSet = *descriptorSets[index],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = _countof(descriptorsBufferInfo),
            .descriptorType = vk::DescriptorType::eUniformBufferDynamic,
            .pImageInfo = nullptr,
            .pBufferInfo = descriptorsBufferInfo,
            .pTexelBufferView = nullptr,
        };
        pDevice->updateDescriptorSets(writeDescriptorSet, {});
    }

    std::vector<vk::UniqueSemaphore> swapChainSemaphoresAvailable;
    std::vector<vk::UniqueSemaphore> swpaChainSemaphoresFinished;
    std::vector<vk::UniqueFence>     swapChainFences;   

    for (size_t index = 0; index < FRAMES_IN_FLIGHT; index++) {
        auto pSemaphoresAvailable = pDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
        auto pSemaphoresFinished  = pDevice->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
        auto pFence = pDevice->createFenceUnique(vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
        
        vkx::setDebugName(*pDevice, *pSemaphoresAvailable, fmt::format("Available[{0}]", index));
        vkx::setDebugName(*pDevice, *pSemaphoresFinished,  fmt::format("Finished[{0}]", index));
        vkx::setDebugName(*pDevice, *pFence, fmt::format("[{0}]", index));
        
        swapChainSemaphoresAvailable.push_back(std::move(pSemaphoresAvailable));
        swpaChainSemaphoresFinished.push_back((std::move(pSemaphoresFinished)));
        swapChainFences.push_back(std::move(pFence));  
    }
    
    vk::UniqueSurfaceKHR pSurface; {
        vk::Win32SurfaceCreateInfoKHR surfaceCI = {
            .hinstance = GetModuleHandleA(nullptr),
            .hwnd = glfwGetWin32Window(pWindow.get())
        };
        pSurface = pInstance->createWin32SurfaceKHRUnique(surfaceCI);
        vkx::setDebugName(*pDevice, *pSurface, "Win32SurfaceKHR");
    }
    vk::UniqueSwapchainKHR pSwapChain;
    vk::UniqueFramebuffer  pFrameBuffer;
    std::vector<vk::UniqueImageView> pSwapChainImageViews;
    
    vk::UniqueImage pDepthImage;
    vk::UniqueImageView pDepthImageView;
    vma::UniqueAllocation pDepthImageAllocation;
  
    auto const ResizeRenderTargets = [&](uint32_t width, uint32_t height)-> void {
        
        std::vector<vk::Fence> fences;
        std::transform(std::begin(swapChainFences), std::end(swapChainFences), std::back_inserter(fences), [](vk::UniqueFence& fence) -> vk::Fence { return *fence; });
      
        std::ignore = pDevice->waitForFences(fences, true, std::numeric_limits<uint64_t>::max());
        pFrameBuffer.reset();
        pSwapChainImageViews.clear();
        pDepthImageView.reset();
        pDepthImage.reset();
        pDepthImageAllocation.reset();
        pSwapChain.reset();

        {
            vk::SwapchainCreateInfoKHR swapchainCI = {
                .surface = *pSurface,
                .minImageCount = BUFFER_COUNT,
                .imageFormat = vk::Format::eB8G8R8A8Srgb,
                .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
                .imageExtent = {.width = width, .height = height },
                .imageArrayLayers = 1,
                .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &indexQueueFamilyGraphics,
                .presentMode = vk::PresentModeKHR::eImmediate,
                .clipped = true,
                .oldSwapchain = *pSwapChain
            };
            physicalDevice.getSurfaceSupportKHR(indexQueueFamilyGraphics, *pSurface);
            pSwapChain = pDevice->createSwapchainKHRUnique(swapchainCI);
            vkx::setDebugName(*pDevice, *pSwapChain, fmt::format("Format: {0} PresentMode: {1}\n", vk::to_string(swapchainCI.imageFormat), vk::to_string(swapchainCI.presentMode) ));
        }
        
        auto const swapChainImages = pDevice->getSwapchainImagesKHR(*pSwapChain);
        {          
            for (size_t index = 0; index < BUFFER_COUNT; index++) {
                vk::ImageViewCreateInfo imageViewCI = {
                    .image = swapChainImages[index],
                    .viewType = vk::ImageViewType::e2D,
                    .format = vk::Format::eB8G8R8A8Srgb,
                    .subresourceRange = {
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .baseMipLevel = 0,
                        .levelCount = VK_REMAINING_MIP_LEVELS,
                        .baseArrayLayer = 0,
                        .layerCount = VK_REMAINING_ARRAY_LAYERS
                    }
                };
                auto pImageView = pDevice->createImageViewUnique(imageViewCI);
                vkx::setDebugName(*pDevice, swapChainImages[index], fmt::format("ColorBuffer[{0}]", index));
                vkx::setDebugName(*pDevice, *pImageView, fmt::format("ColorBuffer[{0}]", index));
                pSwapChainImageViews.push_back(std::move(pImageView));
            }
        }

        {
            vk::ImageCreateInfo imageCI = {
                .imageType = vk::ImageType::e2D,
                .format = vk::Format::eD32SfloatS8Uint,
                .extent = {.width = width, .height = height, .depth = 1},
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = vk::SampleCountFlagBits::e1,
                .tiling = vk::ImageTiling::eOptimal,
                .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                .sharingMode = vk::SharingMode::eExclusive,
            };

            vma::AllocationCreateInfo allocationCI = {
                 .usage = vma::MemoryUsage::eGpuOnly 
            };

            std::tie(pDepthImage, pDepthImageAllocation) = pAllocator->createImageUnique(imageCI, allocationCI);
            vkx::setDebugName(*pDevice, *pDepthImage, "DepthBuffer");

            vk::ImageViewCreateInfo imageViewCI = {
                .image = *pDepthImage,
                .viewType = vk::ImageViewType::e2D,
                .format = vk::Format::eD32SfloatS8Uint,
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eDepth,
                    .baseMipLevel = 0,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS
                },
            };
            pDepthImageView = pDevice->createImageViewUnique(imageViewCI);
            vkx::setDebugName(*pDevice, *pDepthImageView, "DepthBuffer");
        }
   
        {
            vk::Format colorFormats[] = { vk::Format::eB8G8R8A8Srgb };
            vk::Format depthFormats[] = { vk::Format::eD32SfloatS8Uint };

            vk::FramebufferAttachmentImageInfo framebufferAttanchments[] = {
                vk::FramebufferAttachmentImageInfo{
                    .usage = vk::ImageUsageFlagBits::eColorAttachment,
                    .width = width,
                    .height = height,
                    .layerCount = 1,
                    .viewFormatCount = _countof(colorFormats),
                    .pViewFormats = colorFormats    
                },
                vk::FramebufferAttachmentImageInfo {
                    .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    .width = width,
                    .height = height,
                    .layerCount = 1,
                    .viewFormatCount = _countof(depthFormats),
                    .pViewFormats = depthFormats
                },
            };
            
            vk::StructureChain<vk::FramebufferCreateInfo, vk::FramebufferAttachmentsCreateInfo> framebufferCI = {
                vk::FramebufferCreateInfo{ 
                    .flags = vk::FramebufferCreateFlagBits::eImageless,
                    .renderPass = *pRenderPass,
                    .attachmentCount = _countof(framebufferAttanchments),
                    .width = width,
                    .height = height,
                    .layers = 1
                },
                vk::FramebufferAttachmentsCreateInfo{ 
                    .attachmentImageInfoCount = _countof(framebufferAttanchments),
                    .pAttachmentImageInfos = framebufferAttanchments
                }
            };

            pFrameBuffer = pDevice->createFramebufferUnique(framebufferCI.get<vk::FramebufferCreateInfo>());     
            vkx::setDebugName(*pDevice, *pFrameBuffer, "SwapChain");
        }
    };
  
    ResizeRenderTargets(WINDOW_WIDTH, WINDOW_HEIGHT);

    vk::Queue commandQueueGraphics = pDevice->getQueue(indexQueueFamilyGraphics, 0);
    vk::Queue commandQueueCompute  = pDevice->getQueue(indexQueueFamilyCompute,  0);
    vk::Queue commandQueueTransfer = pDevice->getQueue(indexQueueFamilyTransfer, 0);

    vkx::setDebugName(*pDevice, commandQueueGraphics, fmt::format("Graphics [{0}]", 0));
    vkx::setDebugName(*pDevice, commandQueueCompute,  fmt::format("Compute  [{0}]", 0));
    vkx::setDebugName(*pDevice, commandQueueTransfer, fmt::format("Transfer [{0}]", 0));

    std::vector<vk::UniqueQueryPool> queryPools;
    for (size_t index = 0; index < FRAMES_IN_FLIGHT; index++) {
        vk::QueryPoolCreateInfo queryPoolCI = {
            .queryType = vk::QueryType::eTimestamp,
            .queryCount = 2
        };
        auto pQueryPool = pDevice->createQueryPoolUnique(queryPoolCI);
        queryPools.push_back(std::move(pQueryPool));
    }


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("content/fonts/Roboto-Medium.ttf", 15.0f);
    ImGui::StyleColorsDark();
    ImPlot::GetStyle().AntiAliasedLines = true;

    ImGui_ImplGlfw_InitForVulkan(pWindow.get(), true);
    ImGui_ImplVulkan_Init(*pDevice, physicalDevice, *pAllocator, *pRenderPass, FRAMES_IN_FLIGHT);

   
    float CPUFrameTime = 0.0f;
    float GPUFrameTime = 0.0f;

    while (!glfwWindowShouldClose(pWindow.get())) {
        auto const timestampT0 = std::chrono::high_resolution_clock::now();

        glfwPollEvents();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        if (ImGui::Begin("Debug")) {      
            if (ImGui::CollapsingHeader("Frame Statistic")) {
                auto FillRangeRounded = [](ImVector<double>& buffer, size_t count, double vmin, double vmax, int32_t p) -> void {
                    buffer.resize(count);
                    double step = (vmax - vmin) / (count - 1);
                    for (size_t index = 0; index < count; index++) {
                        buffer[index] = std::round(std::pow(10, p) * (vmin + index * step)) / std::pow(10, p);
                    }
                };

                static ScrollingBuffer scrollingCPUData{};
                static ScrollingBuffer scrollingGPUData{};
                ImVector<double> customTicks;

                static uint64_t frameIndex = 0;
                static float totalTime = 0;
                static float history = 5.0f;

                float maxTimeCPU = scrollingCPUData.ComputeMax(history);
                float maxTimeGPU = scrollingGPUData.ComputeMax(history);
                float maxTime = 1.5f * std::max(maxTimeGPU, maxTimeCPU);
                float averageTime = scrollingCPUData.ComputeAverage(history);

                scrollingGPUData.AddPoint(totalTime, GPUFrameTime);
                scrollingCPUData.AddPoint(totalTime, CPUFrameTime);
                FillRangeRounded(customTicks, 5, 0.0, maxTime, 1);

                ImGui::BulletText("CPU Time: %.1fms", CPUFrameTime);
                ImGui::BulletText("GPU Time: %.1fms", GPUFrameTime);
                ImGui::BulletText("Average FPS: %.0f", 1000.0f * (1.0f / averageTime));

                if (ImGui::TreeNode("Show Graphics ##Frame Statistic")) { 
                    ImGui::SliderFloat("History", &history, 1, 10, "%.1fs");
                    ImPlot::SetNextPlotLimitsX(totalTime - history, totalTime, ImGuiCond_Always);
                    ImPlot::SetNextPlotLimitsY(0.0, maxTime, ImGuiCond_Always);
                    ImPlot::SetNextPlotTicksY(std::begin(customTicks), std::size(customTicks));
                    if (ImPlot::BeginPlot("##FrameRate", 0, 0, ImVec2(-1, 175), 0, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_None | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks)) {
                        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                        ImPlot::PlotShaded("CPU Time", &scrollingCPUData.Data[0].x, &scrollingCPUData.Data[0].y, static_cast<int32_t>(std::size(scrollingCPUData.Data)), 0, scrollingCPUData.Offset, sizeof(ImVec2));
                        ImPlot::PlotShaded("GPU Time", &scrollingGPUData.Data[0].x, &scrollingGPUData.Data[0].y, static_cast<int32_t>(std::size(scrollingGPUData.Data)), 0, scrollingGPUData.Offset, sizeof(ImVec2));
                        ImPlot::PopStyleVar();
                        ImPlot::PlotLine("CPU Time", &scrollingCPUData.Data[0].x, &scrollingCPUData.Data[0].y, static_cast<int32_t>(std::size(scrollingCPUData.Data)), scrollingCPUData.Offset, sizeof(ImVec2));
                        ImPlot::PlotLine("GPU Time", &scrollingGPUData.Data[0].x, &scrollingGPUData.Data[0].y, static_cast<int32_t>(std::size(scrollingGPUData.Data)), scrollingGPUData.Offset, sizeof(ImVec2));
                        ImPlot::EndPlot();
                    }
                    ImGui::TreePop();
                }

                totalTime += CPUFrameTime / 1000.0f;
                frameIndex++;
            }
      
            if (ImGui::CollapsingHeader("Memory Statistic")) {
                auto memoryType = [](MemoryStatisticGPU::HeapType type) -> std::string {
                    switch (type) {
                        case MemoryStatisticGPU::HeapType::DeviceMemory: return "Device Memory";
                        case MemoryStatisticGPU::HeapType::SystemMemory: return "System Memory";
                        default: return "Invalid";
                    }
                };
                
                float maxMemorySize = 0.0f;
                float maxMemoryTime = 0.0f;

                for (auto const& heap : memoryGPU.GetMemoryStatistic()) {
                    auto id = fmt::format("Memory Heap: [{0}] Type: {1}", heap.MemoryIndex, memoryType(heap.MemoryType));
                    if (ImGui::TreeNode(id.c_str())) {
                        ImGui::BulletText("Memory size: %imb", heap.MemorySize >> 20);
                        ImGui::BulletText("Memory used: %imb", heap.MemoryUsed >> 20);
                        ImGui::BulletText("Allocations count: %i", heap.MemoryCount);     
                        ImGui::TreePop();
                    }    
                    maxMemorySize = std::max(maxMemorySize, static_cast<float>(heap.MemorySize >> 20));
                    maxMemoryTime = std::max(maxMemoryTime, static_cast<float>(heap.MemoryFrames.back().TimePoint));
                }  
               
                if (ImGui::TreeNode("Show Graphics ##Memory Statistic")) {
                    ImPlot::SetNextPlotLimitsX(0.0f, maxMemoryTime, ImGuiCond_Always);
                    ImPlot::SetNextPlotLimitsY(0.0f, maxMemorySize, ImGuiCond_Always);
                    ImPlot::SetNextPlotTicksY(0.0f, maxMemorySize, 5, nullptr);

                    if (ImPlot::BeginPlot("##MemoryStatistic", 0, 0, ImVec2(-1, 175), 0, ImPlotAxisFlags_None, ImPlotAxisFlags_None)) {
                        for (auto const& heap : memoryGPU.GetMemoryStatistic()) {
                            std::vector<ImVec2> memoryFrames{};
                            std::transform(std::begin(heap.MemoryFrames), std::end(heap.MemoryFrames), std::back_inserter(memoryFrames), [](auto x) -> ImVec2 { return ImVec2(static_cast<float>(x.TimePoint), static_cast<float>(x.MemoryUsed >> 20)); });

                            auto id = fmt::format("Heap: [{0}] Type: {1}", heap.MemoryIndex, memoryType(heap.MemoryType));
                            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
                            ImPlot::PlotShaded(id.c_str(), &memoryFrames[0].x, &memoryFrames[0].y, static_cast<int32_t>(std::size(memoryFrames)), 0, 0, sizeof(ImVec2));
                            ImPlot::PopStyleVar();
                            ImPlot::PlotLine(id.c_str(), &memoryFrames[0].x, &memoryFrames[0].y, static_cast<int32_t>(std::size(memoryFrames)), 0, sizeof(ImVec2));
                            ImPlot::PlotScatter(id.c_str(), &memoryFrames[0].x, &memoryFrames[0].y, static_cast<int32_t>(std::size(memoryFrames)), 0, sizeof(ImVec2));
                        }
                        ImPlot::EndPlot();
                    }
                    ImGui::TreePop();
                }      
            }              
        }

        ImGui::End();
        ImGui::Render();
     
        int32_t width  = 0;
        int32_t height = 0;
        glfwGetWindowSize(pWindow.get(), &width, &height);

        static uint32_t currentFrame = 0;
        auto [_, index] = pDevice->acquireNextImageKHR(pSwapChain.get(), std::numeric_limits<uint64_t>::max(), *swapChainSemaphoresAvailable[currentFrame], nullptr);
        
        std::ignore = pDevice->waitForFences({ *swapChainFences[currentFrame] }, true, std::numeric_limits<uint64_t>::max());
        std::ignore = pDevice->resetFences({ *swapChainFences[currentFrame] });
  
        vk::ClearValue clearValues[] = {
            vk::ClearValue{ vk::ClearColorValue{ std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } }},
            vk::ClearValue{ vk::ClearDepthStencilValue{ .depth = 1.0f, .stencil = 0 } }
        };

        vk::Rect2D   scissor  = { .offset = { .x = 0, .y = 0 }, .extent = { .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) } };
        vk::Viewport viewport = { .x = 0, .y = static_cast<float>(height), .width = static_cast<float>(width), .height = -static_cast<float>(height), .minDepth = 0.0f, .maxDepth = 1.0f };

        vk::UniqueCommandBuffer& pCmdBuffer = cmdBuffers[currentFrame];
        vk::UniqueQueryPool& pQueryPool = queryPools[currentFrame];
        
        pCmdBuffer->begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse });
        pCmdBuffer->resetQueryPool(*pQueryPool, 0, 2);
        pCmdBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *pQueryPool, 0);
        {
            vkx::DebugUtilsLabelScoped debug{ *pCmdBuffer, "Begin Frame", std::array<float, 4>{ 1.0f, 0.0f, 0.0f, 1.0f } };
            
            vk::ImageView renderPassAttachments[] = {
                *pSwapChainImageViews[index],
                *pDepthImageView
            };

            vk::StructureChain<vk::RenderPassBeginInfo, vk::RenderPassAttachmentBeginInfo> renderPassBeginInfo = {
                vk::RenderPassBeginInfo { 
                    .renderPass = *pRenderPass, 
                    .framebuffer = *pFrameBuffer, 
                    .renderArea = scissor, 
                    .clearValueCount = _countof(clearValues), 
                    .pClearValues = clearValues
                },
                vk::RenderPassAttachmentBeginInfo { 
                    .attachmentCount = _countof(renderPassAttachments),
                    .pAttachments = renderPassAttachments
                }
            };
           
            pCmdBuffer->beginRenderPass(renderPassBeginInfo.get<vk::RenderPassBeginInfo>(), vk::SubpassContents::eInline);
            
            {
                vkx::DebugUtilsLabelScoped debug{ *pCmdBuffer, "Render Color", std::array<float, 4>{ 0.0f, 1.0f, 0.0f, 1.0f } };
                pCmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *pPipeline);
                pCmdBuffer->setScissor(0, { scissor });
                pCmdBuffer->setViewport(0, { viewport });
                pCmdBuffer->draw(3, 1, 0, 0);
            }  

            {
                vkx::DebugUtilsLabelScoped debug{ *pCmdBuffer, "Render GUI", std::array<float, 4>{ 0.0f, 0.0f, 1.0f, 1.0f } };
                ImGui_ImplVulkan_NewFrame(*pCmdBuffer, currentFrame);
            }
            pCmdBuffer->endRenderPass();
        }
        pCmdBuffer->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *pQueryPool, 1);
        pCmdBuffer->end();
        
        vk::PipelineStageFlags waitStageMask[]  = {
            vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        };

        vk::SubmitInfo submitInfo = {
            .waitSemaphoreCount = 1, 
            .pWaitSemaphores = swapChainSemaphoresAvailable[currentFrame].getAddressOf(),
            .pWaitDstStageMask  = waitStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = pCmdBuffer.getAddressOf(),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = swpaChainSemaphoresFinished[currentFrame].getAddressOf()
            
        };    
        
        commandQueueGraphics.submit({ submitInfo }, *swapChainFences[currentFrame]);

        std::ignore = commandQueueGraphics.presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = swpaChainSemaphoresFinished[currentFrame].getAddressOf(),
            .swapchainCount = 1,
            .pSwapchains = pSwapChain.getAddressOf(),
            .pImageIndices = &index
        });    
       
      
        if (auto result = pDevice->getQueryPoolResults<uint64_t>(*pQueryPool, 0, 2, 2 * sizeof(uint64_t), sizeof(uint64_t), vk::QueryResultFlagBits::e64); result.result == vk::Result::eSuccess) 
            GPUFrameTime = physicalDevice.getProperties().limits.timestampPeriod * (result.value[1] - result.value[0]) / 1E6f;
        
        auto const timestampT1 = std::chrono::high_resolution_clock::now();
        CPUFrameTime = std::chrono::duration<float, std::milli>(timestampT1 - timestampT0).count();
        currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
    }

    {
        std::vector<uint8_t> cacheData = pDevice->getPipelineCacheData(*pPipelineCache);
        std::unique_ptr<FILE, decltype(&std::fclose)> pFile(std::fopen("PipelineCache", "wb"), std::fclose);
        std::fwrite(std::data(cacheData), sizeof(uint8_t), std::size(cacheData), pFile.get());
    }
  

    pDevice->waitIdle();
    

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}