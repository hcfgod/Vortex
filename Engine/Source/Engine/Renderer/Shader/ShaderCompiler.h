#pragma once

#include "ShaderTypes.h"
#include "ShaderReflection.h"
#include "Core/Debug/ErrorCodes.h"

#include <memory>
#include <functional>
#include <unordered_map>
#include <thread>
#include <mutex>

namespace Vortex::Shader
{
    /**
     * @brief AAA-grade shader compiler with advanced features
     * 
     * Features:
     * - Multi-threaded compilation
     * - Intelligent caching with dependency tracking
     * - Shader variants and permutations
     * - Hot-reload support
     * - SPIR-V reflection and validation
     * - Include file processing
     * - Comprehensive error reporting
     * - Asset pipeline integration
     */
    class ShaderCompiler
    {
    public:
        using ShaderLoadedCallback = std::function<void(const std::string& shaderPath, const CompiledShader& shader)>;
        using ShaderErrorCallback = std::function<void(const std::string& shaderPath, const std::vector<ShaderCompileError>& errors)>;

        ShaderCompiler();
        ~ShaderCompiler();

        // ============================================================================
        // COMPILATION METHODS
        // ============================================================================

        /**
         * @brief Compile shader from source code
         * @param source Shader source code
         * @param stage Shader stage
         * @param options Compilation options
         * @param filename Optional filename for error reporting
         * @return Compiled shader result
         */
        Result<CompiledShader> CompileFromSource(const std::string& source, 
                                               ShaderStage stage,
                                               const ShaderCompileOptions& options,
                                               const std::string& filename = "");

        /**
         * @brief Compile shader from file
         * @param filePath Path to shader file
         * @param options Compilation options
         * @return Compiled shader result
         */
        Result<CompiledShader> CompileFromFile(const std::string& filePath, 
                                             const ShaderCompileOptions& options);

        /**
         * @brief Compile shader with variants (permutations)
         * @param source Shader source code
         * @param stage Shader stage
         * @param variants Vector of macro combinations
         * @param options Base compilation options
         * @return Map of variant hash to compiled shader
         */
        Result<std::unordered_map<uint64_t, CompiledShader>> CompileVariants(
            const std::string& source,
            ShaderStage stage, 
            const std::vector<ShaderMacros>& variants,
            const ShaderCompileOptions& options);

        /**
         * @brief Compile complete shader program from multiple files
         * @param shaderFiles Map of shader stage to file path
         * @param options Compilation options
         * @return Compiled shader program
         */
        Result<ShaderProgram> CompileProgram(const std::unordered_map<ShaderStage, std::string>& shaderFiles,
                                           const ShaderCompileOptions& options);

        // ============================================================================
        // CACHE MANAGEMENT
        // ============================================================================

        /**
         * @brief Enable/disable shader caching
         * @param enabled Whether caching is enabled
         * @param cacheDirectory Directory to store cache files
         */
        void SetCachingEnabled(bool enabled, const std::string& cacheDirectory = "cache/shaders/");

        /**
         * @brief Clear the shader cache
         * @param deleteFiles Whether to delete cache files from disk
         */
        void ClearCache(bool deleteFiles = false);

        /**
         * @brief Get cache statistics
         */
        struct Stats
        {
            uint32_t ShadersCompiled = 0;
            uint32_t CacheHits = 0;
            uint32_t CacheMisses = 0;
            uint32_t CompilationErrors = 0;
            uint64_t CompilationTime = 0; // In microseconds
        };
        Stats GetStats() const;

        /**
         * @brief Reset compilation statistics
         */
        void ResetStats();

        // ============================================================================
        // HOT-RELOAD SUPPORT
        // ============================================================================

        /**
         * @brief Enable hot-reload for development
         * @param enabled Whether hot-reload is enabled
         * @param watchDirectories Directories to watch for changes
         */
        void SetHotReloadEnabled(bool enabled, const std::vector<std::string>& watchDirectories = {});

        /**
         * @brief Set callback for when shaders are reloaded
         */
        void SetShaderReloadCallback(ShaderLoadedCallback callback);

        /**
         * @brief Set callback for shader compilation errors
         */
        void SetShaderErrorCallback(ShaderErrorCallback callback);

        /**
         * @brief Manually trigger recompilation of a shader
         * @param filePath Path to shader file to recompile
         */
        void RecompileShader(const std::string& filePath);

        // ============================================================================
        // INCLUDE PROCESSING
        // ============================================================================

        /**
         * @brief Add include directory for shader processing
         * @param directory Directory to search for include files
         */
        void AddIncludeDirectory(const std::string& directory);

        /**
         * @brief Clear all include directories
         */
        void ClearIncludeDirectories();

        // ============================================================================
        // UTILITIES
        // ============================================================================

        /**
         * @brief Precompile shaders for faster runtime loading
         * @param inputDirectory Directory containing shader source files
         * @param outputDirectory Directory to write compiled shaders
         * @param options Compilation options
         * @return Number of shaders compiled
         */
        Result<uint32_t> PrecompileShaders(const std::string& inputDirectory,
                                         const std::string& outputDirectory,
                                         const ShaderCompileOptions& options);

        /**
         * @brief Validate SPIR-V bytecode
         * @param spirv SPIR-V bytecode to validate
         * @return True if valid, false otherwise
         */
        static bool ValidateSpirV(const std::vector<uint32_t>& spirv);

        /**
         * @brief Generate shader hash for caching
         * @param source Shader source code
         * @param options Compilation options
         * @return Hash value
         */
        static uint64_t GenerateShaderHash(const std::string& source, 
                                         const ShaderCompileOptions& options);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };

    /**
     * @brief Shader variant manager for handling permutations
     */
    class ShaderVariantManager
    {
    public:
        /**
         * @brief Generate all possible macro combinations
         * @param macroGroups Groups of mutually exclusive macros
         * @return Vector of all possible combinations
         */
        static std::vector<ShaderMacros> GenerateVariants(
            const std::vector<std::vector<ShaderMacro>>& macroGroups);

        /**
         * @brief Generate hash for a specific variant
         * @param macros Macro combination
         * @return Variant hash
         */
        static uint64_t GenerateVariantHash(const ShaderMacros& macros);

        /**
         * @brief Parse variant string (e.g., "FEATURE_A=1;FEATURE_B=0")
         * @param variantString String representation of variant
         * @return Parsed macros
         */
        static ShaderMacros ParseVariantString(const std::string& variantString);

        /**
         * @brief Convert macros to string representation
         * @param macros Macro list
         * @return String representation
         */
        static std::string MacrosToString(const ShaderMacros& macros);
    };

} // namespace Vortex::Shader
