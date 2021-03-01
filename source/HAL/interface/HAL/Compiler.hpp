#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    struct CompilerCreateInfo {

    };

    class Compiler {
    public:
        class Internal;
    public:
        Compiler(CompilerCreateInfo const& createInfo);
             
    private:    
        Internal_Ptr<Internal, InternalSize_Compiler> m_pInternal;   
    };

}