#pragma once

#include "Core/Debug/ErrorCodes.h"
#include "Engine/Assets/AssetHandle.h"
#include "Engine/Assets/ShaderAsset.h"
#include "Engine/Renderer/Shader/ShaderTypes.h"
#include "Engine/Renderer/Shader/ShaderCompiler.h"
#include "Engine/Renderer/Shader/Shader.h"
#include "Engine/Renderer/Shader/ShaderReflection.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>

namespace Vortex
{
    // Use AssetSystem's typed handle directly for shader assets

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
        Result<AssetHandle<ShaderAsset>> LoadShader(const std::string& name, const std::string& filePath, const ShaderCompileOptions& options = {});

        /**
         * @brief Load a shader program from multiple stage files
         * @param name Program identifier  
         * @param shaderFiles Map of stage to file path
         * @param options Compilation options
         * @return Shader handle or error
         */
        Result<AssetHandle<ShaderAsset>> LoadShaderProgram(const std::string& name, const std::unordered_map<ShaderStage, std::string>& shaderFiles, const ShaderCompileOptions& options = {});

        /**
         * @brief Load shader with variants
         * @param name Base shader identifier
         * @param filePath Path to shader file
         * @param variants Macro combinations for variants
         * @param options Compilation options
         * @return Map of variant hash to shader handle
         */
        Result<std::unordered_map<uint64_t, AssetHandle<ShaderAsset>>> LoadShaderVariants(const std::string& name, const std::string& filePath, const std::vector<ShaderMacros>& variants, const ShaderCompileOptions& options = {});

        // ============================================================================
        // SHADER ACCESS
        // ============================================================================

        /**
         * @brief Get shader program by handle
         * @param handle Shader handle
         * @return Pointer to shader program, nullptr if invalid
         */
        const ShaderProgram* GetShaderProgram(const AssetHandle<ShaderAsset>& handle) const;

        /**
         * @brief Find shader by name
         * @param name Shader identifier
         * @return Shader handle, invalid if not found
         */
        AssetHandle<ShaderAsset> FindShader(const std::string& name) const;

        /**
         * @brief Get shader reflection data
         * @param handle Shader handle
         * @return Pointer to reflection data, nullptr if invalid
         */
        const ShaderReflectionData* GetReflectionData(const AssetHandle<ShaderAsset>& handle) const;

        // ============================================================================
        // SHADER BINDING
        // ============================================================================

        /**
         * @brief Bind shader program for rendering
         * @param handle Shader program handle
         * @return Success/failure result
         */
        Result<void> BindShader(const AssetHandle<ShaderAsset>& handle);

        /**
         * @brief Unbind current shader program
         */
        void UnbindShader();

        /**
         * @brief Get currently bound shader handle
         * @return Current shader handle, invalid if none bound
         */
        AssetHandle<ShaderAsset> GetCurrentShader() const;

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
        Result<void> ReloadShader(const AssetHandle<ShaderAsset>& handle);

        /**
         * @brief Reload all shaders from disk
         * @return Number of shaders reloaded
         */
        uint32_t ReloadAllShaders();

        /**
         * @brief Remove shader from manager
         * @param handle Shader handle
         */
        void RemoveShader(const AssetHandle<ShaderAsset>& handle);

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

        using ShaderReloadCallback = std::function<void(const AssetHandle<ShaderAsset>& handle, const std::string& name)>;
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