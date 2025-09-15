#pragma once

#include "Engine/Systems/EngineSystem.h"
#include "Engine/Assets/Asset.h"
#include "Engine/Assets/AssetHandle.h"
#include "Engine/Assets/ShaderAsset.h"
#include "Engine/Renderer/Shader/ShaderCompiler.h"
#include "Core/Async/Task.h"
#include "Core/Async/Coroutine.h"
#include "Core/Debug/Log.h"
#include <unordered_map>
#include <queue>
#include <deque>
#include <mutex>
#include <filesystem>

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

        // Load shader via simple manifest (json or direct guess)
        AssetHandle<ShaderAsset> LoadShaderAsync(const std::string& name,
            const std::string& vertexPath,
            const std::string& fragmentPath,
            const ShaderCompileOptions& options = {},
            ProgressCallback onProgress = {});
        AssetHandle<ShaderAsset> LoadShaderFromManifestAsync(const std::string& manifestPath,
            const ShaderCompileOptions& defaultOptions = {},
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
            ProgressCallback progress);

        // Simple built-in fallback shader
        void EnsureFallbackShader();

    private:
        mutable std::mutex m_Mutex;
        std::unordered_map<UUID, AssetEntry> m_Assets;
        std::unordered_map<std::string, UUID> m_NameToUUID;
        std::unordered_map<UUID, uint32_t> m_Refs;
        std::vector<Task<void>> m_PendingTasks;

        // Delayed unload policy and state
        std::deque<PendingUnload> m_PendingUnloads;
        std::unordered_map<UUID, double> m_IdToScheduledTime;
        double m_UnloadGracePeriodSeconds = 5.0; // default grace window

        std::filesystem::path m_AssetsRoot;
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