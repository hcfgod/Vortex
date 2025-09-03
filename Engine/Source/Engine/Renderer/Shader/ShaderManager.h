#pragma once

#include "ShaderTypes.h"
#include "ShaderCompiler.h"
#include "Core/Debug/ErrorCodes.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>

namespace Vortex
{
    /**
     * @brief Shader resource handle for efficient lookup and management
     */
    class ShaderHandle
    {
    public:
        ShaderHandle() = default;
        explicit ShaderHandle(uint64_t id) : m_Id(id) {}
        
        bool IsValid() const { return m_Id != 0; }
        uint64_t GetId() const { return m_Id; }
        
        bool operator==(const ShaderHandle& other) const { return m_Id == other.m_Id; }
        bool operator!=(const ShaderHandle& other) const { return m_Id != other.m_Id; }
        
    private:
        uint64_t m_Id = 0;
    };

    /**
     * @brief High-level shader resource manager
     * 
     * Provides:
     * - Shader asset loading and management
     * - Automatic recompilation and hot-reload
     * - Shader program binding and state management
     * - Uniform buffer management
     * - Material system integration
     * - Thread-safe resource access
     */
    class ShaderManager
    {
    public:
        ShaderManager();
        ~ShaderManager();

        // ============================================================================
        // INITIALIZATION
        // ============================================================================

        /**
         * @brief Initialize the shader manager
         * @param shaderDirectory Base directory for shader files
         * @param cacheDirectory Directory for shader cache
         * @param enableHotReload Whether to enable hot-reload for development
         * @return Success/failure result
         */
        Result<void> Initialize(const std::string& shaderDirectory = "Assets/Shaders/",
                               const std::string& cacheDirectory = "Cache/Shaders/",
                               bool enableHotReload = false);

        /**
         * @brief Shutdown the shader manager and release resources
         */
        void Shutdown();

        // ============================================================================
        // SHADER LOADING
        // ============================================================================

        /**
         * @brief Load a single shader from file
         * @param name Shader identifier
         * @param filePath Path to shader file
         * @param options Compilation options
         * @return Shader handle or error
         */
        Result<ShaderHandle> LoadShader(const std::string& name, 
                                       const std::string& filePath,
                                       const ShaderCompileOptions& options = {});

        /**
         * @brief Load a shader program from multiple stage files
         * @param name Program identifier  
         * @param shaderFiles Map of stage to file path
         * @param options Compilation options
         * @return Shader handle or error
         */
        Result<ShaderHandle> LoadShaderProgram(const std::string& name,
                                              const std::unordered_map<ShaderStage, std::string>& shaderFiles,
                                              const ShaderCompileOptions& options = {});

        /**
         * @brief Load shader with variants
         * @param name Base shader identifier
         * @param filePath Path to shader file
         * @param variants Macro combinations for variants
         * @param options Compilation options
         * @return Map of variant hash to shader handle
         */
        Result<std::unordered_map<uint64_t, ShaderHandle>> LoadShaderVariants(
            const std::string& name,
            const std::string& filePath, 
            const std::vector<ShaderMacros>& variants,
            const ShaderCompileOptions& options = {});

        /**
         * @brief Create shader from source code
         * @param name Shader identifier
         * @param source Source code
         * @param stage Shader stage
         * @param options Compilation options
         * @return Shader handle or error
         */
        Result<ShaderHandle> CreateShader(const std::string& name,
                                         const std::string& source,
                                         ShaderStage stage,
                                         const ShaderCompileOptions& options = {});

        // ============================================================================
        // SHADER ACCESS
        // ============================================================================

        /**
         * @brief Get shader by handle
         * @param handle Shader handle
         * @return Pointer to compiled shader, nullptr if invalid
         */
        const CompiledShader* GetShader(ShaderHandle handle) const;

        /**
         * @brief Get shader program by handle
         * @param handle Shader handle
         * @return Pointer to shader program, nullptr if invalid
         */
        const ShaderProgram* GetShaderProgram(ShaderHandle handle) const;

        /**
         * @brief Find shader by name
         * @param name Shader identifier
         * @return Shader handle, invalid if not found
         */
        ShaderHandle FindShader(const std::string& name) const;

        /**
         * @brief Get shader reflection data
         * @param handle Shader handle
         * @return Pointer to reflection data, nullptr if invalid
         */
        const ShaderReflectionData* GetReflectionData(ShaderHandle handle) const;

        // ============================================================================
        // SHADER BINDING
        // ============================================================================

        /**
         * @brief Bind shader program for rendering
         * @param handle Shader program handle
         * @return Success/failure result
         */
        Result<void> BindShader(ShaderHandle handle);

        /**
         * @brief Unbind current shader program
         */
        void UnbindShader();

        /**
         * @brief Get currently bound shader handle
         * @return Current shader handle, invalid if none bound
         */
        ShaderHandle GetCurrentShader() const;

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
        Result<void> SetUniform(const std::string& name, const void* data, uint32_t size);

        /**
         * @brief Set uniform buffer
         * @param binding Binding point
         * @param bufferId Buffer handle
         * @param offset Offset into buffer
         * @param size Buffer size
         * @return Success/failure result
         */
        Result<void> SetUniformBuffer(uint32_t binding, uint32_t bufferId, uint32_t offset, uint32_t size);

        /**
         * @brief Convenient uniform setters for common types
         */
        Result<void> SetUniform(const std::string& name, int value);
        Result<void> SetUniform(const std::string& name, float value);
        Result<void> SetUniform(const std::string& name, const glm::vec2& value);
        Result<void> SetUniform(const std::string& name, const glm::vec3& value);
        Result<void> SetUniform(const std::string& name, const glm::vec4& value);
        Result<void> SetUniform(const std::string& name, const glm::mat3& value);
        Result<void> SetUniform(const std::string& name, const glm::mat4& value);

        // ============================================================================
        // RESOURCE MANAGEMENT
        // ============================================================================

        /**
         * @brief Reload shader from disk
         * @param handle Shader handle
         * @return Success/failure result
         */
        Result<void> ReloadShader(ShaderHandle handle);

        /**
         * @brief Reload all shaders from disk
         * @return Number of shaders reloaded
         */
        uint32_t ReloadAllShaders();

        /**
         * @brief Remove shader from manager
         * @param handle Shader handle
         */
        void RemoveShader(ShaderHandle handle);

        /**
         * @brief Clear all shaders
         */
        void ClearAllShaders();

        // ============================================================================
        // STATISTICS AND DEBUG
        // ============================================================================

        /**
         * @brief Get manager statistics
         */
        struct Stats
        {
            uint32_t LoadedShaders = 0;
            uint32_t LoadedPrograms = 0;
            uint32_t CacheHits = 0;
            uint32_t CacheMisses = 0;
            uint64_t MemoryUsage = 0; // In bytes
            uint32_t HotReloads = 0;
        };
        Stats GetStats() const;

        /**
         * @brief Get detailed information about all loaded shaders
         * @return Debug info string
         */
        std::string GetDebugInfo() const;

        /**
         * @brief Validate all loaded shaders
         * @return Number of validation errors
         */
        uint32_t ValidateAllShaders();

        // ============================================================================
        // CALLBACKS
        // ============================================================================

        using ShaderReloadCallback = std::function<void(ShaderHandle handle, const std::string& name)>;
        using ShaderErrorCallback = std::function<void(const std::string& name, const std::vector<ShaderCompileError>& errors)>;

        /**
         * @brief Set callback for when shaders are reloaded
         */
        void SetShaderReloadCallback(ShaderReloadCallback callback);

        /**
         * @brief Set callback for shader compilation errors
         */
        void SetShaderErrorCallback(ShaderErrorCallback callback);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };

    /**
     * @brief Global shader manager instance
     * @return Reference to the global shader manager
     */
    ShaderManager& GetShaderManager();

} // namespace Vortex

// Convenience macros for shader management
#define VX_LOAD_SHADER(name, path) Vortex::Shader::GetShaderManager().LoadShader(name, path)
#define VX_BIND_SHADER(handle) Vortex::Shader::GetShaderManager().BindShader(handle)
#define VX_SET_UNIFORM(name, value) Vortex::Shader::GetShaderManager().SetUniform(name, value)
