#include "vxpch.h"
#include "OpenGLShader.h"

#include "Core/Debug/Log.h"
#include "Engine/Renderer/Shader/ShaderReflection.h"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

// For SPIRV-Cross (stub for now - can be implemented when SPIRV-Cross is available)
#ifdef VX_SPIRV_CROSS_AVAILABLE
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#endif
#include <Engine/Renderer/RenderCommandQueue.h>

// Helper function for success result
namespace {
    inline Vortex::Result<void> Success() {
        return Vortex::Result<void>();
    }
}

namespace Vortex
{
    OpenGLShader::OpenGLShader(const std::string& name)
        : m_ProgramId(0), m_IsLinked(false)
    {
        // Set the shader name via the base class
        if (!name.empty())
        {
            Vortex::Shader::ShaderReflectionData emptyReflection{};
            SetMetadata(name, emptyReflection, 0);
        }
    }

    OpenGLShader::~OpenGLShader()
    {
        Destroy();
    }

    // ============================================================================
    // SHADER LIFECYCLE
    // ============================================================================

    Result<void> OpenGLShader::Create(const std::unordered_map<Vortex::Shader::ShaderStage, std::vector<uint32_t>>& shaders, const Vortex::Shader::ShaderReflectionData& reflection)
    {
        // Destroy existing shader if any
        Destroy();

        // Create OpenGL program
        m_ProgramId = glCreateProgram();
        if (m_ProgramId == 0)
        {
            VX_CORE_ERROR("OpenGLShader: Failed to create OpenGL program");
            return ErrorCode::RendererInitFailed;
        }

        // Convert SPIR-V to GLSL and compile each stage
        std::vector<GLuint> compiledShaders;
        compiledShaders.reserve(shaders.size());

        for (const auto& [stage, spirv] : shaders)
        {
            // Convert SPIR-V to GLSL
            auto glslResult = ConvertSpirVToGLSL(spirv, stage);
            if (!glslResult.IsSuccess())
            {
                // Cleanup compiled shaders
                for (GLuint shaderId : compiledShaders)
                {
                    glDeleteShader(shaderId);
                }
                glDeleteProgram(m_ProgramId);
                m_ProgramId = 0;
                return glslResult.GetErrorCode();
            }

            // Compile GLSL shader
            auto shaderResult = CompileShader(glslResult.GetValue(), stage);
            if (!shaderResult.IsSuccess())
            {
                // Cleanup compiled shaders
                for (GLuint shaderId : compiledShaders)
                {
                    glDeleteShader(shaderId);
                }
                glDeleteProgram(m_ProgramId);
                m_ProgramId = 0;
                return shaderResult.GetErrorCode();
            }

            compiledShaders.push_back(shaderResult.GetValue());
        }

        // Link program
        auto linkResult = LinkProgram(compiledShaders);
        
        // Cleanup individual shaders (they're now part of the program)
        for (GLuint shaderId : compiledShaders)
        {
            glDetachShader(m_ProgramId, shaderId);
            glDeleteShader(shaderId);
        }

        if (!linkResult.IsSuccess())
        {
            glDeleteProgram(m_ProgramId);
            m_ProgramId = 0;
            return linkResult.GetErrorCode();
        }

        m_IsLinked = true;

        // Cache uniform locations for faster access
        CacheUniformLocations();

        // Update shader metadata
        Vortex::Shader::ShaderStageFlags stageFlags = 0;
        for (const auto& [stage, _] : shaders)
        {
            stageFlags |= static_cast<uint32_t>(stage);
        }
        SetMetadata(GetName(), reflection, stageFlags);

        VX_CORE_INFO("OpenGLShader: Successfully created shader '{}' with program ID {}", GetName(), m_ProgramId);
        return Success();
    }

    void OpenGLShader::Destroy()
    {
        if (m_ProgramId != 0)
        {
            glDeleteProgram(m_ProgramId);
            m_ProgramId = 0;
        }
        m_IsLinked = false;
        m_UniformLocationCache.clear();
    }

    bool OpenGLShader::IsValid() const
    {
        return m_ProgramId != 0 && m_IsLinked;
    }

    // ============================================================================
    // SHADER BINDING
    // ============================================================================

    void OpenGLShader::Bind()
    {
        if (!IsValid())
        {
            VX_CORE_ERROR("OpenGLShader: Attempting to bind invalid shader '{}'", GetName());
            return;
        }

		GetRenderCommandQueue().BindShader(m_ProgramId, false);
    }

    void OpenGLShader::Unbind()
    {
        GetRenderCommandQueue().BindShader(0, false);
    }

    // ============================================================================
    // UNIFORM MANAGEMENT
    // ============================================================================

    Result<void> OpenGLShader::SetUniform(const std::string& name, const void* data, uint32_t size)
    {
        GLint location = GetUniformLocation(name);
        if (location == -1)
        {
            VX_CORE_WARN("OpenGLShader: Uniform '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        // This is a generic setter - the caller needs to ensure proper type handling
        // For now, we'll log a warning that this should use the typed setters
        VX_CORE_WARN("OpenGLShader: Generic SetUniform called for '{}'. Consider using typed setters.", name);
        return Success();
    }

    Result<void> OpenGLShader::SetUniform(const std::string& name, int value)
    {
        GLint location = GetUniformLocation(name);
        if (location == -1)
        {
            VX_CORE_WARN("OpenGLShader: Uniform '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        glUniform1i(location, value);
        return Success();
    }

    Result<void> OpenGLShader::SetUniform(const std::string& name, float value)
    {
        GLint location = GetUniformLocation(name);
        if (location == -1)
        {
            VX_CORE_WARN("OpenGLShader: Uniform '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        glUniform1f(location, value);
        return Success();
    }

    Result<void> OpenGLShader::SetUniform(const std::string& name, const glm::vec2& value)
    {
        GLint location = GetUniformLocation(name);
        if (location == -1)
        {
            VX_CORE_WARN("OpenGLShader: Uniform '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        glUniform2fv(location, 1, glm::value_ptr(value));
        return Success();
    }

    Result<void> OpenGLShader::SetUniform(const std::string& name, const glm::vec3& value)
    {
        GLint location = GetUniformLocation(name);
        if (location == -1)
        {
            VX_CORE_WARN("OpenGLShader: Uniform '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        glUniform3fv(location, 1, glm::value_ptr(value));
        return Success();
    }

    Result<void> OpenGLShader::SetUniform(const std::string& name, const glm::vec4& value)
    {
        GLint location = GetUniformLocation(name);
        if (location == -1)
        {
            VX_CORE_WARN("OpenGLShader: Uniform '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        glUniform4fv(location, 1, glm::value_ptr(value));
        return Success();
    }

    Result<void> OpenGLShader::SetUniform(const std::string& name, const glm::mat3& value)
    {
        GLint location = GetUniformLocation(name);
        if (location == -1)
        {
            VX_CORE_WARN("OpenGLShader: Uniform '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
        return Success();
    }

    Result<void> OpenGLShader::SetUniform(const std::string& name, const glm::mat4& value)
    {
        GLint location = GetUniformLocation(name);
        if (location == -1)
        {
            VX_CORE_WARN("OpenGLShader: Uniform '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        return Success();
    }

    Result<void> OpenGLShader::SetTexture(const std::string& name, uint32_t textureId, uint32_t slot)
    {
        GLint location = GetUniformLocation(name);
        if (location == -1)
        {
            VX_CORE_WARN("OpenGLShader: Texture uniform '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        // Bind texture to specified slot
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        // Set uniform to texture slot
        glUniform1i(location, static_cast<int>(slot));
        
        return Success();
    }

    Result<void> OpenGLShader::SetUniformBuffer(const std::string& name, uint32_t bufferId, uint32_t offset, uint32_t size)
    {
        // Get uniform block index
        GLuint blockIndex = glGetUniformBlockIndex(m_ProgramId, name.c_str());
        if (blockIndex == GL_INVALID_INDEX)
        {
            VX_CORE_WARN("OpenGLShader: Uniform block '{}' not found in shader '{}'", name, GetName());
            return ErrorCode::ResourceNotFound;
        }

        // Bind uniform buffer to a binding point (for simplicity, use the block index as binding point)
        GLuint bindingPoint = blockIndex;
        glUniformBlockBinding(m_ProgramId, blockIndex, bindingPoint);
        
        // Bind buffer range to binding point
        glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, bufferId, offset, size);
        
        return Success();
    }

    GLint OpenGLShader::GetUniformLocation(const std::string& name) const
    {
        auto it = m_UniformLocationCache.find(name);
        if (it != m_UniformLocationCache.end())
        {
            return it->second;
        }

        GLint location = glGetUniformLocation(m_ProgramId, name.c_str());
        m_UniformLocationCache[name] = location;
        return location;
    }

    // ============================================================================
    // DEBUG AND UTILITIES
    // ============================================================================

    std::string OpenGLShader::GetDebugInfo() const
    {
        std::string info = "OpenGL Shader '" + GetName() + "':\n";
        info += "  Program ID: " + std::to_string(m_ProgramId) + "\n";
        info += "  Linked: " + std::string(m_IsLinked ? "Yes" : "No") + "\n";
        info += "  Valid: " + std::string(IsValid() ? "Yes" : "No") + "\n";
        info += "  Cached Uniforms: " + std::to_string(m_UniformLocationCache.size()) + "\n";
        
        if (IsValid())
        {
            // Get additional program info
            GLint maxLength = 0;
            glGetProgramiv(m_ProgramId, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
            info += "  Max Attribute Name Length: " + std::to_string(maxLength) + "\n";
            
            GLint attributeCount = 0;
            glGetProgramiv(m_ProgramId, GL_ACTIVE_ATTRIBUTES, &attributeCount);
            info += "  Active Attributes: " + std::to_string(attributeCount) + "\n";
            
            GLint uniformCount = 0;
            glGetProgramiv(m_ProgramId, GL_ACTIVE_UNIFORMS, &uniformCount);
            info += "  Active Uniforms: " + std::to_string(uniformCount) + "\n";
        }
        
        return info;
    }

    // ============================================================================
    // PRIVATE METHODS
    // ============================================================================

    Result<std::string> OpenGLShader::ConvertSpirVToGLSL(const std::vector<uint32_t>& spirv, 
                                                         Vortex::Shader::ShaderStage stage)
    {
#ifdef VX_SPIRV_CROSS_AVAILABLE
        try
        {
            // Create SPIRV-Cross compiler
            spirv_cross::CompilerGLSL glsl(spirv);
            
            // Set options for OpenGL
            spirv_cross::CompilerGLSL::Options options;
            options.version = 450; // OpenGL 4.5 core
            options.es = false;
            options.vulkan_semantics = false;
            glsl.set_common_options(options);
            
            // Compile to GLSL
            std::string source = glsl.compile();
            
            VX_CORE_TRACE("OpenGLShader: Successfully converted SPIR-V to GLSL for {} stage", GetShaderStageName(stage));
            return source;
        }
        catch (const std::exception& e)
        {
            VX_CORE_ERROR("OpenGLShader: SPIR-V to GLSL conversion failed for {} stage: {}", GetShaderStageName(stage), e.what());
            return ErrorCode::ShaderCompilationFailed;
        }
#else
        // Fallback: Create minimal GLSL shaders for testing
        VX_CORE_TRACE("OpenGLShader: SPIRV-Cross not available, using fallback GLSL generation");
        
        switch (stage)
        {
            case Vortex::Shader::ShaderStage::Vertex:
                return std::string(R"(
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 0) out vec2 v_TexCoord;

layout(location = 0) uniform float u_Time;
layout(location = 1) uniform mat4 u_ViewProjection;
layout(location = 2) uniform mat4 u_Transform;

void main()
{
    v_TexCoord = a_TexCoord;
    
    // Apply basic transformation
    vec4 worldPos = u_Transform * vec4(a_Position, 1.0);
    
    // Simple rotation animation
    float angle = u_Time * 0.5;
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    worldPos.xy = rotation * worldPos.xy;
    
    gl_Position = u_ViewProjection * worldPos;
}
)");
                
            case Vortex::Shader::ShaderStage::Fragment:
                return std::string(R"(
#version 450 core

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 FragColor;

layout(location = 0) uniform float u_Time;
layout(location = 3) uniform vec3 u_Color;
layout(location = 4) uniform float u_Alpha;

void main()
{
    // Create animated effects
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(v_TexCoord, center);
    
    // Pulsing effect
    float pulse = sin(u_Time * 2.0) * 0.5 + 0.5;
    
    // Color gradient
    vec3 gradient = mix(u_Color, u_Color * 1.5, v_TexCoord.x);
    gradient = mix(gradient, gradient * 0.7, dist);
    gradient = mix(gradient * 0.8, gradient, pulse);
    
    FragColor = vec4(gradient, u_Alpha);
}
)");
                
            default:
                VX_CORE_ERROR("OpenGLShader: Unsupported shader stage for fallback GLSL generation");
                return ErrorCode::ShaderCompilationFailed;
        }
#endif
    }

    Result<GLuint> OpenGLShader::CompileShader(const std::string& source, Vortex::Shader::ShaderStage stage)
    {
        GLenum glType = ShaderStageToGLType(stage);
        GLuint shaderId = glCreateShader(glType);
        
        if (shaderId == 0)
        {
            VX_CORE_ERROR("OpenGLShader: Failed to create {} shader", GetShaderStageName(stage));
            return ErrorCode::RendererInitFailed;
        }

        const char* sourceCStr = source.c_str();
        glShaderSource(shaderId, 1, &sourceCStr, nullptr);
        glCompileShader(shaderId);

        // Check compilation status
        GLint success;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
        
        if (!success)
        {
            GLint maxLength = 0;
            glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);
            
            std::vector<char> infoLog(maxLength);
            glGetShaderInfoLog(shaderId, maxLength, &maxLength, &infoLog[0]);
            
            VX_CORE_ERROR("OpenGLShader: {} shader compilation failed:\n{}", GetShaderStageName(stage), infoLog.data());
            
            glDeleteShader(shaderId);
            return ErrorCode::ShaderCompilationFailed;
        }

        VX_CORE_TRACE("OpenGLShader: {} shader compiled successfully", GetShaderStageName(stage));
        return shaderId;
    }

    Result<void> OpenGLShader::LinkProgram(const std::vector<GLuint>& shaderIds)
    {
        // Attach all shaders
        for (GLuint shaderId : shaderIds)
        {
            glAttachShader(m_ProgramId, shaderId);
        }

        // Link program
        glLinkProgram(m_ProgramId);

        // Check link status
        GLint success;
        glGetProgramiv(m_ProgramId, GL_LINK_STATUS, &success);
        
        if (!success)
        {
            GLint maxLength = 0;
            glGetProgramiv(m_ProgramId, GL_INFO_LOG_LENGTH, &maxLength);
            
            std::vector<char> infoLog(maxLength);
            glGetProgramInfoLog(m_ProgramId, maxLength, &maxLength, &infoLog[0]);
            
            VX_CORE_ERROR("OpenGLShader: Program linking failed:\n{}", infoLog.data());
            return ErrorCode::ShaderCompilationFailed;
        }

        VX_CORE_TRACE("OpenGLShader: Program linked successfully");
        return Success();
    }

    void OpenGLShader::CacheUniformLocations()
    {
        if (!IsValid()) return;

        GLint uniformCount;
        glGetProgramiv(m_ProgramId, GL_ACTIVE_UNIFORMS, &uniformCount);

        GLint maxNameLength;
        glGetProgramiv(m_ProgramId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);

        for (GLint i = 0; i < uniformCount; ++i)
        {
            std::vector<char> nameBuffer(maxNameLength);
            GLsizei length;
            GLint size;
            GLenum type;
            
            glGetActiveUniform(m_ProgramId, static_cast<GLuint>(i), maxNameLength, &length, &size, &type, nameBuffer.data());
            
            std::string name(nameBuffer.data(), length);
            GLint location = glGetUniformLocation(m_ProgramId, name.c_str());
            
            if (location != -1)
            {
                m_UniformLocationCache[name] = location;
            }
        }

        VX_CORE_TRACE("OpenGLShader: Cached {} uniform locations", m_UniformLocationCache.size());
    }

    GLenum OpenGLShader::ShaderStageToGLType(Vortex::Shader::ShaderStage stage)
    {
        switch (stage)
        {
            case Vortex::Shader::ShaderStage::Vertex:    return GL_VERTEX_SHADER;
            case Vortex::Shader::ShaderStage::Fragment:  return GL_FRAGMENT_SHADER;
            case Vortex::Shader::ShaderStage::Geometry:  return GL_GEOMETRY_SHADER;
            case Vortex::Shader::ShaderStage::Compute:   return GL_COMPUTE_SHADER;
            case Vortex::Shader::ShaderStage::TessellationControl: return GL_TESS_CONTROL_SHADER;
            case Vortex::Shader::ShaderStage::TessellationEvaluation: return GL_TESS_EVALUATION_SHADER;
            default:
                VX_CORE_ERROR("OpenGLShader: Unknown shader stage");
                return GL_VERTEX_SHADER;
        }
    }

    const char* OpenGLShader::GetShaderStageName(Vortex::Shader::ShaderStage stage)
    {
        switch (stage)
        {
            case Vortex::Shader::ShaderStage::Vertex:    return "Vertex";
            case Vortex::Shader::ShaderStage::Fragment:  return "Fragment";
            case Vortex::Shader::ShaderStage::Geometry:  return "Geometry";
            case Vortex::Shader::ShaderStage::Compute:   return "Compute";
            case Vortex::Shader::ShaderStage::TessellationControl: return "Tessellation Control";
            case Vortex::Shader::ShaderStage::TessellationEvaluation: return "Tessellation Evaluation";
            default: return "Unknown";
        }
    }

} // namespace Vortex
