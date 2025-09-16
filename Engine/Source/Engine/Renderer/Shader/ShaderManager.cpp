#include "vxpch.h"
#include "ShaderManager.h"
#include "Shader.h"
#include "Core/Debug/ErrorCodes.h"
#include "Core/Debug/Log.h"
#include "Core/Debug/Assert.h"
#include "Core/Debug/Exception.h"
#include "Core/Debug/CrashHandler.h"
#include "Core/Application.h"
#include "Engine/Engine.h"
#include "Engine/Systems/SystemManager.h"
#include "Engine/Assets/AssetSystem.h"
#include "Engine/Assets/ShaderAsset.h"

#include <unordered_map>
#include <mutex>

namespace Vortex
{
    // ============================================================================
    // ShaderManager::Impl
    // ============================================================================

    class ShaderManager::Impl
    {
    public:
        struct ShaderEntry
        {
            std::string Name;
            // Use AssetSystem-managed shader assets
            AssetHandle<ShaderAsset> Handle;
        };

        // Core data
        std::unordered_map<uint64_t, ShaderEntry> m_Shaders; // key: UUID value (uint64)
        std::unordered_map<std::string, uint64_t> m_NameToId;
        AssetSystem* m_AssetSystem = nullptr; // non-owning
        
        // State
        uint64_t m_CurrentlyBound = 0; // UUID value
        bool m_Initialized = false;
        
        // Settings (kept for compatibility/logging)
        std::string m_ShaderDirectory;
        std::string m_CacheDirectory;
        bool m_HotReloadEnabled = false;
        
        // Callbacks
        ShaderReloadCallback m_ReloadCallback;
        ShaderErrorCallback m_ErrorCallback;
        
        // Statistics
        mutable std::mutex m_Mutex;
        Stats m_Stats;

        Impl() = default;
    };

    // ============================================================================
    // ShaderManager Implementation
    // ============================================================================

    ShaderManager::ShaderManager() : m_Impl(std::make_unique<Impl>())
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

        // Acquire AssetSystem
        m_Impl->m_AssetSystem = nullptr;
        if (auto* app = Application::Get())
        {
            if (auto* eng = app->GetEngine())
            {
                m_Impl->m_AssetSystem = eng->GetSystemManager().GetSystem<AssetSystem>();
            }
        }
        if (!m_Impl->m_AssetSystem)
        {
            VX_CORE_ERROR("ShaderManager: AssetSystem not available. Initialization will proceed but most operations will fail.");
        }

        m_Impl->m_Initialized = true;
        
        VX_CORE_INFO("ShaderManager initialized (AssetSystem-backed)");
        VX_CORE_INFO("  Shader Directory: {0}", shaderDirectory);
        VX_CORE_INFO("  Cache Directory: {0}", cacheDirectory);
        VX_CORE_INFO("  Hot Reload: {0}", enableHotReload ? "Enabled" : "Disabled");

        return Result<void>();
    }

    void ShaderManager::Shutdown()
    {
        // First mark as not initialized under lock to block new operations
        {
            std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
            if (!m_Impl->m_Initialized)
            {
                return;
            }
            m_Impl->m_Initialized = false;
        }

        // Then clear resources (this function takes its own lock)
        ClearAllShaders();

        VX_CORE_INFO("ShaderManager shut down");
    }

    Result<AssetHandle<ShaderAsset>> ShaderManager::LoadShader(const std::string& name, const std::string& filePath, const ShaderCompileOptions& options)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);

        if (!m_Impl->m_Initialized)
        {
            return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidState, "ShaderManager not initialized");
        }
        if (!m_Impl->m_AssetSystem)
        {
            return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidState, "AssetSystem not available");
        }

        // If a shader with this name already exists, return its handle
        auto nameIt = m_Impl->m_NameToId.find(name);
        if (nameIt != m_Impl->m_NameToId.end())
        {
            auto sit = m_Impl->m_Shaders.find(nameIt->second);
            if (sit != m_Impl->m_Shaders.end())
            {
                return Result<AssetHandle<ShaderAsset>>(sit->second.Handle);
            }
            return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidState, "Shader mapping inconsistent");
        }

        // Interpret filePath as a manifest when it ends with .json
        if (filePath.size() >= 5 && filePath.substr(filePath.size() - 5) == ".json")
        {
            auto handle = m_Impl->m_AssetSystem->LoadAsset<ShaderAsset>(name, [](float){});
            if (!handle.IsValid())
            {
                return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidParameter, "Failed to request shader manifest: " + filePath);
            }
            uint64_t id = static_cast<uint64_t>(handle.GetId());
            Impl::ShaderEntry entry;
            entry.Name = name;
            entry.Handle = std::move(handle);
            m_Impl->m_Shaders[id] = std::move(entry);
            m_Impl->m_NameToId[name] = id;
            m_Impl->m_Stats.LoadedShaders++;
            VX_CORE_INFO("ShaderManager: Requested shader '{}' from manifest {} (AssetSystem)", name, filePath);
            return Result<AssetHandle<ShaderAsset>>(m_Impl->m_Shaders[id].Handle);
        }

        // Otherwise, this API is ambiguous (single file). Encourage using LoadShaderProgram.
        return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidParameter, "Use LoadShaderProgram() with stage files or provide a manifest (.json)");
    }

    Result<AssetHandle<ShaderAsset>> ShaderManager::LoadShaderProgram(const std::string& name, const std::unordered_map<ShaderStage, std::string>& shaderFiles, const ShaderCompileOptions& options)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);

        if (!m_Impl->m_Initialized)
        {
            return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidState, "ShaderManager not initialized");
        }
        if (!m_Impl->m_AssetSystem)
        {
            return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidState, "AssetSystem not available");
        }

        auto nameIt = m_Impl->m_NameToId.find(name);
        if (nameIt != m_Impl->m_NameToId.end())
        {
            auto sit = m_Impl->m_Shaders.find(nameIt->second);
            if (sit != m_Impl->m_Shaders.end())
            {
                return Result<AssetHandle<ShaderAsset>>(sit->second.Handle);
            }
            return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidState, "Shader mapping inconsistent");
        }

        auto vsIt = shaderFiles.find(ShaderStage::Vertex);
        auto fsIt = shaderFiles.find(ShaderStage::Fragment);
        if (vsIt == shaderFiles.end() || fsIt == shaderFiles.end())
        {
            return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidParameter, "Vertex and Fragment paths required");
        }

        auto handle = m_Impl->m_AssetSystem->LoadAsset<ShaderAsset>(name, [](float){});
        if (!handle.IsValid())
        {
            return Result<AssetHandle<ShaderAsset>>(ErrorCode::InvalidParameter, "Failed to request shader program");
        }

        uint64_t id = static_cast<uint64_t>(handle.GetId());
        Impl::ShaderEntry entry; entry.Name = name; entry.Handle = std::move(handle);
        m_Impl->m_Shaders[id] = std::move(entry);
        m_Impl->m_NameToId[name] = id;
        m_Impl->m_Stats.LoadedPrograms++;

        VX_CORE_INFO("ShaderManager: Requested program '{}' (VS='{}', FS='{}') via AssetSystem", name, vsIt->second, fsIt->second);
        return Result<AssetHandle<ShaderAsset>>(m_Impl->m_Shaders[id].Handle);
    }

    Result<std::unordered_map<uint64_t, AssetHandle<ShaderAsset>>> ShaderManager::LoadShaderVariants(const std::string& name, const std::string& filePath, const std::vector<ShaderMacros>& variants, const ShaderCompileOptions& options)
    {
        return Result<std::unordered_map<uint64_t, AssetHandle<ShaderAsset>>>(ErrorCode::NotImplemented, "LoadShaderVariants not yet implemented (consider extending AssetSystem)");
    }

    AssetHandle<ShaderAsset> ShaderManager::FindShader(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        auto it = m_Impl->m_NameToId.find(name);
        if (it != m_Impl->m_NameToId.end())
        {
            auto sit = m_Impl->m_Shaders.find(it->second);
            if (sit != m_Impl->m_Shaders.end())
            {
                return sit->second.Handle;
            }
        }

        return AssetHandle<ShaderAsset>(); // Invalid handle
    }

    Result<void> ShaderManager::BindShader(const AssetHandle<ShaderAsset>& handle)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);

        if (!m_Impl->m_Initialized)
        {
            return Result<void>(ErrorCode::InvalidState, "ShaderManager not initialized");
        }
        if (!handle.IsValid() || !handle.IsLoaded())
        {
            return Result<void>(ErrorCode::InvalidParameter, "Invalid or not yet loaded shader handle");
        }

        const ShaderAsset* asset = handle.TryGet();
        if (!asset || !asset->IsReady() || !asset->GetShader())
        {
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");
        }

        // Ensure this handle exists in our registry so SetUniform/Unbind work with m_CurrentlyBound.
        const uint64_t id = static_cast<uint64_t>(handle.GetId());
        auto it = m_Impl->m_Shaders.find(id);
        if (it == m_Impl->m_Shaders.end())
        {
            Impl::ShaderEntry entry;
            entry.Name = asset->GetName();
            entry.Handle = handle;
            m_Impl->m_Shaders[id] = entry;
            // Only add name mapping if not present to avoid clobbering existing names.
            if (!entry.Name.empty() && m_Impl->m_NameToId.find(entry.Name) == m_Impl->m_NameToId.end())
            {
                m_Impl->m_NameToId[entry.Name] = id;
            }
        }

        asset->GetShader()->Bind();
        m_Impl->m_CurrentlyBound = id;
        return Result<void>();
    }

    void ShaderManager::UnbindShader()
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        if (m_Impl->m_CurrentlyBound != 0)
        {
            auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
            if (it != m_Impl->m_Shaders.end())
            {
                if (const ShaderAsset* asset = it->second.Handle.TryGet())
                {
                    if (asset->GetShader()) asset->GetShader()->Unbind();
                }
            }
            m_Impl->m_CurrentlyBound = 0;
        }
    }

    AssetHandle<ShaderAsset> ShaderManager::GetCurrentShader() const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        if (m_Impl->m_CurrentlyBound == 0) return AssetHandle<ShaderAsset>();
        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it != m_Impl->m_Shaders.end())
            return it->second.Handle;
        return AssetHandle<ShaderAsset>();
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
            float progress = entry.Handle.GetProgress();
            bool ready = entry.Handle.IsLoaded();
            ss << "  ID: " << id << ", Name: " << entry.Name << ", Ready: " << (ready ? "Yes" : "No") << ", Progress: " << static_cast<int>(progress * 100.0f) << "%\n";
        }
        
        return ss.str();
    }

    const ShaderProgram* ShaderManager::GetShaderProgram(const AssetHandle<ShaderAsset>& handle) const
    {
        return nullptr; // Not yet implemented
    }

    const ShaderReflectionData* ShaderManager::GetReflectionData(const AssetHandle<ShaderAsset>& handle) const
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        
        auto it = m_Impl->m_Shaders.find(static_cast<uint64_t>(handle.GetId()));
        if (it != m_Impl->m_Shaders.end())
        {
            if (const ShaderAsset* asset = it->second.Handle.TryGet())
            {
                // Asset stores reflection for the complete program
                return &asset->GetReflection();
            }
        }
        return nullptr;
    }

    void ShaderManager::ClearAllShaders()
    {
        // Take the lock here and avoid re-entrant locking by inlining the unbind logic
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);

        // Unbind current shader if any (inline to avoid locking twice)
        if (m_Impl->m_CurrentlyBound != 0)
        {
            auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
            if (it != m_Impl->m_Shaders.end())
            {
                if (const ShaderAsset* asset = it->second.Handle.TryGet())
                {
                    if (asset->GetShader())
                        asset->GetShader()->Unbind();
                }
            }
            m_Impl->m_CurrentlyBound = 0;
        }

        // Clear all data
        m_Impl->m_Shaders.clear();
        m_Impl->m_NameToId.clear();
        m_Impl->m_Stats = {};

        VX_CORE_INFO("Cleared all shaders (AssetSystem-backed)");
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
        if (it == m_Impl->m_Shaders.end())
        {
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        }

        const ShaderAsset* asset = it->second.Handle.TryGet();
        if (!asset || !asset->GetShader())
        {
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");
        }

        return asset->GetShader()->SetUniform(name, data, size);
    }

    Result<void> ShaderManager::SetUniformBuffer(uint32_t binding, uint32_t bufferId, uint32_t offset, uint32_t size)
    {
        return Result<void>(ErrorCode::NotImplemented, "SetUniformBuffer not yet implemented");
    }

    // Convenient uniform setters - call the underlying GPUShader typed setters to avoid generic path warnings
    Result<void> ShaderManager::SetUniform(const std::string& name, int value)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        if (m_Impl->m_CurrentlyBound == 0)
            return Result<void>(ErrorCode::InvalidState, "No shader currently bound");
        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it == m_Impl->m_Shaders.end())
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        const ShaderAsset* asset = it->second.Handle.TryGet();
        if (!asset || !asset->GetShader())
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");
        return asset->GetShader()->SetUniform(name, value);
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, float value)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        if (m_Impl->m_CurrentlyBound == 0)
            return Result<void>(ErrorCode::InvalidState, "No shader currently bound");
        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it == m_Impl->m_Shaders.end())
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        const ShaderAsset* asset = it->second.Handle.TryGet();
        if (!asset || !asset->GetShader())
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");
        return asset->GetShader()->SetUniform(name, value);
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::vec2& value)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        if (m_Impl->m_CurrentlyBound == 0)
            return Result<void>(ErrorCode::InvalidState, "No shader currently bound");
        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it == m_Impl->m_Shaders.end())
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        const ShaderAsset* asset = it->second.Handle.TryGet();
        if (!asset || !asset->GetShader())
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");
        return asset->GetShader()->SetUniform(name, value);
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::vec3& value)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        if (m_Impl->m_CurrentlyBound == 0)
            return Result<void>(ErrorCode::InvalidState, "No shader currently bound");
        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it == m_Impl->m_Shaders.end())
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        const ShaderAsset* asset = it->second.Handle.TryGet();
        if (!asset || !asset->GetShader())
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");
        return asset->GetShader()->SetUniform(name, value);
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::vec4& value)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        if (m_Impl->m_CurrentlyBound == 0)
            return Result<void>(ErrorCode::InvalidState, "No shader currently bound");
        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it == m_Impl->m_Shaders.end())
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        const ShaderAsset* asset = it->second.Handle.TryGet();
        if (!asset || !asset->GetShader())
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");
        return asset->GetShader()->SetUniform(name, value);
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::mat3& value)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        if (m_Impl->m_CurrentlyBound == 0)
            return Result<void>(ErrorCode::InvalidState, "No shader currently bound");
        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it == m_Impl->m_Shaders.end())
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        const ShaderAsset* asset = it->second.Handle.TryGet();
        if (!asset || !asset->GetShader())
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");
        return asset->GetShader()->SetUniform(name, value);
    }

    Result<void> ShaderManager::SetUniform(const std::string& name, const glm::mat4& value)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        if (m_Impl->m_CurrentlyBound == 0)
            return Result<void>(ErrorCode::InvalidState, "No shader currently bound");
        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it == m_Impl->m_Shaders.end())
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        const ShaderAsset* asset = it->second.Handle.TryGet();
        if (!asset || !asset->GetShader())
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");
        return asset->GetShader()->SetUniform(name, value);
    }

    Result<void> ShaderManager::SetTexture(const std::string& name, uint32_t textureId, uint32_t slot)
    {
        std::lock_guard<std::mutex> lock(m_Impl->m_Mutex);
        if (m_Impl->m_CurrentlyBound == 0)
            return Result<void>(ErrorCode::InvalidState, "No shader currently bound");
        auto it = m_Impl->m_Shaders.find(m_Impl->m_CurrentlyBound);
        if (it == m_Impl->m_Shaders.end())
            return Result<void>(ErrorCode::InvalidState, "Invalid bound shader");
        const ShaderAsset* asset = it->second.Handle.TryGet();
        if (!asset || !asset->GetShader())
            return Result<void>(ErrorCode::InvalidState, "Shader asset not ready");

        // Set the sampler uniform and bind texture via render command queue helper
        auto res = asset->GetShader()->SetTexture(name, textureId, slot);
        return res;
    }

    // Stubs for other methods
    Result<void> ShaderManager::ReloadShader(const AssetHandle<ShaderAsset>& handle) { return Result<void>(ErrorCode::NotImplemented, "Not implemented"); }
    uint32_t ShaderManager::ReloadAllShaders() { return 0; }
    void ShaderManager::RemoveShader(const AssetHandle<ShaderAsset>& handle) {}
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

} // namespace Vortex