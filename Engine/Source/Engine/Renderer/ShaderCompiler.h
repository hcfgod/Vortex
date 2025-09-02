#pragma once

#include <string>
#include <vector>
#include <memory>

namespace Vortex {

    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Geometry,
        TessellationControl,
        TessellationEvaluation,
        Compute
    };

    struct CompiledShader
    {
        std::vector<uint32_t> SpirV;
        std::string ErrorMessage;
        bool Success = false;
    };

    class ShaderCompiler
    {
    public:
        ShaderCompiler();
        ~ShaderCompiler();

        // Compile GLSL source code to SPIR-V
        CompiledShader CompileGLSL(const std::string& source, 
                                   ShaderStage stage,
                                   const std::string& filename = "shader");

        // Compile HLSL source code to SPIR-V
        CompiledShader CompileHLSL(const std::string& source, 
                                   ShaderStage stage,
                                   const std::string& entryPoint = "main",
                                   const std::string& filename = "shader");

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };

} // namespace Vortex
