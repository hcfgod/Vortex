#pragma once

#include "ShaderTypes.h"
#include "Core/Debug/ErrorCodes.h"
#include "Core/Common/UUID.h"

#include <string>
#include <memory>
#include <unordered_map>

namespace Vortex
{
    /**
     * @brief Render API agnostic shader interface
     * 
     * Represents a compiled shader program that can be bound for rendering.
     * Implementations handle the specifics for each graphics API (OpenGL, Vulkan, D3D12).
     */
    class GPUShader
    {
    public:
        GPUShader() = default;
        virtual ~GPUShader() = default;

        // Non-copyable, moveable
        GPUShader(const GPUShader&) = delete;
        GPUShader& operator=(const GPUShader&) = delete;
        GPUShader(GPUShader&&) = default;
        GPUShader& operator=(GPUShader&&) = default;

        // ============================================================================
        // SHADER LIFECYCLE
        // ============================================================================

        /**
         * @brief Create shader from compiled SPIR-V bytecode
         * @param shaders Map of shader stages to compiled SPIR-V
         * @param reflection Combined reflection data for all stages
         * @return Success/failure result
         */
        virtual Result<void> Create(const std::unordered_map<Vortex::Shader::ShaderStage, std::vector<uint32_t>>& shaders,
                                   const Vortex::Shader::ShaderReflectionData& reflection) = 0;

        /**
         * @brief Destroy shader and release GPU resources
         */
        virtual void Destroy() = 0;

        /**
         * @brief Check if shader is valid and ready to use
         * @return True if valid, false otherwise
         */
        virtual bool IsValid() const = 0;

        // ============================================================================
        // SHADER BINDING
        // ============================================================================

        /**
         * @brief Bind shader for rendering
         */
        virtual void Bind() = 0;

        /**
         * @brief Unbind shader
         */
        virtual void Unbind() = 0;

        // ============================================================================
        // UNIFORM MANAGEMENT
        // ============================================================================

        /**
         * @brief Set uniform value by name
         * @param name Uniform name
         * @param data Pointer to uniform data
         * @param size Data size in bytes
         * @return Success/failure result
         */
        virtual Result<void> SetUniform(const std::string& name, const void* data, uint32_t size) = 0;

        /**
         * @brief Convenient uniform setters for common types
         */
        virtual Result<void> SetUniform(const std::string& name, int value) = 0;
        virtual Result<void> SetUniform(const std::string& name, float value) = 0;
        virtual Result<void> SetUniform(const std::string& name, const glm::vec2& value) = 0;
        virtual Result<void> SetUniform(const std::string& name, const glm::vec3& value) = 0;
        virtual Result<void> SetUniform(const std::string& name, const glm::vec4& value) = 0;
        virtual Result<void> SetUniform(const std::string& name, const glm::mat3& value) = 0;
        virtual Result<void> SetUniform(const std::string& name, const glm::mat4& value) = 0;

        /**
         * @brief Set texture uniform
         * @param name Texture uniform name
         * @param textureId Texture handle
         * @param slot Texture slot
         * @return Success/failure result
         */
        virtual Result<void> SetTexture(const std::string& name, uint32_t textureId, uint32_t slot = 0) = 0;

        /**
         * @brief Set uniform buffer
         * @param name Buffer name or binding point
         * @param bufferId Buffer handle
         * @param offset Offset into buffer
         * @param size Buffer size
         * @return Success/failure result
         */
        virtual Result<void> SetUniformBuffer(const std::string& name, uint32_t bufferId, uint32_t offset, uint32_t size) = 0;

        // ============================================================================
        // SHADER INFORMATION
        // ============================================================================

        /**
         * @brief Get shader name/identifier
         * @return Shader name
         */
        const std::string& GetName() const { return m_Name; }

        /**
         * @brief Get unique shader ID
         * @return Shader UUID
         */
        const UUID& GetId() const { return m_Id; }

        /**
         * @brief Get shader reflection data
         * @return Reflection data
         */
        const Vortex::Shader::ShaderReflectionData& GetReflectionData() const { return m_ReflectionData; }

        /**
         * @brief Get shader stage flags
         * @return Bitmask of shader stages
         */
        Vortex::Shader::ShaderStageFlags GetStageFlags() const { return m_StageFlags; }

        /**
         * @brief Check if shader has a specific stage
         * @param stage Shader stage to check
         * @return True if shader has this stage
         */
        bool HasStage(Vortex::Shader::ShaderStage stage) const;

        /**
         * @brief Get debug information
         * @return Debug info string
         */
        virtual std::string GetDebugInfo() const = 0;

        // ============================================================================
        // FACTORY METHODS
        // ============================================================================

        /**
         * @brief Create shader for current graphics API
         * @param name Shader name
         * @return Unique pointer to shader
         */
        static std::unique_ptr<GPUShader> Create(const std::string& name = "");

    protected:
        /**
         * @brief Set shader metadata (called by implementations)
         */
        void SetMetadata(const std::string& name, 
                        const Vortex::Shader::ShaderReflectionData& reflection,
                        Vortex::Shader::ShaderStageFlags stageFlags);

    private:
        UUID m_Id;
        std::string m_Name;
        Vortex::Shader::ShaderReflectionData m_ReflectionData;
        Vortex::Shader::ShaderStageFlags m_StageFlags = 0;
    };

    /**
     * @brief Shared pointer type for shaders
     */
    using ShaderRef = std::shared_ptr<GPUShader>;

} // namespace Vortex