#pragma once

#include "Engine/Systems/EngineSystem.h"
#include "Engine/Assets/Asset.h"
#include "Engine/Assets/AssetHandle.h"
#include "Engine/Assets/ShaderAsset.h"
#include "Engine/Assets/TextureAsset.h"
#include "Engine/Renderer/Shader/ShaderCompiler.h"
#include "Core/Async/Task.h"
#include "Core/Async/Coroutine.h"
#include "Core/Debug/Log.h"
#include <unordered_map>
#include <queue>
#include <deque>
#include <mutex>
#include <filesystem>
#include <unordered_set>

namespace Vortex
{
    struct ShaderManifest
    {
        std::string Name;
        std::string VertexPath;
        std::string FragmentPath;
    };

    // Progress callback signature: 0..1
    using ProgressCallback = std::function<void(float)>;

    class AssetSystem : public EngineSystem
    {
    public:
        AssetSystem();
        ~AssetSystem() override;

        Result<void> Initialize() override;
        Result<void> Update() override;
        Result<void> Shutdown() override;

        // Assets root directory (default resolves to Assets next to exe)
        void SetAssetsRoot(const std::filesystem::path& root);
        std::filesystem::path GetAssetsRoot() const { return m_AssetsRoot; }

        // Name and UUID lookup
        template<typename T>
        AssetHandle<T> GetByUUID(const UUID& id) { return AssetHandle<T>(this, id); }
        template<typename T>
        AssetHandle<T> GetByName(const std::string& name);

        // Preferred generic asset loading API
        template<typename T>
        AssetHandle<T> LoadAsset(const std::string& name, ProgressCallback onProgress = {});
        template<typename T>
        AssetHandle<T> LoadAssetByUUID(const UUID& id) { return GetByUUID<T>(id); }

        // Legacy explicit loaders (kept for now; will be phased out)
        // Load shader via simple manifest (json or direct guess)
        AssetHandle<ShaderAsset> LoadShaderAsync(const std::string& name,
            const std::string& vertexPath,
            const std::string& fragmentPath,
            const ShaderCompileOptions& options = {},
            ProgressCallback onProgress = {});
        AssetHandle<ShaderAsset> LoadShaderFromManifestAsync(const std::string& manifestPath,
            const ShaderCompileOptions& defaultOptions = {},
            ProgressCallback onProgress = {});

        // Load texture (simple async loader with procedural fallback)
        AssetHandle<TextureAsset> LoadTextureAsync(const std::string& name,
            const std::string& filePath,
            ProgressCallback onProgress = {});

        // Ref management used by AssetHandle (friend)
        void Acquire(const UUID& id);
        void Release(const UUID& id);

        // Internal accessors for AssetHandle
        bool IsLoaded(const UUID& id) const;
        float GetProgress(const UUID& id) const;
        template<typename T>
        T* TryGet(const UUID& id);
        template<typename T>
        const T* TryGet(const UUID& id) const;

        // Builder stubs (for release builds to prebuild assets)
        Result<void> BuildShader(const std::string& name,
            const std::string& vertexPath,
            const std::string& fragmentPath,
            const std::filesystem::path& outputDir);

    private:
        struct ShaderSourceInfo
        {
            std::string VertexPath;
            std::string FragmentPath;
            ShaderCompileOptions Options{};
            std::filesystem::file_time_type VertexLastWrite{};
            std::filesystem::file_time_type FragmentLastWrite{};
        };

        struct AssetEntry
        {
            std::shared_ptr<Asset> assetPtr;
            std::string Name; // for name lookup
        };

        struct PendingUnload
        {
            UUID Id;
            double UnloadAtSeconds = 0.0; // absolute engine time when eligible
        };

        UUID RegisterAsset(const std::shared_ptr<Asset>& asset);
        void UnregisterAsset(const UUID& id);

        // Internal helpers for unload policy
        void ScheduleUnload(const UUID& id);
        void CancelScheduledUnload(const UUID& id);
        bool CanUnloadNow_NoLock(const UUID& id) const; // assumes m_Mutex locked
        void PerformUnload_NoLock(const UUID& id);

        Task<void> CompileShaderTask(UUID id,
            std::string name,
            std::string vertexPath,
            std::string fragmentPath,
            ShaderCompileOptions options,
            ProgressCallback progress,
            bool isReload = false);

        // Simple built-in fallback shader
        void EnsureFallbackShader();

        // Periodic shader hot-reload check
        void CheckForShaderHotReloads();

    private:
        mutable std::mutex m_Mutex;
        std::unordered_map<UUID, AssetEntry> m_Assets;
        std::unordered_map<std::string, UUID> m_NameToUUID;
        std::unordered_map<UUID, uint32_t> m_Refs;
        std::vector<Task<void>> m_PendingTasks;

        // Shader hot-reload tracking
        std::unordered_map<UUID, ShaderSourceInfo> m_ShaderSources;
        std::unordered_set<UUID> m_ShaderReloading;
        double m_LastHotReloadCheckTime = 0.0;
        double m_HotReloadIntervalSeconds = 0.5;
        bool m_ShaderHotReloadEnabled = true;

        // Basic texture loader does not hot-reload yet

        // Delayed unload policy and state
        std::deque<PendingUnload> m_PendingUnloads;
        std::unordered_map<UUID, double> m_IdToScheduledTime;
        double m_UnloadGracePeriodSeconds = 5.0; // default grace window

        std::filesystem::path m_AssetsRoot;
        std::filesystem::path m_DevAssetsRoot; // Optional: repo-level Assets for development
        bool m_DevAssetsAvailable = false;
        ShaderRef m_FallbackShader;
        bool m_FallbackInitialized = false;
        bool m_Initialized = false;
    };

    // ===== Inline/template implementations =====

    template<typename T>
    AssetHandle<T> AssetSystem::GetByName(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_NameToUUID.find(name);
        if (it == m_NameToUUID.end())
            return {};
        return AssetHandle<T>(this, it->second);
    }

    template<typename T>
    T* AssetSystem::TryGet(const UUID& id)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Assets.find(id);
        if (it == m_Assets.end()) return nullptr;
        return dynamic_cast<T*>(it->second.assetPtr.get());
    }

    template<typename T>
    const T* AssetSystem::TryGet(const UUID& id) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Assets.find(id);
        if (it == m_Assets.end()) return nullptr;
        return dynamic_cast<const T*>(it->second.assetPtr.get());
    }

    // Generic LoadAsset default (static_assert for unsupported types)
    template<typename T>
    AssetHandle<T> AssetSystem::LoadAsset(const std::string& /*name*/, ProgressCallback /*onProgress*/)
    {
        static_assert(sizeof(T) == 0, "LoadAsset<T>: Unsupported asset type T");
        return {};
    }

    // TextureAsset specialization
    template<>
    inline AssetHandle<TextureAsset> AssetSystem::LoadAsset<TextureAsset>(const std::string& name, ProgressCallback onProgress)
    {
        namespace fs = std::filesystem;
        // Resolve to Assets/Textures/<name>
        fs::path rel = fs::path("Textures") / name;
        fs::path abs = m_DevAssetsAvailable ? (m_DevAssetsRoot / rel) : (m_AssetsRoot / rel);
        return LoadTextureAsync(name, abs.string(), std::move(onProgress));
    }

    // ShaderAsset specialization
    template<>
    inline AssetHandle<ShaderAsset> AssetSystem::LoadAsset<ShaderAsset>(const std::string& name, ProgressCallback onProgress)
    {
        namespace fs = std::filesystem;
        ShaderCompileOptions options{}; // default
        // Prefer manifest if available: Assets/Shaders/<name>.json
        fs::path manifestRel = fs::path("Shaders") / (name + ".json");
        fs::path manifestAbs = m_DevAssetsAvailable ? (m_DevAssetsRoot / manifestRel) : (m_AssetsRoot / manifestRel);
        std::error_code ec;
        if (fs::exists(manifestAbs, ec))
        {
            return LoadShaderFromManifestAsync(manifestAbs.string(), options, std::move(onProgress));
        }
        // Otherwise try conventional VS/FS filenames
        fs::path vsRel = fs::path("Shaders") / (name + ".vert");
        fs::path fsRel = fs::path("Shaders") / (name + ".frag");
        fs::path vsAbs = m_DevAssetsAvailable ? (m_DevAssetsRoot / vsRel) : (m_AssetsRoot / vsRel);
        fs::path fsAbs = m_DevAssetsAvailable ? (m_DevAssetsRoot / fsRel) : (m_AssetsRoot / fsRel);
        return LoadShaderAsync(name, vsAbs.string(), fsAbs.string(), options, std::move(onProgress));
    }

    // AssetHandle inline method defs
    template<typename T>
    inline bool AssetHandle<T>::IsLoaded() const
    {
        return m_System && m_System->IsLoaded(m_Id);
    }

    template<typename T>
    inline float AssetHandle<T>::GetProgress() const
    {
        return m_System ? m_System->GetProgress(m_Id) : 0.0f;
    }

    template<typename T>
    inline const T* AssetHandle<T>::TryGet() const
    {
        return m_System ? m_System->TryGet<T>(m_Id) : nullptr;
    }

    template<typename T>
    inline T* AssetHandle<T>::TryGet()
    {
        return m_System ? m_System->TryGet<T>(m_Id) : nullptr;
    }
}