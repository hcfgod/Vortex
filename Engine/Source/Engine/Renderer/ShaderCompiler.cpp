#include "vxpch.h"
#include "ShaderCompiler.h"

#include <shaderc/shaderc.hpp>
#include <spirv-tools/libspirv.hpp>

namespace Vortex {

    class ShaderCompiler::Impl
    {
    public:
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        Impl()
        {
            // Set optimization level for release builds
            #ifdef VX_RELEASE
                options.SetOptimizationLevel(shaderc_optimization_level_performance);
            #else
                options.SetOptimizationLevel(shaderc_optimization_level_zero);
            #endif

            // Generate debug info for debug builds
            #ifdef VX_DEBUG
                options.SetGenerateDebugInfo();
            #endif
        }

        static shaderc_shader_kind GetShadercKind(ShaderStage stage)
        {
            switch (stage)
            {
            case ShaderStage::Vertex:              return shaderc_vertex_shader;
            case ShaderStage::Fragment:            return shaderc_fragment_shader;
            case ShaderStage::Geometry:            return shaderc_geometry_shader;
            case ShaderStage::TessellationControl: return shaderc_tess_control_shader;
            case ShaderStage::TessellationEvaluation: return shaderc_tess_evaluation_shader;
            case ShaderStage::Compute:             return shaderc_compute_shader;
            default:
                return shaderc_vertex_shader;
            }
        }
    };

    ShaderCompiler::ShaderCompiler()
        : m_Impl(std::make_unique<Impl>())
    {
    }

    ShaderCompiler::~ShaderCompiler() = default;

    CompiledShader ShaderCompiler::CompileGLSL(const std::string& source, 
                                               ShaderStage stage,
                                               const std::string& filename)
    {
        CompiledShader result;

        auto shadercKind = Impl::GetShadercKind(stage);
        auto compilationResult = m_Impl->compiler.CompileGlslToSpv(
            source, shadercKind, filename.c_str(), m_Impl->options);

        if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            result.Success = false;
            result.ErrorMessage = compilationResult.GetErrorMessage();
            return result;
        }

        result.Success = true;
        result.SpirV = std::vector<uint32_t>(compilationResult.cbegin(), compilationResult.cend());
        
        return result;
    }

    CompiledShader ShaderCompiler::CompileHLSL(const std::string& source, 
                                               ShaderStage stage,
                                               const std::string& entryPoint,
                                               const std::string& filename)
    {
        CompiledShader result;

        auto shadercKind = Impl::GetShadercKind(stage);
        
        // Set HLSL entry point
        shaderc::CompileOptions hlslOptions = m_Impl->options;
        hlslOptions.SetSourceLanguage(shaderc_source_language_hlsl);
        
        auto compilationResult = m_Impl->compiler.CompileGlslToSpv(
            source, shadercKind, filename.c_str(), entryPoint.c_str(), hlslOptions);

        if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            result.Success = false;
            result.ErrorMessage = compilationResult.GetErrorMessage();
            return result;
        }

        result.Success = true;
        result.SpirV = std::vector<uint32_t>(compilationResult.cbegin(), compilationResult.cend());
        
        return result;
    }

} // namespace Vortex
