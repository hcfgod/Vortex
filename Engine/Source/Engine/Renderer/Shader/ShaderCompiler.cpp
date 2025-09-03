#include "vxpch.h"
#include "ShaderCompiler.h"
#include "ShaderReflection.h"
#include "Core/Debug/Log.h"

#include <shaderc/shaderc.hpp>
#include <fstream>
#include <filesystem>
#include <future>
#include <sstream>
#include <shared_mutex>

namespace Vortex::Shader
{
    // ============================================================================
    // Utility Functions
    // ============================================================================

    static shaderc_shader_kind GetShadercKind(ShaderStage stage)
    {
        switch (stage)
        {
        case ShaderStage::Vertex:                   return shaderc_vertex_shader;
        case ShaderStage::Fragment:                 return shaderc_fragment_shader;
        case ShaderStage::Geometry:                 return shaderc_geometry_shader;
        case ShaderStage::TessellationControl:      return shaderc_tess_control_shader;
        case ShaderStage::TessellationEvaluation:   return shaderc_tess_evaluation_shader;
        case ShaderStage::Compute:                  return shaderc_compute_shader;
        case ShaderStage::RayGeneration:            return shaderc_raygen_shader;
        case ShaderStage::AnyHit:                   return shaderc_anyhit_shader;
        case ShaderStage::ClosestHit:               return shaderc_closesthit_shader;
        case ShaderStage::Miss:                     return shaderc_miss_shader;
        case ShaderStage::Intersection:             return shaderc_intersection_shader;
        case ShaderStage::Callable:                 return shaderc_callable_shader;
        default:
            VX_CORE_ERROR("Unknown shader stage: {0}", static_cast<int>(stage));
            return shaderc_vertex_shader;
        }
    }

    static std::string GetShaderStageString(ShaderStage stage)
    {
        switch (stage)
        {
        case ShaderStage::Vertex:                   return "vertex";
        case ShaderStage::Fragment:                 return "fragment";
        case ShaderStage::Geometry:                 return "geometry";
        case ShaderStage::TessellationControl:     return "tess_control";
        case ShaderStage::TessellationEvaluation:  return "tess_eval";
        case ShaderStage::Compute:                  return "compute";
        case ShaderStage::RayGeneration:            return "raygen";
        case ShaderStage::AnyHit:                   return "anyhit";
        case ShaderStage::ClosestHit:               return "closesthit";
        case ShaderStage::Miss:                     return "miss";
        case ShaderStage::Intersection:             return "intersection";
        case ShaderStage::Callable:                 return "callable";
        default:                                    return "unknown";
        }
    }

    static std::string ReadFileToString(const std::string& filepath)
    {
        std::ifstream file(filepath, std::ios::in | std::ios::binary);
        if (!file.is_open())
        {
            return "";
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string content(size, '\0');
        file.read(&content[0], size);
        return content;
    }

    static uint64_t HashString(const std::string& str)
    {
        std::hash<std::string> hasher;
        return hasher(str);
    }

    // ============================================================================
    // ShaderCompiler::Impl
    // ============================================================================

    class ShaderCompiler::Impl
    {
    public:
        // Compiler state
        shaderc::Compiler m_Compiler;
        ShaderReflection m_Reflection;

        // Settings
        bool m_CachingEnabled = false;
        std::string m_CacheDirectory;
        bool m_HotReloadEnabled = false;
        std::vector<std::string> m_WatchDirectories;
        
        // Cache
        mutable std::unordered_map<uint64_t, CompiledShader> m_ShaderCache;
        mutable std::shared_mutex m_CacheMutex;

        // Statistics
        mutable std::mutex m_StatsMutex;
        Stats m_Stats;

        Impl() = default;

        shaderc::CompileOptions CreateCompileOptions(const ShaderCompileOptions& options) const
        {
            shaderc::CompileOptions shadercOptions;

            // Optimization level
            switch (options.OptimizationLevel)
            {
            case ShaderOptimizationLevel::None:
                shadercOptions.SetOptimizationLevel(shaderc_optimization_level_zero);
                break;
            case ShaderOptimizationLevel::Size:
                shadercOptions.SetOptimizationLevel(shaderc_optimization_level_size);
                break;
            case ShaderOptimizationLevel::Performance:
                shadercOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
                break;
            case ShaderOptimizationLevel::Debug:
                shadercOptions.SetOptimizationLevel(shaderc_optimization_level_zero);
                shadercOptions.SetGenerateDebugInfo();
                break;
            }

            // Target environment based on TargetProfile string (simple heuristic)
            if (!options.TargetProfile.empty())
            {
                if (options.TargetProfile.rfind("vulkan", 0) == 0)
                {
                    // Default to Vulkan 1.1
                    shadercOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
                }
                else if (options.TargetProfile.rfind("opengl", 0) == 0)
                {
                    shadercOptions.SetTargetEnvironment(shaderc_target_env_opengl, 0);
                }
            }

            // Debug info
            if (options.GenerateDebugInfo)
            {
                shadercOptions.SetGenerateDebugInfo();
            }

            // Warnings as errors
            if (options.TreatWarningsAsErrors)
            {
                shadercOptions.SetWarningsAsErrors();
            }

            // Macros
            for (const auto& [name, value] : options.Macros)
            {
                shadercOptions.AddMacroDefinition(name, value);
            }

            // Include directories
            // Note: shaderc doesn't support include directories directly,
            // we'd need to implement a custom includer

            return shadercOptions;
        }

        uint64_t ComputeShaderHash(const std::string& source, ShaderStage stage, const ShaderCompileOptions& options) const
        {
            std::stringstream ss;
            ss << source;
            ss << static_cast<int>(stage);
            ss << static_cast<int>(options.OptimizationLevel);
            ss << options.TargetProfile;
            ss << options.GenerateDebugInfo;
            ss << options.TreatWarningsAsErrors;
            
            for (const auto& [name, value] : options.Macros)
            {
                ss << name << "=" << value << ";";
            }

            return HashString(ss.str());
        }

        std::string GetCacheFilePath(uint64_t hash, ShaderStage stage) const
        {
            return m_CacheDirectory + "/" + std::to_string(hash) + "_" + GetShaderStageString(stage) + ".cache";
        }

        bool LoadFromCache(uint64_t hash, ShaderStage stage, CompiledShader& outShader) const
        {
            if (!m_CachingEnabled)
                return false;

            // Check memory cache first
            {
                std::shared_lock<std::shared_mutex> lock(m_CacheMutex);
                auto it = m_ShaderCache.find(hash);
                if (it != m_ShaderCache.end())
                {
                    outShader = it->second;
                    return true;
                }
            }

            // Check disk cache
            std::string cacheFile = GetCacheFilePath(hash, stage);
            if (!std::filesystem::exists(cacheFile))
                return false;

            try
            {
                std::ifstream file(cacheFile, std::ios::binary);
                if (!file.is_open())
                    return false;

                // Read SPIR-V size
                uint32_t spirvSize;
                file.read(reinterpret_cast<char*>(&spirvSize), sizeof(spirvSize));

                // Read SPIR-V data
                outShader.SpirV.resize(spirvSize);
                file.read(reinterpret_cast<char*>(outShader.SpirV.data()), spirvSize * sizeof(uint32_t));

                outShader.Stage = stage;
                outShader.Status = ShaderCompileStatus::Success;

                // TODO: Load reflection data

                // Store in memory cache
                {
                    std::unique_lock<std::shared_mutex> lock(m_CacheMutex);
                    m_ShaderCache[hash] = outShader;
                }

                return true;
            }
            catch (const std::exception& e)
            {
                VX_CORE_ERROR("Failed to load shader from cache: {0}", e.what());
                return false;
            }
        }

        void SaveToCache(uint64_t hash, const CompiledShader& shader) const
        {
            if (!m_CachingEnabled)
                return;

            // Save to memory cache
            {
                std::unique_lock<std::shared_mutex> lock(m_CacheMutex);
                m_ShaderCache[hash] = shader;
            }

            // Save to disk cache
            try
            {
                std::filesystem::create_directories(m_CacheDirectory);
                std::string cacheFile = GetCacheFilePath(hash, shader.Stage);
                
                std::ofstream file(cacheFile, std::ios::binary);
                if (!file.is_open())
                {
                    VX_CORE_ERROR("Failed to open cache file for writing: {0}", cacheFile);
                    return;
                }

                // Write SPIR-V size
                uint32_t spirvSize = static_cast<uint32_t>(shader.SpirV.size());
                file.write(reinterpret_cast<const char*>(&spirvSize), sizeof(spirvSize));

                // Write SPIR-V data
                file.write(reinterpret_cast<const char*>(shader.SpirV.data()), spirvSize * sizeof(uint32_t));

                // TODO: Save reflection data
            }
            catch (const std::exception& e)
            {
                VX_CORE_ERROR("Failed to save shader to cache: {0}", e.what());
            }
        }
    };

    // ============================================================================
    // ShaderCompiler Implementation
    // ============================================================================

    ShaderCompiler::ShaderCompiler()
        : m_Impl(std::make_unique<Impl>())
    {
        VX_CORE_INFO("ShaderCompiler created");
    }

    ShaderCompiler::~ShaderCompiler()
    {
        VX_CORE_INFO("ShaderCompiler destroyed");
    }

    Result<CompiledShader> ShaderCompiler::CompileFromSource(const std::string& source, 
                                                             ShaderStage stage, 
                                                             const ShaderCompileOptions& options,
                                                             const std::string& filename)
    {
        if (source.empty())
        {
return Result<CompiledShader>(ErrorCode::InvalidParameter, "Empty shader source code");
        }

        // Compute hash for caching
        uint64_t hash = m_Impl->ComputeShaderHash(source, stage, options);

        // Try loading from cache first
        CompiledShader cachedShader;
        if (m_Impl->LoadFromCache(hash, stage, cachedShader))
        {
            std::lock_guard<std::mutex> lock(m_Impl->m_StatsMutex);
            m_Impl->m_Stats.CacheHits++;
            VX_CORE_TRACE("Loaded shader from cache (hash: {0})", hash);
            return Result<CompiledShader>(std::move(cachedShader));
        }

        // Compile shader
        auto shadercOptions = m_Impl->CreateCompileOptions(options);
        shaderc_shader_kind shadercKind = GetShadercKind(stage);

        std::string actualFilename = filename.empty() ? "shader" : filename;

        auto compilationResult = m_Impl->m_Compiler.CompileGlslToSpv(
            source, 
            shadercKind, 
            actualFilename.c_str(), 
            shadercOptions
        );

        if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            std::string errorMsg = compilationResult.GetErrorMessage();
            VX_CORE_ERROR("Shader compilation failed: {0}", errorMsg);
            
            std::lock_guard<std::mutex> lock(m_Impl->m_StatsMutex);
            m_Impl->m_Stats.CompilationErrors++;
            
            return Result<CompiledShader>(ErrorCode::ShaderCompilationFailed, "Shader compilation failed: " + errorMsg);
        }

        // Create compiled shader
        CompiledShader compiledShader;
        compiledShader.Status = ShaderCompileStatus::Success;
        compiledShader.Stage = stage;
        compiledShader.SourceHash = hash;
        compiledShader.SpirV = std::vector<uint32_t>(compilationResult.cbegin(), compilationResult.cend());

        // Perform reflection
        auto reflectionResult = m_Impl->m_Reflection.Reflect(compiledShader.SpirV, stage);
        if (reflectionResult.IsSuccess())
        {
            compiledShader.Reflection = reflectionResult.GetValue();
        }
        else
        {
            VX_CORE_WARN("Shader reflection failed: {0}", reflectionResult.GetErrorMessage());
        }

        // Cache the compiled shader
        m_Impl->SaveToCache(hash, compiledShader);

        // Update statistics
        {
            std::lock_guard<std::mutex> lock(m_Impl->m_StatsMutex);
            m_Impl->m_Stats.ShadersCompiled++;
            m_Impl->m_Stats.CacheMisses++;
        }

        VX_CORE_INFO("Compiled {0} shader successfully (hash: {1})", GetShaderStageString(stage), hash);

        return Result<CompiledShader>(std::move(compiledShader));
    }

    Result<CompiledShader> ShaderCompiler::CompileFromFile(const std::string& filePath, 
                                                          const ShaderCompileOptions& options)
    {
        if (!std::filesystem::exists(filePath))
        {
            return Result<CompiledShader>(ErrorCode::FileNotFound, "Shader file not found: " + filePath);
        }

        std::string source = ReadFileToString(filePath);
        if (source.empty())
        {
            return Result<CompiledShader>(ErrorCode::FileCorrupted, "Failed to read shader file or file is empty: " + filePath);
        }

        // Determine stage from file extension
        ShaderStage stage;
        {
            std::string extension = std::filesystem::path(filePath).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".vert" || extension == ".vs")
                stage = ShaderStage::Vertex;
            else if (extension == ".frag" || extension == ".fs")
                stage = ShaderStage::Fragment;
            else if (extension == ".geom" || extension == ".gs")
                stage = ShaderStage::Geometry;
            else if (extension == ".tesc" || extension == ".tcs")
                stage = ShaderStage::TessellationControl;
            else if (extension == ".tese" || extension == ".tes")
                stage = ShaderStage::TessellationEvaluation;
            else if (extension == ".comp" || extension == ".cs")
                stage = ShaderStage::Compute;
            else
            {
return Result<CompiledShader>(ErrorCode::InvalidParameter, "Could not determine shader stage from file extension: " + extension);
            }
        }

        auto result = CompileFromSource(source, stage, options, filePath);
        if (result.IsSuccess())
        {
            // Store file path for hot-reload
            CompiledShader& shader = const_cast<CompiledShader&>(result.GetValue());
            shader.SourceFile = filePath;
        }

        return result;
    }

    void ShaderCompiler::SetCachingEnabled(bool enabled, const std::string& cacheDirectory)
    {
        m_Impl->m_CachingEnabled = enabled;
        m_Impl->m_CacheDirectory = cacheDirectory;

        if (enabled)
        {
            std::filesystem::create_directories(cacheDirectory);
            VX_CORE_INFO("Shader caching enabled: {0}", cacheDirectory);
        }
        else
        {
            VX_CORE_INFO("Shader caching disabled");
        }
    }

    void ShaderCompiler::ClearCache(bool deleteFiles)
    {
        // Clear memory cache
        {
            std::unique_lock<std::shared_mutex> lock(m_Impl->m_CacheMutex);
            m_Impl->m_ShaderCache.clear();
        }

        // Clear disk cache
        if (deleteFiles && m_Impl->m_CachingEnabled && std::filesystem::exists(m_Impl->m_CacheDirectory))
        {
            try
            {
                for (const auto& entry : std::filesystem::directory_iterator(m_Impl->m_CacheDirectory))
                {
                    if (entry.path().extension() == ".cache")
                    {
                        std::filesystem::remove(entry.path());
                    }
                }
                VX_CORE_INFO("Shader cache cleared");
            }
            catch (const std::exception& e)
            {
                VX_CORE_ERROR("Failed to clear shader cache: {0}", e.what());
            }
        }
    }

    void ShaderCompiler::SetHotReloadEnabled(bool enabled, const std::vector<std::string>& watchDirectories)
    {
        m_Impl->m_HotReloadEnabled = enabled;
        m_Impl->m_WatchDirectories = watchDirectories;

        if (enabled)
        {
            VX_CORE_INFO("Shader hot-reload enabled");
            for (const auto& dir : watchDirectories)
            {
                VX_CORE_INFO("  Watching directory: {0}", dir);
            }
        }
        else
        {
            VX_CORE_INFO("Shader hot-reload disabled");
        }

        // TODO: Implement file system watching
    }

    ShaderCompiler::Stats ShaderCompiler::GetStats() const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_StatsMutex);
        return m_Impl->m_Stats;
    }

    void ShaderCompiler::ResetStats()
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_StatsMutex);
        m_Impl->m_Stats = {};
        VX_CORE_INFO("Shader compiler statistics reset");
    }

    // ============================================================================
    // Shader Program Compilation (stub implementations)
    // ============================================================================

    Result<std::unordered_map<uint64_t, CompiledShader>> ShaderCompiler::CompileVariants(
        const std::string& source,
        ShaderStage stage, 
        const std::vector<ShaderMacros>& variants,
        const ShaderCompileOptions& baseOptions)
    {
        return Result<std::unordered_map<uint64_t, CompiledShader>>(ErrorCode::NotImplemented, "CompileVariants not yet implemented");
    }

    Result<ShaderProgram> ShaderCompiler::CompileProgram(const std::unordered_map<ShaderStage, std::string>& shaderFiles,
                                                        const ShaderCompileOptions& options)
    {
        return Result<ShaderProgram>(ErrorCode::NotImplemented, "CompileProgram not yet implemented");
    }

    // ============================================================================
    // Static utility methods (stub implementations)
    // ============================================================================

    bool ShaderCompiler::ValidateSpirV(const std::vector<uint32_t>& spirv)
    {
        // TODO: Implement SPIR-V validation using spirv-tools
        return !spirv.empty();
    }

    uint64_t ShaderCompiler::GenerateShaderHash(const std::string& source, const ShaderCompileOptions& options)
    {
        std::stringstream ss;
        ss << source;
        ss << static_cast<int>(options.OptimizationLevel);
        ss << options.TargetProfile;
        ss << options.GenerateDebugInfo;
        ss << options.TreatWarningsAsErrors;
        
        for (const auto& [name, value] : options.Macros)
        {
            ss << name << "=" << value << ";";
        }

        return HashString(ss.str());
    }

    // ============================================================================
    // Stub methods for missing functionality
    // ============================================================================

    void ShaderCompiler::SetShaderReloadCallback(ShaderLoadedCallback callback)
    {
        // TODO: Store callback for hot-reload
    }

    void ShaderCompiler::SetShaderErrorCallback(ShaderErrorCallback callback)
    {
        // TODO: Store callback for errors
    }

    void ShaderCompiler::RecompileShader(const std::string& filePath)
    {
        // TODO: Implement hot-reload recompilation
    }

    void ShaderCompiler::AddIncludeDirectory(const std::string& directory)
    {
        // TODO: Store include directories
    }

    void ShaderCompiler::ClearIncludeDirectories()
    {
        // TODO: Clear include directories
    }

    Result<uint32_t> ShaderCompiler::PrecompileShaders(const std::string& inputDirectory,
                                                       const std::string& outputDirectory,
                                                       const ShaderCompileOptions& options)
    {
        return Result<uint32_t>(ErrorCode::NotImplemented, "PrecompileShaders not yet implemented");
    }

} // namespace Vortex::Shader
