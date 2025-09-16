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

namespace Vortex
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
        // Map source-file+stage -> last stored hash to prune stale cache entries on edits
        mutable std::unordered_map<std::string, uint64_t> m_SourceStageToHash;

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
            return m_CacheDirectory + "/" + std::to_string(hash) + "_" + GetShaderStageString(stage) + ".spv";
        }
        
        std::string GetCacheInfoPath(uint64_t hash, ShaderStage stage) const
        {
            return m_CacheDirectory + "/" + std::to_string(hash) + "_" + GetShaderStageString(stage) + ".info";
        }
        
        bool IsSourceFileNewer(const std::string& sourceFile, const std::string& cacheFile) const
        {
            if (sourceFile.empty() || !std::filesystem::exists(sourceFile))
                return false;
                
            if (!std::filesystem::exists(cacheFile))
                return true;
                
            try
            {
                auto sourceTime = std::filesystem::last_write_time(sourceFile);
                auto cacheTime = std::filesystem::last_write_time(cacheFile);
                return sourceTime > cacheTime;
            }
            catch (const std::exception& e)
            {
                VX_CORE_WARN("Failed to compare file timestamps: {0}", e.what());
                return true; // Assume source is newer on error
            }
        }

        bool LoadFromCache(uint64_t hash, ShaderStage stage, CompiledShader& outShader, const std::string& sourceFile = "") const
        {
            if (!m_CachingEnabled)
                return false;

            // Check memory cache first
            {
                std::shared_lock<std::shared_mutex> lock(m_CacheMutex);
                auto it = m_ShaderCache.find(hash);
                if (it != m_ShaderCache.end())
                {
                    // Check if source file is newer than cached version
                    if (!sourceFile.empty() && IsSourceFileNewer(sourceFile, GetCacheFilePath(hash, stage)))
                    {
                        // Invalidate memory cache
                        const_cast<std::unordered_map<uint64_t, CompiledShader>&>(m_ShaderCache).erase(hash);
                    }
                    else
                    {
                        outShader = it->second;
                        return true;
                    }
                }
            }

            // Check disk cache
            std::string cacheFile = GetCacheFilePath(hash, stage);
            std::string infoFile = GetCacheInfoPath(hash, stage);
            
            if (!std::filesystem::exists(cacheFile) || !std::filesystem::exists(infoFile))
                return false;
                
            // Check if source file is newer than cache
            if (!sourceFile.empty() && IsSourceFileNewer(sourceFile, cacheFile))
            {
                VX_CORE_TRACE("Source file is newer than cache, recompiling shader");
                return false;
            }

            try
            {
                // Load SPIR-V from .spv file
                std::ifstream spirvFile(cacheFile, std::ios::binary);
                if (!spirvFile.is_open())
                    return false;

                // Get file size
                spirvFile.seekg(0, std::ios::end);
                size_t fileSize = spirvFile.tellg();
                spirvFile.seekg(0, std::ios::beg);
                
                // Read SPIR-V data directly
                outShader.SpirV.resize(fileSize / sizeof(uint32_t));
                spirvFile.read(reinterpret_cast<char*>(outShader.SpirV.data()), fileSize);
                spirvFile.close();

                // Load reflection data from .info file
                std::ifstream infoFileStream(infoFile, std::ios::binary);
                if (infoFileStream.is_open())
                {
                    // Read reflection data size and content
                    uint32_t reflectionSize;
                    infoFileStream.read(reinterpret_cast<char*>(&reflectionSize), sizeof(reflectionSize));
                    
                    if (reflectionSize > 0)
                    {
                        std::string reflectionJson(reflectionSize, '\0');
                        infoFileStream.read(&reflectionJson[0], reflectionSize);
                        
                        // Parse reflection data from JSON (simplified - in real implementation use proper JSON)
                        // For now, just mark as valid but empty reflection
                        outShader.Reflection = {};
                    }
                    infoFileStream.close();
                }

                outShader.Stage = stage;
                outShader.Status = ShaderCompileStatus::Success;
                outShader.SourceHash = hash;
                outShader.SourceFile = sourceFile;

                // Store in memory cache and update source mapping
                {
                    std::unique_lock<std::shared_mutex> lock(m_CacheMutex);
                    m_ShaderCache[hash] = outShader;
                    if (!outShader.SourceFile.empty())
                    {
                        std::string sourceKey = outShader.SourceFile + "|" + std::to_string(static_cast<int>(outShader.Stage));
                        m_SourceStageToHash[sourceKey] = hash;
                    }
                }

                VX_CORE_TRACE("Loaded SPIR-V shader from cache: {0}", cacheFile);
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

            // Save to memory cache and update source mapping
            uint64_t prevHashForSource = 0;
            std::string sourceKey;
            {
                std::unique_lock<std::shared_mutex> lock(m_CacheMutex);
                m_ShaderCache[hash] = shader;
                if (!shader.SourceFile.empty())
                {
                    sourceKey = shader.SourceFile + "|" + std::to_string(static_cast<int>(shader.Stage));
                    auto it = m_SourceStageToHash.find(sourceKey);
                    if (it != m_SourceStageToHash.end()) prevHashForSource = it->second;
                    m_SourceStageToHash[sourceKey] = hash;
                }
            }

            // Save to disk cache
            try
            {
                std::filesystem::create_directories(m_CacheDirectory);
                std::string cacheFile = GetCacheFilePath(hash, shader.Stage);
                std::string infoFile = GetCacheInfoPath(hash, shader.Stage);
                
                // Write SPIR-V data to .spv file
                std::ofstream spirvFileStream(cacheFile, std::ios::binary);
                if (!spirvFileStream.is_open())
                {
                    VX_CORE_ERROR("Failed to open SPIR-V cache file for writing: {0}", cacheFile);
                    return;
                }

                // Write raw SPIR-V data
                spirvFileStream.write(reinterpret_cast<const char*>(shader.SpirV.data()), 
                                    shader.SpirV.size() * sizeof(uint32_t));
                spirvFileStream.close();

                // Write reflection data to .info file
                std::ofstream infoFileStream(infoFile, std::ios::binary);
                if (infoFileStream.is_open())
                {
                    // For now, write a placeholder for reflection data
                    // In a real implementation, serialize the reflection data to JSON or binary format
                    std::string reflectionPlaceholder = "{}";
                    uint32_t reflectionSize = static_cast<uint32_t>(reflectionPlaceholder.size());
                    
                    infoFileStream.write(reinterpret_cast<const char*>(&reflectionSize), sizeof(reflectionSize));
                    infoFileStream.write(reflectionPlaceholder.c_str(), reflectionSize);
                    infoFileStream.close();
                }
                
                VX_CORE_TRACE("Saved SPIR-V shader to cache: {0}", cacheFile);
            }
            catch (const std::exception& e)
            {
                VX_CORE_ERROR("Failed to save shader to cache: {0}", e.what());
            }

            // Prune stale cache entry for the same source+stage (if any and different hash)
            if (prevHashForSource != 0 && prevHashForSource != hash)
            {
                try
                {
                    std::string oldCache = GetCacheFilePath(prevHashForSource, shader.Stage);
                    std::string oldInfo = GetCacheInfoPath(prevHashForSource, shader.Stage);
                    std::error_code ec;
                    if (std::filesystem::exists(oldCache, ec)) std::filesystem::remove(oldCache, ec);
                    if (std::filesystem::exists(oldInfo, ec)) std::filesystem::remove(oldInfo, ec);

                    // Also drop old entry from memory cache
                    {
                        std::unique_lock<std::shared_mutex> lock(m_CacheMutex);
                        m_ShaderCache.erase(prevHashForSource);
                    }
                    VX_CORE_TRACE("Pruned stale shader cache for source '{0}' (old hash {1})", shader.SourceFile, prevHashForSource);
                }
                catch (const std::exception& e)
                {
                    VX_CORE_WARN("Failed to prune old shader cache entries: {0}", e.what());
                }
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

        // Try loading from cache first (pass filename for timestamp checking)
        CompiledShader cachedShader;
        if (m_Impl->LoadFromCache(hash, stage, cachedShader, filename))
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
            // Ensure cache mapping is updated as soon as we have a compiled shader
            if (m_Impl->m_CachingEnabled)
            {
                m_Impl->SaveToCache(shader.SourceHash, shader);
            }
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
                    if (entry.path().extension() == ".spv" || entry.path().extension() == ".info")
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
    // ASYNC COROUTINE COMPILATION METHODS
    // ============================================================================

    Task<Result<CompiledShader>> ShaderCompiler::CompileFromSourceAsync(
        std::string source, 
        ShaderStage stage,
        ShaderCompileOptions options,
        CoroutinePriority priority,
        std::string filename)
    {
        // Yield control with specified priority to allow scheduler to manage workload
        co_await YieldExecution(priority);

        // Log compilation start for debugging
        VX_CORE_TRACE("Starting async shader compilation: {0} (priority: {1})", 
                     filename.empty() ? "<source>" : filename, static_cast<int>(priority));

        // Perform the compilation (this is the expensive operation)
        auto result = CompileFromSource(source, stage, options, filename);

        // Yield again after compilation to allow other coroutines to run
        co_await YieldExecution();

        VX_CORE_TRACE("Completed async shader compilation: {0}", 
                     filename.empty() ? "<source>" : filename);

        co_return result;
    }

    Task<Result<CompiledShader>> ShaderCompiler::CompileFromFileAsync(
        std::string filePath, 
        ShaderCompileOptions options,
        CoroutinePriority priority)
    {
        // Yield control with specified priority
        co_await YieldExecution(priority);

        VX_CORE_TRACE("Starting async file shader compilation: {0} (priority: {1})", 
                     filePath, static_cast<int>(priority));

        // Use the synchronous method for actual compilation
        auto result = CompileFromFile(filePath, options);

        // Yield after compilation
        co_await YieldExecution();

        VX_CORE_TRACE("Completed async file shader compilation: {0}", filePath);

        co_return result;
    }

    Task<Result<std::unordered_map<uint64_t, CompiledShader>>> ShaderCompiler::CompileVariantsAsync(
        std::string source,
        ShaderStage stage, 
        std::vector<ShaderMacros> variants,
        ShaderCompileOptions options,
        CoroutinePriority priority)
    {
        co_await YieldExecution(priority);

        VX_CORE_TRACE("Starting async variant compilation: {0} variants (priority: {1})", 
                     variants.size(), static_cast<int>(priority));

        std::unordered_map<uint64_t, CompiledShader> results;
        
        // Compile each variant, yielding between compilations to prevent blocking
        for (size_t i = 0; i < variants.size(); ++i)
        {
            const auto& variantMacros = variants[i];
            
            // Create options for this variant
            ShaderCompileOptions variantOptions = options;
            for (const auto& macro : variantMacros)
            {
                variantOptions.Macros[macro.Name] = macro.Value;
            }
            
            // Generate hash for this variant
            uint64_t variantHash = ShaderVariantManager::GenerateVariantHash(variantMacros);
            
            // Compile this variant
            auto result = CompileFromSource(source, stage, variantOptions, "variant_" + std::to_string(i));
            
            if (result.IsSuccess())
            {
                results[variantHash] = std::move(const_cast<CompiledShader&>(result.GetValue()));
                VX_CORE_TRACE("Compiled shader variant {0}/{1} successfully (hash: {2})", 
                             i + 1, variants.size(), variantHash);
            }
            else
            {
                VX_CORE_ERROR("Failed to compile shader variant {0}/{1}: {2}", 
                             i + 1, variants.size(), result.GetErrorMessage());
                // Return early on first error
                co_return Result<std::unordered_map<uint64_t, CompiledShader>>(
                    result.GetErrorCode(), 
                    "Variant compilation failed: " + result.GetErrorMessage());
            }
            
            // Yield between variants to allow other work
            if (i < variants.size() - 1)
            {
                co_await YieldExecution();
            }
        }

        VX_CORE_INFO("Completed async variant compilation: {0} variants compiled", results.size());
        
        co_return Result<std::unordered_map<uint64_t, CompiledShader>>(std::move(results));
    }

    Task<std::vector<Result<CompiledShader>>> ShaderCompiler::CompileBatchAsync(
        std::vector<std::tuple<std::string, ShaderStage, ShaderCompileOptions>> compilationTasks,
        size_t maxConcurrency)
    {
        co_await YieldExecution(CoroutinePriority::Normal);

        VX_CORE_INFO("Starting batch shader compilation: {0} shaders (max concurrency: {1})", 
                    compilationTasks.size(), maxConcurrency);

        std::vector<Result<CompiledShader>> results;
        results.reserve(compilationTasks.size());

        // Process tasks in batches to respect concurrency limit
        for (size_t startIdx = 0; startIdx < compilationTasks.size(); startIdx += maxConcurrency)
        {
            size_t endIdx = std::min(startIdx + maxConcurrency, compilationTasks.size());
            std::vector<Task<Result<CompiledShader>>> currentBatch;
            
            // Start coroutines for current batch
            for (size_t i = startIdx; i < endIdx; ++i)
            {
                const auto& [source, stage, options] = compilationTasks[i];
                currentBatch.push_back(
                    CompileFromSourceAsync(source, stage, options, CoroutinePriority::Normal, 
                                         "batch_" + std::to_string(i))
                );
            }
            
            // Wait for all tasks in current batch to complete
            // For now, process each task sequentially since WhenAll doesn't support iterators yet
            for (auto& task : currentBatch)
            {
                auto result = co_await task;
                results.push_back(std::move(result));
            }
            
            // Yield between batches to allow other work
            co_await YieldExecution();
        }

        VX_CORE_INFO("Completed batch shader compilation: {0} shaders processed", results.size());
        
        co_return results;
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

    // ============================================================================
    // ShaderVariantManager Implementation
    // ============================================================================

    std::vector<ShaderMacros> ShaderVariantManager::GenerateVariants(
        const std::vector<std::vector<ShaderMacro>>& macroGroups)
    {
        // TODO: Implement variant generation logic
        std::vector<ShaderMacros> variants;
        return variants;
    }

    uint64_t ShaderVariantManager::GenerateVariantHash(const ShaderMacros& macros)
    {
        std::stringstream ss;
        for (const auto& macro : macros)
        {
            ss << macro.Name << "=" << macro.Value << ";";
        }
        return HashString(ss.str());
    }

    ShaderMacros ShaderVariantManager::ParseVariantString(const std::string& variantString)
    {
        ShaderMacros macros;
        // TODO: Implement string parsing logic
        // Format: "MACRO1=value1;MACRO2=value2;..."
        return macros;
    }

    std::string ShaderVariantManager::MacrosToString(const ShaderMacros& macros)
    {
        std::stringstream ss;
        for (size_t i = 0; i < macros.size(); ++i)
        {
            if (i > 0) ss << ";";
            ss << macros[i].Name << "=" << macros[i].Value;
        }
        return ss.str();
    }

} // namespace Vortex
