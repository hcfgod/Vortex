#pragma once

#include "Engine/Systems/EngineSystem.h"
#include "Engine/Assets/Asset.h"
#include "Engine/Assets/AssetHandle.h"
#include "Engine/Assets/ShaderAsset.h"
#include "Engine/Assets/TextureAsset.h"
#include "Engine/Renderer/Shader/ShaderCompiler.h"
#include "Engine/Renderer/GraphicsContext.h"
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

    struct TextureLoadOptions
    {
        int DesiredChannels = 4;  // 0=auto, 1=grayscale, 2=grayscale+alpha, 3=RGB, 4=RGBA
        bool FlipVertically = true;  // Whether to flip texture vertically on load
        TextureFormat Format = TextureFormat::RGBA8;  // Target GPU format
        
        // Default constructor provides sensible defaults
        TextureLoadOptions() = default;
        
        // Convenience constructors for common use cases
        static TextureLoadOptions LoadRGBA() { 
            TextureLoadOptions opts;
            opts.DesiredChannels = 4;
            opts.FlipVertically = true;
            opts.Format = TextureFormat::RGBA8;
            return opts;
        }
        static TextureLoadOptions LoadRGB() { 
            TextureLoadOptions opts;
            opts.DesiredChannels = 3;
            opts.FlipVertically = true;
            opts.Format = TextureFormat::RGB8;
            return opts;
        }
        static TextureLoadOptions LoadGrayscale() { 
            TextureLoadOptions opts;
            opts.DesiredChannels = 1;
            opts.FlipVertically = true;
            opts.Format = TextureFormat::RGB8;
            return opts;
        }
        static TextureLoadOptions LoadGrayscaleAlpha() { 
            TextureLoadOptions opts;
            opts.DesiredChannels = 2;
            opts.FlipVertically = true;
            opts.Format = TextureFormat::RGBA8;
            return opts;
        }
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

        // Recursive lookup helpers (search under a root for matching files)
        std::filesystem::path FindFirstFileRecursive(const std::filesystem::path& root, const std::string& fileName) const;
        bool FindShaderPairRecursive(const std::filesystem::path& root, const std::string& baseName, std::filesystem::path& outVertexPath, std::filesystem::path& outFragmentPath) const;
        std::filesystem::path FindTextureRecursive(const std::filesystem::path& root, const std::string& baseNameOrFile) const;

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

        // Asset loading helpers
        AssetHandle<ShaderAsset> LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath, const ShaderCompileOptions& options, ProgressCallback onProgress);
        AssetHandle<ShaderAsset> LoadShaderFromManifest(const std::string& manifestPath, const ShaderCompileOptions& defaultOptions, ProgressCallback onProgress);
        AssetHandle<TextureAsset> LoadTexture(const std::string& name, const std::string& filePath, const TextureLoadOptions& options, ProgressCallback onProgress);
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
        std::error_code ec;
        fs::path assetsRoot = m_DevAssetsAvailable ? m_DevAssetsRoot : m_AssetsRoot;
        fs::path p(name);

        // 0) Absolute path provided
        if (p.is_absolute())
        {
            return LoadTexture(p.filename().string(), p.string(), {}, std::move(onProgress));
        }

        // 1) If name already includes subdirectories, try direct resolution first
        if (p.has_parent_path())
        {
            fs::path abs = assetsRoot / p;
            if (fs::exists(abs, ec))
            {
                if (fs::is_regular_file(abs, ec))
                    return LoadTexture(p.filename().string(), abs.string(), {}, std::move(onProgress));
                if (fs::is_directory(abs, ec))
                {
                    fs::path found = FindTextureRecursive(abs, p.filename().string());
                    if (!found.empty())
                        return LoadTexture(found.filename().string(), found.string(), {}, std::move(onProgress));
                }
            }

            if (m_DevAssetsAvailable)
            {
                fs::path absDev = m_DevAssetsRoot / p;
                if (fs::exists(absDev, ec))
                {
                    if (fs::is_regular_file(absDev, ec))
                        return LoadTexture(p.filename().string(), absDev.string(), {}, std::move(onProgress));
                    
                    if (fs::is_directory(absDev, ec))
                    {
                        fs::path found = FindTextureRecursive(absDev, p.filename().string());
                        if (!found.empty())
                            return LoadTexture(found.filename().string(), found.string(), {}, std::move(onProgress));
                    }
                }
            }
        }

        // 2) Try common direct locations under Assets
        fs::path candidates[] = 
        {
            assetsRoot / fs::path("Textures") / p,
            assetsRoot / p
        };

        for (const auto& c : candidates)
        {
            if (fs::exists(c, ec))
            {
                if (fs::is_regular_file(c, ec))
                    return LoadTexture(p.filename().string(), c.string(), {}, std::move(onProgress));
                if (fs::is_directory(c, ec))
                {
                    fs::path found = FindTextureRecursive(c, p.filename().string());
                    if (!found.empty())
                        return LoadTexture(found.filename().string(), found.string(), {}, std::move(onProgress));
                }
            }
        }

        // 3) Recursively search DevAssets first (if available), then packaged Assets
        if (m_DevAssetsAvailable)
        {
            fs::path found = FindTextureRecursive(m_DevAssetsRoot, p.has_filename() ? p.filename().string() : name);
            if (!found.empty())
                return LoadTexture(found.filename().string(), found.string(), {}, std::move(onProgress));
        }
        {
            fs::path found = FindTextureRecursive(m_AssetsRoot, p.has_filename() ? p.filename().string() : name);
            if (!found.empty())
                return LoadTexture(found.filename().string(), found.string(), {}, std::move(onProgress));
        }

        // 4) Fall back to Assets/Textures/<name> (may trigger procedural fallback if missing)
        fs::path rel = fs::path("Textures") / p;
        fs::path abs = assetsRoot / rel;

        return LoadTexture(p.filename().string(), abs.string(), {}, std::move(onProgress));
    }

    // ShaderAsset specialization
    template<>
    inline AssetHandle<ShaderAsset> AssetSystem::LoadAsset<ShaderAsset>(const std::string& name, ProgressCallback onProgress)
    {
        namespace fs = std::filesystem;
        ShaderCompileOptions options{}; // default
        // Select target profile based on active graphics API
        switch (GetGraphicsAPI())
        {
            case GraphicsAPI::OpenGL:   options.TargetProfile = "opengl";      break;
            case GraphicsAPI::Vulkan:   options.TargetProfile = "vulkan1.1";   break;
            case GraphicsAPI::DirectX11:
            case GraphicsAPI::DirectX12:
            case GraphicsAPI::Metal:

            default:                    options.TargetProfile = "opengl";      break; // fallback for now
        }
        std::error_code ec;

        // 1) Prefer manifest if available directly in conventional Shaders/ folder
        fs::path manifestRel = fs::path("Shaders") / (name + ".json");
        fs::path manifestAbs = m_DevAssetsAvailable ? (m_DevAssetsRoot / manifestRel) : (m_AssetsRoot / manifestRel);
        if (fs::exists(manifestAbs, ec))
        {
            return LoadShaderFromManifest(manifestAbs.string(), options, std::move(onProgress));
        }

        // 2) Otherwise try conventional VS/FS filenames directly in Shaders/
        fs::path vsRel = fs::path("Shaders") / (name + ".vert");
        fs::path fsRel = fs::path("Shaders") / (name + ".frag");
        fs::path vsAbs = m_DevAssetsAvailable ? (m_DevAssetsRoot / vsRel) : (m_AssetsRoot / vsRel);
        fs::path fsAbs = m_DevAssetsAvailable ? (m_DevAssetsRoot / fsRel) : (m_AssetsRoot / fsRel);
        if (fs::exists(vsAbs, ec) && fs::exists(fsAbs, ec))
        {
            return LoadShader(name, vsAbs.string(), fsAbs.string(), options, std::move(onProgress));
        }

        // 3) If not found, recursively search under DevAssets first (when available), then packaged Assets
        if (m_DevAssetsAvailable)
        {
            fs::path manifestFound = FindFirstFileRecursive(m_DevAssetsRoot, name + ".json");
            if (!manifestFound.empty())
                return LoadShaderFromManifest(manifestFound.string(), options, std::move(onProgress));

            fs::path vsFound, fsFound;
            if (FindShaderPairRecursive(m_DevAssetsRoot, name, vsFound, fsFound))
                return LoadShader(name, vsFound.string(), fsFound.string(), options, std::move(onProgress));
        }

        {
            fs::path manifestFound = FindFirstFileRecursive(m_AssetsRoot, name + ".json");
            if (!manifestFound.empty())
                return LoadShaderFromManifest(manifestFound.string(), options, std::move(onProgress));

            fs::path vsFound, fsFound;
            if (FindShaderPairRecursive(m_AssetsRoot, name, vsFound, fsFound))
                return LoadShader(name, vsFound.string(), fsFound.string(), options, std::move(onProgress));
        }

        // 4) As a last resort, point to conventional Shaders/<name> files (may fail and use fallback)
        return LoadShader(name, vsAbs.string(), fsAbs.string(), options, std::move(onProgress));
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