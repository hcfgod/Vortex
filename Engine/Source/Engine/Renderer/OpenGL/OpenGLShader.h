#pragma once

#include "Engine/Renderer/Shader/Shader.h"
#include <glad/gl.h>

#include <unordered_map>
#include <string>

namespace Vortex
{
    /**
     * @brief OpenGL implementation of the Shader interface
     */
class OpenGLShader : public GPUShader
    {
    public:
        explicit OpenGLShader(const std::string& name = "");
        ~OpenGLShader() override;

        // ============================================================================
        // SHADER LIFECYCLE
        // ============================================================================

        Result<void> Create(const std::unordered_map<ShaderStage, std::vector<uint32_t>>& shaders,
                           const ShaderReflectionData& reflection) override;

        void Destroy() override;
        bool IsValid() const override;

        // ============================================================================
        // SHADER BINDING
        // ============================================================================

        void Bind() override;
        void Unbind() override;

        // ============================================================================
        // UNIFORM MANAGEMENT
        // ============================================================================

        Result<void> SetUniform(const std::string& name, const void* data, uint32_t size) override;

        Result<void> SetUniform(const std::string& name, int value) override;
        Result<void> SetUniform(const std::string& name, float value) override;
        Result<void> SetUniform(const std::string& name, const glm::vec2& value) override;
        Result<void> SetUniform(const std::string& name, const glm::vec3& value) override;
        Result<void> SetUniform(const std::string& name, const glm::vec4& value) override;
        Result<void> SetUniform(const std::string& name, const glm::mat3& value) override;
        Result<void> SetUniform(const std::string& name, const glm::mat4& value) override;

        Result<void> SetTexture(const std::string& name, uint32_t textureId, uint32_t slot = 0) override;
        Result<void> SetUniformBuffer(const std::string& name, uint32_t bufferId, uint32_t offset, uint32_t size) override;

        // ============================================================================
        // OPENGL SPECIFIC
        // ============================================================================

        /**
         * @brief Get OpenGL program ID
         * @return OpenGL program handle
         */
        GLuint GetProgramId() const { return m_ProgramId; }

        /**
         * @brief Get uniform location by name
         * @param name Uniform name
         * @return Uniform location, -1 if not found
         */
        GLint GetUniformLocation(const std::string& name) const;

        // ============================================================================
        // DEBUG AND UTILITIES
        // ============================================================================

        std::string GetDebugInfo() const override;

    private:
        // ============================================================================
        // PRIVATE METHODS
        // ============================================================================

        /**
         * @brief Convert SPIR-V to OpenGL shader source (via SPIRV-Cross)
         * @param spirv SPIR-V bytecode
         * @param stage Shader stage
         * @return OpenGL shader source code or error
         */
        Result<std::string> ConvertSpirVToGLSL(const std::vector<uint32_t>& spirv, 
                                              ShaderStage stage);

        /**
         * @brief Compile OpenGL shader from GLSL source
         * @param source GLSL source code
         * @param stage Shader stage
         * @return OpenGL shader handle or error
         */
        Result<GLuint> CompileShader(const std::string& source, ShaderStage stage);

        /**
         * @brief Link OpenGL shader program from compiled shaders
         * @param shaderIds Vector of compiled shader IDs
         * @return Success/failure result
         */
        Result<void> LinkProgram(const std::vector<GLuint>& shaderIds);

        /**
         * @brief Cache uniform locations from reflection data
         * @param reflection Shader reflection data containing uniforms
         */
        void CacheUniformLocationsFromReflection(const ShaderReflectionData& reflection);
        
        /**
         * @brief Extract additional uniform locations from compiled program
         */
        void CacheUniformLocations();
        
        /**
         * @brief Log detailed reflection information for debugging
         * @param reflection Shader reflection data
         */
        void LogReflectionInfo(const ShaderReflectionData& reflection);

        /**
         * @brief Convert shader stage to OpenGL shader type
         * @param stage Vortex shader stage
         * @return OpenGL shader type
         */
        static GLenum ShaderStageToGLType(ShaderStage stage);

        /**
         * @brief Get shader stage name for error reporting
         * @param stage Shader stage
         * @return Stage name string
         */
        static const char* GetShaderStageName(ShaderStage stage);

        // ============================================================================
        // MEMBER VARIABLES
        // ============================================================================

        GLuint m_ProgramId = 0;
        mutable std::unordered_map<std::string, GLint> m_UniformLocationCache;
        bool m_IsLinked = false;
    };

} // namespace Vortex
