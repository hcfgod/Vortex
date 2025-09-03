#include "vxpch.h"
#include "ShaderManager.h"
#include "ShaderCompiler.h"
#include "ShaderReflection.h"
#include "Shader.h"
#include "Core/Debug/ErrorCodes.h"
#include "Core/Debug/Log.h"
#include "Core/Debug/Assert.h"
#include "Core/Debug/Exception.h"
#include "Core/Debug/CrashHandler.h"

#include <unordered_map>
#include <mutex>

namespace Vortex::Shader
{
    // ============================================================================
    // ShaderHandle Implementation
    // ============================================================================

    // ShaderHandle methods are already defined inline in the header

    // ============================================================================
    // ShaderManager::Impl
    // ============================================================================

    class ShaderManager::Impl
    {
    public:
        struct ShaderEntry
        {
            std::string Name;
            std::string FilePath;
            std::shared_ptr<Vortex::GPUShader> Shader;
            CompiledShader CompiledData;
            ShaderCompileOptions Options;
            uint64_t FileHash = 0;
        };

        // Core data
        std::unordered_map<uint64_t, ShaderEntry> m_Shaders;
        std::unordered_map<std::string, uint64_t> m_NameToId;
        std::unique_ptr<ShaderCompiler> m_Compiler;
        std::unique_ptr<ShaderReflection> m_Reflection;
        
        // State
        uint64_t m_NextShaderId = 1;
        uint64_t m_CurrentlyBound = 0;
        bool m_Initialized = false;
        
        // Settings
        std::string m_ShaderDirectory;
        std::string m_CacheDirectory;
        bool m_HotReloadEnabled = false;
        
        // Callbacks
        ShaderReloadCallback m_ReloadCallback;
        ShaderErrorCallback m_ErrorCallback;
        
        // Statistics
        mutable std::mutex m_Mutex;
        Stats m_Stats;

        Impl()
        {
            m_Compiler = std::make_unique<ShaderCompiler>();
            m_Reflection = std::make_unique<ShaderReflection>();
        }

        uint64_t GenerateId()
        {
            return m_NextShaderId++;
        }

        ShaderHandle CreateHandle(uint64_t id)
        {
            return ShaderHandle(id);
        }
    };

    // ============================================================================
    // ShaderManager Implementation
    // ============================================================================

    ShaderManager::ShaderManager()
        : m_Impl(std::make_unique<Impl>())
    {
    }

    ShaderManager::~ShaderManager()
    {
        if (m_Impl->m_Initialized)
        {
            Shutdown();
        }
    }

    Result<void> ShaderManager::Initialize(const std::string& shaderDirectory, const std::string& cacheDirectory, bool enableHotReload)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);

        if (m_Impl->m_Initialized)
        {
            return Result<void>(ErrorCode::EngineAlreadyInitialized, "ShaderManager already initialized");
        }

        m_Impl->m_ShaderDirectory = shaderDirectory;
        m_Impl->m_CacheDirectory = cacheDirectory;
        m_Impl->m_HotReloadEnabled = enableHotReload;

        // Initialize compiler with settings
        m_Impl->m_Compiler->SetCachingEnabled(true, cacheDirectory);
        
        if (enableHotReload)
        {
            m_Impl->m_Compiler->SetHotReloadEnabled(true, {shaderDirectory});
        }

        m_Impl->m_Initialized = true;
        
        VX_CORE_INFO("ShaderManager initialized");
        VX_CORE_INFO("  Shader Directory: {0}", shaderDirectory);
        VX_CORE_INFO("  Cache Directory: {0}", cacheDirectory);
        VX_CORE_INFO("  Hot Reload: {0}", enableHotReload ? "Enabled" : "Disabled");

        return Result<void>();
    }

    void ShaderManager::Shutdown()
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);

        if (!m_Impl->m_Initialized)
        {
            return;
        }

        // Clear all shaders
        ClearAllShaders();

        m_Impl->m_Initialized = false;
        VX_CORE_INFO("ShaderManager shut down");
    }

    Result<ShaderHandle> ShaderManager::LoadShader(const std::string& name, const std::string& filePath, const ShaderCompileOptions& options)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);

        if (!m_Impl->m_Initialized)
        {
            return Result<ShaderHandle>(ErrorCode::InvalidState, "ShaderManager not initialized");
        }

        // Check if shader with this name already exists
        auto nameIt = m_Impl->m_NameToId.find(name);
        if (nameIt != m_Impl->m_NameToId.end())
        {
            return Result<ShaderHandle>(ErrorCode::InvalidState, "Shader with name '" + name + "' already exists");
        }

        // Compile shader
        auto compileResult = m_Impl->m_Compiler->CompileFromFile(filePath, options);
        if (!compileResult.IsSuccess())
        {
            if (m_Impl->m_ErrorCallback)
            {
                std::vector<ShaderCompileError> errors;
                m_Impl->m_ErrorCallback(name, errors);
            }
            return Result<ShaderHandle>(compileResult.GetErrorCode(), compileResult.GetErrorMessage());
        }

        const CompiledShader& compiledShader = compileResult.GetValue();

        // Create render API shader
        auto shader = Vortex::GPUShader::Create(name);
        if (!shader)
        {
            return Result<ShaderHandle>(ErrorCode::RendererInitFailed, "Failed to create shader object");
        }

        // Convert compiled shader to format needed by Shader::Create
        std::unordered_map<Vortex::Shader::ShaderStage, std::vector<uint32_t>> shaderMap;
        shaderMap[compiledShader.Stage] = compiledShader.SpirV;

        auto createResult = shader->Create(shaderMap, compiledShader.Reflection);
        if (!createResult.IsSuccess())
        {
            return Result<ShaderHandle>(createResult.GetErrorCode(), createResult.GetErrorMessage());
        }

        // Store shader
        uint64_t id = m_Impl->GenerateId();
        Impl::ShaderEntry entry;
        entry.Name = name;
        entry.FilePath = filePath;
        entry.Shader = std::move(shader);
        entry.CompiledData = compiledShader;
        entry.Options = options;
        // entry.FileHash = FileSystem::GetFileHash(filePath);

        m_Impl->m_Shaders[id] = std::move(entry);
        m_Impl->m_NameToId[name] = id;

        m_Impl->m_Stats.LoadedShaders++;

        VX_CORE_INFO("Loaded shader: {0} from {1}", name, filePath);

        return Result<ShaderHandle>(m_Impl->CreateHandle(id));
    }

    Result<ShaderHandle> ShaderManager::CreateShader(const std::string& name, const std::string& source, Vortex::Shader::ShaderStage stage, const ShaderCompileOptions& options)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);

        if (!m_Impl->m_Initialized)
        {
            return Result<ShaderHandle>(ErrorCode::InvalidState, "ShaderManager not initialized");
        }

        // Check if shader with this name already exists
        auto nameIt = m_Impl->m_NameToId.find(name);
        if (nameIt != m_Impl->m_NameToId.end())
        {
            return Result<ShaderHandle>(ErrorCode::InvalidState, "Shader with name '" + name + "' already exists");
        }

        // Compile shader
        auto compileResult = m_Impl->m_Compiler->CompileFromSource(source, stage, options);
        if (!compileResult.IsSuccess())
        {
            if (m_Impl->m_ErrorCallback)
            {
                std::vector<ShaderCompileError> errors;
                m_Impl->m_ErrorCallback(name, errors);
            }
            return Result<ShaderHandle>(compileResult.GetErrorCode(), compileResult.GetErrorMessage());
        }

        const CompiledShader& compiledShader = compileResult.GetValue();

        // Create render API shader
        auto shader = Vortex::GPUShader::Create(name);
        if (!shader)
        {
            return Result<ShaderHandle>(ErrorCode::RendererInitFailed, "Failed to create shader object");
        }

        // Convert compiled shader to format needed by Shader::Create
        std::unordered_map<Vortex::Shader::ShaderStage, std::vector<uint32_t>> shaderMap;
        shaderMap[compiledShader.Stage] = compiledShader.SpirV;

        auto createResult = shader->Create(shaderMap, compiledShader.Reflection);
        if (!createResult.IsSuccess())
        {
            return Result<ShaderHandle>(createResult.GetErrorCode(), createResult.GetErrorMessage());
        }

        // Store shader
        uint64_t id = m_Impl->GenerateId();
        Impl::ShaderEntry entry;
        entry.Name = name;
        entry.FilePath = "";  // No file path for source-created shaders
        entry.Shader = std::move(shader);
        entry.CompiledData = compiledShader;
        entry.Options = options;
        entry.FileHash = 0;

        m_Impl->m_Shaders[id] = std::move(entry);
        m_Impl->m_NameToId[name] = id;

        m_Impl->m_Stats.LoadedShaders++;

        VX_CORE_INFO("Created shader from source: {0}", name);

        return Result<ShaderHandle>(m_Impl->CreateHandle(id));
    }

    const CompiledShader* ShaderManager::GetShader(ShaderHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        auto it = m_Impl->m_Shaders.find(handle.GetId());
        if (it != m_Impl->m_Shaders.end())
        {
            return &it->second.CompiledData;
        }
        return nullptr;
    }

    ShaderHandle ShaderManager::FindShader(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        auto it = m_Impl->m_NameToId.find(name);
        if (it != m_Impl->m_NameToId.end())
        {
            return m_Impl->CreateHandle(it->second);
        }
        return ShaderHandle(); // Invalid handle
    }

    Result<void> ShaderManager::BindShader(ShaderHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        auto it = m_Impl->m_Shaders.find(handle.GetId());
        if (it == m_Impl->m_Shaders.end())
        {
            return Result<void>(ErrorCode::InvalidParameter, "Invalid shader handle");
        }

        if (!it->second.Shader || !it->second.Shader->IsValid())
        {
            return Result<void>(ErrorCode::InvalidState, "Shader is not valid");
        }

        it->second.Shader->Bind();
        m_Impl->m_CurrentlyBound = handle.GetId();

        return Result<void>();
    }

    void ShaderManager::UnbindShader()
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        if (m_Impl->m_CurrentlyBound != 0)
        {
            auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
            if (it != m_Impl->m_Shaders.end() && it->second.Shader)
            {
                it->second.Shader->Unbind();
            }
            m_Impl->m_CurrentlyBound = 0;
        }
    }

    ShaderHandle ShaderManager::GetCurrentShader() const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        return m_Impl->CreateHandle(m_Impl->m_CurrentlyBound);
    }

    void ShaderManager::ClearAllShaders()
    {
        // Unbind current shader first
        UnbindShader();

        // Clear all data
        m_Impl->m_Shaders.clear();
        m_Impl->m_NameToId.clear();
        m_Impl->m_Stats = {};
        m_Impl->m_NextShaderId = 1;

        VX_CORE_INFO("Cleared all shaders");
    }

    ShaderManager::Stats ShaderManager::GetStats() const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        return m_Impl->m_Stats;
    }

    std::string ShaderManager::GetDebugInfo() const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        std::stringstream ss;
        ss << "=== Shader Manager Debug Info ===\n";
        ss << "Loaded Shaders: " << m_Impl->m_Stats.LoadedShaders << "\n";
        ss << "Currently Bound: " << m_Impl->m_CurrentlyBound << "\n";
        ss << "Hot Reload: " << (m_Impl->m_HotReloadEnabled ? "Enabled" : "Disabled") << "\n\n";
        
        ss << "Loaded Shaders:\n";
        for (const auto& [id, entry] : m_Impl->m_Shaders)
        {
            ss << "  ID: " << id << ", Name: " << entry.Name;
            if (!entry.FilePath.empty())
                ss << ", File: " << entry.FilePath;
            ss << "\n";
        }
        
        return ss.str();
    }

    // Stub implementations for unimplemented methods
    Result<ShaderHandle> ShaderManager::LoadShaderProgram(const std::string& name, const std::unordered_map<Vortex::Shader::ShaderStage, std::string>& shaderFiles, const ShaderCompileOptions& options)
    {
        return Result<ShaderHandle>(ErrorCode::NotImplemented, "LoadShaderProgram not yet implemented");
    }

    Result<std::unordered_map<uint64_t, ShaderHandle>> ShaderManager::LoadShaderVariants(const std::string& name, const std::string& filePath, const std::vector<ShaderMacros>& variants, const ShaderCompileOptions& options)
    {
        return Result<std::unordered_map<uint64_t, ShaderHandle>>(ErrorCode::NotImplemented, "LoadShaderVariants not yet implemented");
    }

    const ShaderProgram* ShaderManager::GetShaderProgram(ShaderHandle handle) const
    {
        return nullptr; // Not yet implemented
    }

    const ShaderReflectionData* ShaderManager::GetReflectionData(ShaderHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        auto it = m_Impl->m_Shaders.find(handle.GetId());
        if (it != m_Impl->m_Shaders.end())
        {
            return &it->second.CompiledData.Reflection;
        }
        return nullptr;
    }

    // Uniform management - delegate to currently bound shader
    Result<void> ShaderManager::SetUniform(const std::string& name, const void* data, uint32_t size)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        if (m_Impl->m_CurrentlyBound == 0)
        {
            return Result<void>(ErrorCode::InvalidState, "No shader currently bound");
        }

        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it == m_Impl->m_Shaders.end() || !it->second.Shader)
        {
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        }

        return it->second.Shader->SetUniform(name, data, size);
    }

    Result<void> ShaderManager::SetUniformBuffer(uint32_t binding, uint32_t bufferId, uint32_t offset, uint32_t size)
    {
        return Result<void>(ErrorCode::NotImplemented, "SetUniformBuffer not yet implemented");
    }

    // Convenient uniform setters
    Result<void> ShaderManager::SetUniform(const std::string& name, int value)
    {
        return SetUniform(name, &value, sizeof(int));
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, float value)
    {
        return SetUniform(name, &value, sizeof(float));
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::vec2& value)
    {
        return SetUniform(name, &value, sizeof(glm::vec2));
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::vec3& value)
    {
        return SetUniform(name, &value, sizeof(glm::vec3));
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::vec4& value)
    {
        return SetUniform(name, &value, sizeof(glm::vec4));
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::mat3& value)
    {
        return SetUniform(name, &value, sizeof(glm::mat3));
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::mat4& value)
    {
        return SetUniform(name, &value, sizeof(glm::mat4));
    }

    // Stubs for other methods
    Result<void> ShaderManager::ReloadShader(ShaderHandle handle) { return Result<void>(ErrorCode::NotImplemented, "Not implemented"); }
    uint32_t ShaderManager::ReloadAllShaders() { return 0; }
    void ShaderManager::RemoveShader(ShaderHandle handle) {}
    uint32_t ShaderManager::ValidateAllShaders() { return 0; }
    void ShaderManager::SetShaderReloadCallback(ShaderReloadCallback callback) { m_Impl->m_ReloadCallback = callback; }
    void ShaderManager::SetShaderErrorCallback(ShaderErrorCallback callback) { m_Impl->m_ErrorCallback = callback; }

    // ============================================================================
    // Global Instance
    // ============================================================================

    ShaderManager& GetShaderManager()
    {
        static ShaderManager s_Instance;
        return s_Instance;
    }

} // namespace Vortex::Shader
