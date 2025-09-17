#include "vxpch.h"
#include "AssetSystem.h"
#include "Core/FileSystem.h"
#include "Core/EngineConfig.h"
#include "Engine/Time/Time.h"
#include "Engine/Renderer/Shader/ShaderReflection.h"
#include "Engine/Renderer/RendererAPI.h"
#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <fstream>
#include <random>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace Vortex
{
    AssetSystem::AssetSystem() : EngineSystem("AssetSystem", SystemPriority::Core)
    {
    }

    AssetSystem::~AssetSystem()
    {
        if (m_Initialized)
        {
            Shutdown();
        }
    }

    Result<void> AssetSystem::Initialize()
    {
        if (m_Initialized)
            return Result<void>();

        // Default assets root: try next to executable, fallback to relative Assets/
        auto exeDir = FileSystem::GetExecutableDirectory();
        if (exeDir.has_value())
            m_AssetsRoot = exeDir.value() / "Assets";
        else
            m_AssetsRoot = std::filesystem::path("Assets");

        // If no Assets directory exists and no pack is present, generate minimal defaults
        try
        {
            namespace fs = std::filesystem;
            std::error_code ec;
            fs::path packCandidate = m_AssetsRoot.parent_path() / "Assets.vxpack";
            bool haveAssetsDir = fs::exists(m_AssetsRoot, ec) && fs::is_directory(m_AssetsRoot, ec);
            bool havePack = fs::exists(packCandidate, ec) && fs::is_regular_file(packCandidate, ec);
            if (!haveAssetsDir && !havePack)
            {
                GenerateDefaultAssetsOnDisk(m_AssetsRoot);
            }
        }
        catch (...)
        {
        }

        // Try to detect repository-level Assets directory for development hot reload
        m_DevAssetsAvailable = false;
        m_DevAssetsRoot.clear();
#if !defined(VX_DIST)
        try
        {
            namespace fs = std::filesystem;
            fs::path probe = exeDir.has_value() ? exeDir.value() : fs::current_path();
            // Walk up to 5 levels up looking for an 'Assets' directory
            for (int i = 0; i < 5; ++i)
            {
                fs::path candidate = probe / "Assets";
                std::error_code ec;
                if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec))
                {
                    m_DevAssetsRoot = candidate;
                    m_DevAssetsAvailable = true;
                    break;
                }
                if (probe.has_parent_path()) probe = probe.parent_path(); else break;
            }
            if (m_DevAssetsAvailable)
            {
                VX_CORE_INFO("AssetSystem: Dev Assets detected at: {}", m_DevAssetsRoot.string());
            }
        }
        catch (const std::exception& e)
        {
            VX_CORE_WARN("AssetSystem: Dev assets detection failed: {}", e.what());
        }
#endif

        // Attempt to load packaged asset pack next to executable (Assets.vxpack)
        try
        {
            namespace fs = std::filesystem;
            fs::path packPath = m_AssetsRoot.parent_path() / "Assets.vxpack";
            if (fs::exists(packPath))
            {
                m_AssetPackAvailable = m_AssetPack.Load(packPath);
                if (m_AssetPackAvailable)
                {
                    m_AssetPackPath = packPath;
                    VX_CORE_INFO("AssetSystem: Asset pack detected at: {}", m_AssetPackPath.string());
                }
            }
        }
        catch (...)
        {
        }

        // Defer fallback shader creation until a graphics context exists (after RenderSystem initializes)
        m_FallbackInitialized = false;

        m_Initialized = true;
        VX_CORE_INFO("AssetSystem initialized. AssetsRoot: {}", m_AssetsRoot.string());
        MarkInitialized();
        return Result<void>();
    }

    void AssetSystem::SetWorkingDirectory(const std::filesystem::path& dir)
    {
        namespace fs = std::filesystem;
        m_AssetsRoot = dir / "Assets";
        m_AssetPackPath = dir / "Assets.vxpack";
        std::error_code ec;
        if (!fs::exists(m_AssetsRoot, ec) && !fs::exists(m_AssetPackPath, ec))
        {
            GenerateDefaultAssetsOnDisk(m_AssetsRoot);
        }

        // Reload pack if present
        try
        {
            if (fs::exists(m_AssetPackPath, ec))
            {
                m_AssetPackAvailable = m_AssetPack.Load(m_AssetPackPath);
            }
        }
        catch (...) {}
    }

    void AssetSystem::GenerateDefaultAssetsOnDisk(const std::filesystem::path& outAssetsRoot)
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::create_directories(outAssetsRoot / "Shaders", ec);
        fs::create_directories(outAssetsRoot / "Textures", ec);

        // Write default shader sources and manifest
        // Basic passthrough matching our EnsureFallbackShader uniforms and attributes
        const char* vs = 
        R"(#version 450 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aUV;
            layout(location = 2) in vec3 aNormal;
            layout(location = 3) in vec3 aTangent;
            layout(location = 0) uniform mat4 u_ViewProjection;
            layout(location = 4) uniform mat4 u_Model;
            void main(){ gl_Position = u_ViewProjection * u_Model * vec4(aPos,1.0); }
            )";
            const char* fsSrc = R"(#version 450 core
            layout(location = 0) out vec4 FragColor;
            layout(location = 8) uniform vec4 u_Color;
            void main(){ FragColor = u_Color; }
        )";

        try
        {
            std::ofstream vso(outAssetsRoot / "Shaders" / "Default.vert", std::ios::binary);
            vso.write(vs, std::strlen(vs));
            std::ofstream fso(outAssetsRoot / "Shaders" / "Default.frag", std::ios::binary);
            fso.write(fsSrc, std::strlen(fsSrc));
            std::ofstream mo(outAssetsRoot / "Shaders" / "Default.json", std::ios::binary);
            const char* manifest = 
            R"({
                "name": "Default",
                "vertex": "Shaders/Default.vert",
                "fragment": "Shaders/Default.frag",
                "options": { "TargetProfile": "opengl" }
            })";
                
            mo.write(manifest, std::strlen(manifest));
        }
        catch (...) {}

        // Write a tiny checker texture (procedurally generated)
        try
        {
            const int w = 64, h = 64, check = 8;
            std::vector<unsigned char> pixels(w * h * 4);
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    bool c = ((x / check) % 2) ^ ((y / check) % 2);
                    unsigned char v = c ? 255 : 50;
                    size_t idx = (static_cast<size_t>(y) * w + x) * 4ull;
                    pixels[idx+0] = v;
                    pixels[idx+1] = v;
                    pixels[idx+2] = v;
                    pixels[idx+3] = 255;
                }
            }
            // Write as raw .rgba to avoid additional encoders; loader path reads via stb anyway
            std::ofstream to(outAssetsRoot / "Textures" / "Checker.rgba", std::ios::binary);
            to.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
        }
        catch (...) {}
    }

    Result<void> AssetSystem::Update()
    {
        // Lazily initialize fallback shader once renderer is ready
        if (!m_FallbackInitialized)
        {
            if (auto* renderer = GetRenderer())
            {
                if (renderer->IsInitialized())
                {
                    EnsureFallbackShader();
                    m_FallbackInitialized = true;
                }
            }
        }

        // Clean up finished tasks
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_PendingTasks.begin();
            while (it != m_PendingTasks.end())
            {
                if (it->IsCompleted())
                    it = m_PendingTasks.erase(it);
                else
                    ++it;
            }
        }

        // Periodic shader hot-reload polling
        if (m_ShaderHotReloadEnabled)
        {
            const double now = Time::GetUnscaledTime();
            if (now - m_LastHotReloadCheckTime >= m_HotReloadIntervalSeconds)
            {
                m_LastHotReloadCheckTime = now;
                CheckForShaderHotReloads();
            }
        }

        // Process delayed unloads respecting grace period and dependencies
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            const double now = Time::GetUnscaledTime();
            auto unloadIt = m_PendingUnloads.begin();
            while (unloadIt != m_PendingUnloads.end())
            {
                const UUID id = unloadIt->Id;
                const double when = unloadIt->UnloadAtSeconds;
                // If rescheduled or removed, ensure we consult the authoritative map
                auto timeIt = m_IdToScheduledTime.find(id);
                if (timeIt == m_IdToScheduledTime.end())
                {
                    unloadIt = m_PendingUnloads.erase(unloadIt);
                    continue;
                }
                // Only act when the front item reaches time
                if (now < when)
                {
                    ++unloadIt;
                    continue;
                }
                // Only unload if still at zero refs and no dependents require it
                if (CanUnloadNow_NoLock(id))
                {
                    PerformUnload_NoLock(id);
                }
                // Remove from queue regardless; if still not unloadable it can be rescheduled on next release
                m_IdToScheduledTime.erase(id);
                unloadIt = m_PendingUnloads.erase(unloadIt);
            }
        }
        return Result<void>();
    }

    Result<void> AssetSystem::Shutdown()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_PendingTasks.clear();
        m_Assets.clear();
        m_NameToUUID.clear();
        m_Refs.clear();
        m_FallbackShader.reset();
        m_Initialized = false;
        MarkShutdown();
        VX_CORE_INFO("AssetSystem shut down");
        return Result<void>();
    }

    void AssetSystem::SetAssetsRoot(const std::filesystem::path& root)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_AssetsRoot = root;
        VX_CORE_INFO("AssetSystem assets root set to: {}", m_AssetsRoot.string());
    }

    // ===== Recursive helpers =====
    std::filesystem::path AssetSystem::FindFirstFileRecursive(const std::filesystem::path& root, const std::string& fileName) const
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::exists(root, ec) || !fs::is_directory(root, ec))
            return {};

        auto toLower = [](std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        };

        const std::string targetLower = toLower(fileName);
        for (fs::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec))
        {
            if (ec) { ec.clear(); continue; }
            if (!it->is_regular_file(ec)) { if (ec) ec.clear(); continue; }
            const std::string fname = it->path().filename().string();
            if (toLower(fname) == targetLower)
                return it->path();
        }
        return {};
    }

    bool AssetSystem::FindShaderPairRecursive(const std::filesystem::path& root, const std::string& baseName, std::filesystem::path& outVertexPath, std::filesystem::path& outFragmentPath) const
    {
        auto endsWithNoCase = [](const std::string& str, const std::string& suffix)
        {
            if (suffix.size() > str.size()) return false;
            return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(), [](char a, char b){ return std::tolower(a) == std::tolower(b); });
        };

        // Try exact base name first
        std::filesystem::path vs = FindFirstFileRecursive(root, baseName + ".vert");
        std::filesystem::path fs = FindFirstFileRecursive(root, baseName + ".frag");
        if (!vs.empty() && !fs.empty())
        {
            outVertexPath = std::move(vs);
            outFragmentPath = std::move(fs);
            return true;
        }

        // Heuristic: strip trailing "Shader" if present
        std::string stripped = baseName;
        if (endsWithNoCase(stripped, "Shader"))
            stripped.erase(stripped.size() - 6);
        if (!stripped.empty())
        {
            vs = FindFirstFileRecursive(root, stripped + ".vert");
            fs = FindFirstFileRecursive(root, stripped + ".frag");
            if (!vs.empty() && !fs.empty())
            {
                outVertexPath = std::move(vs);
                outFragmentPath = std::move(fs);
                return true;
            }
        }
        return false;
    }

    std::filesystem::path AssetSystem::FindTextureRecursive(const std::filesystem::path& root, const std::string& baseNameOrFile) const
    {
        namespace fs = std::filesystem;
        fs::path p(baseNameOrFile);
        if (p.has_extension())
        {
            return FindFirstFileRecursive(root, p.filename().string());
        }

        static const char* exts[] = { "png", "jpg", "jpeg", "bmp", "tga", "ktx", "dds", "rgba" };
        for (const char* ext : exts)
        {
            std::string candidate = baseNameOrFile + "." + ext;
            fs::path found = FindFirstFileRecursive(root, candidate);
            if (!found.empty())
                return found;
        }
        return {};
    }

    void AssetSystem::Acquire(const UUID& id)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Refs[id] += 1;
        auto it = m_Assets.find(id);
        if (it != m_Assets.end() && it->second.assetPtr)
            it->second.assetPtr->AddRef();
        // Cancel pending unload if any
        CancelScheduledUnload(id);
    }

    void AssetSystem::Release(const UUID& id)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Refs.find(id);
        if (it == m_Refs.end()) return;
        if (it->second > 0)
            it->second -= 1;
        auto a = m_Assets.find(id);
        if (a != m_Assets.end() && a->second.assetPtr)
            a->second.assetPtr->ReleaseRef();
        if (it->second == 0)
        {
            // Schedule delayed unload instead of immediate
            ScheduleUnload(id);
        }
    }

    bool AssetSystem::IsLoaded(const UUID& id) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Assets.find(id);
        if (it == m_Assets.end()) return false;
        return it->second.assetPtr->GetState() == AssetState::Loaded;
    }

    float AssetSystem::GetProgress(const UUID& id) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Assets.find(id);
        if (it == m_Assets.end()) return 0.0f;
        return it->second.assetPtr->GetProgress();
    }

    UUID AssetSystem::RegisterAsset(const std::shared_ptr<Asset>& asset)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        UUID id = asset->GetId();
        AssetEntry entry; entry.assetPtr = asset; entry.Name = asset->GetName();
        m_NameToUUID[entry.Name] = id;
        m_Assets[id] = std::move(entry);
        return id;
    }

    void AssetSystem::UnregisterAsset(const UUID& id)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Assets.find(id);
        if (it != m_Assets.end())
        {
            m_NameToUUID.erase(it->second.Name);
            m_Assets.erase(it);
            m_Refs.erase(id);
            CancelScheduledUnload(id);
        }
    }

    AssetHandle<ShaderAsset> AssetSystem::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath, const ShaderCompileOptions& options, ProgressCallback onProgress)
    {
        // Create placeholder asset entry
        auto shaderAsset = std::make_shared<ShaderAsset>(name);
        shaderAsset->SetState(AssetState::Loading);
        shaderAsset->SetProgress(0.0f);
        UUID id = RegisterAsset(shaderAsset);

        // Kick off compile coroutine and keep task alive in pending list
        auto task = CompileShaderTask(id, name, vertexPath, fragmentPath, options, std::move(onProgress), false);
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_PendingTasks.emplace_back(std::move(task));
            // Track for hot-reload
            ShaderSourceInfo info{};
            info.VertexPath = vertexPath;
            info.FragmentPath = fragmentPath;
            info.Options = options;
            // Initialize last write times now to avoid duplicate reload during initial compile
            std::error_code ec;
            namespace fs = std::filesystem;
            auto strip_assets_prefix = [](const fs::path& p) -> fs::path {
                if (p.empty()) return p;
                auto it = p.begin();
                if (it != p.end() && it->string() == "Assets")
                {
                    ++it;
                    fs::path acc;
                    for (; it != p.end(); ++it) acc /= *it;
                    return acc;
                }
                return p;
            };
            fs::path vpRel = strip_assets_prefix(fs::path(vertexPath));
            fs::path fpRel = strip_assets_prefix(fs::path(fragmentPath));
            fs::path vpAbs = (m_DevAssetsAvailable ? (m_DevAssetsRoot / vpRel) : (m_AssetsRoot / vpRel));
            fs::path fpAbs = (m_DevAssetsAvailable ? (m_DevAssetsRoot / fpRel) : (m_AssetsRoot / fpRel));
            if (!vpAbs.empty() && fs::exists(vpAbs, ec)) info.VertexLastWrite = fs::last_write_time(vpAbs, ec);
            if (!fpAbs.empty() && fs::exists(fpAbs, ec)) info.FragmentLastWrite = fs::last_write_time(fpAbs, ec);
            m_ShaderSources[id] = std::move(info);
        }

        return AssetHandle<ShaderAsset>(this, id);
    }

    AssetHandle<ShaderAsset> AssetSystem::LoadShaderFromManifest(const std::string& manifestPath, const ShaderCompileOptions& defaultOptions, ProgressCallback onProgress)
    {
        // Read JSON manifest (prefer from asset pack when available)
        using json = nlohmann::json;
        json j;
        bool loaded = false;
        if (m_AssetPackAvailable)
        {
            std::vector<uint8_t> bytes;
            std::string key = manifestPath;
            // Normalize path to be relative within Assets
            try
            {
                namespace fs = std::filesystem;
                fs::path p(manifestPath);
                if (p.is_absolute())
                {
                    auto s = p.string();
                    auto pos = s.rfind("Assets/");
                    if (pos != std::string::npos) key = s.substr(pos + 7);
                    else key = p.filename().string();
                }
                else
                {
                    key = p.generic_string();
                }
            }
            catch (...)
            {
            }
            if (!m_AssetPack.Read(key, bytes))
            {
                // Try Shaders/<name>.json fallback
                std::string alt = std::string("Shaders/") + std::filesystem::path(manifestPath).filename().string();
                m_AssetPack.Read(alt, bytes);
            }
            if (!bytes.empty())
            {
                try
                {
                    j = json::parse(bytes.begin(), bytes.end());
                    loaded = true;
                }
                catch (const std::exception& e)
                {
                    VX_CORE_ERROR("AssetSystem: Failed to parse manifest from pack '{}': {}", manifestPath, e.what());
                }
            }
        }
        if (!loaded)
        {
            std::filesystem::path path = manifestPath;
            if (path.is_relative())
                path = m_AssetsRoot / path;

            std::ifstream f(path);
            if (!f.is_open())
            {
                VX_CORE_ERROR("AssetSystem: Shader manifest not found: {}", path.string());
                return {};
            }
            f >> j;
        }

        ShaderManifest manifest;
        std::string defaultName;
        try { defaultName = std::filesystem::path(manifestPath).stem().string(); }
        catch (...) { defaultName = std::string(); }
        manifest.Name = j.value("name", defaultName);
        manifest.VertexPath = j.value("vertex", std::string{});
        manifest.FragmentPath = j.value("fragment", std::string{});

        ShaderCompileOptions options = defaultOptions;
        if (j.contains("options") && j["options"].is_object())
        {
            const auto& jo = j["options"];
            options.OptimizationLevel = jo.value("OptimizationLevel", options.OptimizationLevel);
            options.GenerateDebugInfo = jo.value("GenerateDebugInfo", options.GenerateDebugInfo);
            options.TargetProfile = jo.value("TargetProfile", options.TargetProfile);
        }

        // Resolve paths (prefer pack if present by letting LoadShader handle resolution)
        std::string vert = manifest.VertexPath;
        std::string frag = manifest.FragmentPath;
        // If manifests contain absolute paths, fall back to m_AssetsRoot
        try
        {
            namespace fs = std::filesystem;
            fs::path vp(vert), fp(frag);
            if (vp.is_relative()) vert = (m_AssetsRoot / vp).string();
            if (fp.is_relative()) frag = (m_AssetsRoot / fp).string();
        }
        catch (...) {}
        return LoadShader(manifest.Name, vert, frag, options, std::move(onProgress));
    }

    AssetHandle<TextureAsset> AssetSystem::LoadTexture(const std::string& name, const std::string& filePath, const TextureLoadOptions& options, ProgressCallback onProgress)
    {
        VX_CORE_INFO("AssetSystem: Loading texture '{}' from '{}'", name, filePath);
        // Create placeholder asset
        auto texAsset = std::make_shared<TextureAsset>(name);
        texAsset->SetState(AssetState::Loading);
        texAsset->SetProgress(0.0f);
        UUID id = RegisterAsset(texAsset);

        // Load from pack (preferred when available) or from disk using stb_image; fallback to solid magenta error texture
        Task<void> task = [this, id, name, filePath, options, onProgress]() -> Task<void>
        {
            auto setProgress = [&](float p)
            {
                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    auto it = m_Assets.find(id);
                    if (it != m_Assets.end()) it->second.assetPtr->SetProgress(p);
                }
                if (onProgress) onProgress(p);
            };

            setProgress(0.1f);

            uint32_t width = 0, height = 0;
            std::vector<uint8_t> pixels;

            // If asset pack is available, try reading raw bytes from pack first
            std::vector<uint8_t> packedBytes;
            bool loadedFromPack = false;
            if (m_AssetPackAvailable)
            {
                // Construct a relative key under Assets/ by stripping any absolute prefix
                try
                {
                    namespace fs = std::filesystem;
                    fs::path p(filePath);
                    std::string key = p.string();
                    if (p.is_absolute())
                    {
                        // Try to find substring after "Assets/"
                        auto pos = key.rfind("Assets/");
                        if (pos != std::string::npos)
                            key = key.substr(pos + 7); // skip "Assets/"
                        else if (m_DevAssetsAvailable)
                        {
                            // If dev assets root is present, make path relative to it
                            std::error_code ec;
                            auto rel = fs::relative(p, m_DevAssetsRoot, ec);
                            if (!ec) key = rel.generic_string();
                        }
                    }
                    if (!key.empty())
                    {
                        // Build a small list of candidate keys to try in the pack
                        std::vector<std::string> candidates;
                        candidates.emplace_back(key);
                        {
                            std::string fname = fs::path(key).filename().generic_string();
                            candidates.emplace_back(std::string("Textures/") + fname);
                            candidates.emplace_back(fname);
                        }

                        // Try direct reads first
                        for (const auto& k : candidates)
                        {
                            if (m_AssetPack.Read(k, packedBytes) && !packedBytes.empty())
                                break;
                        }

                        // If not found, try common image extensions when original key has no extension
                        fs::path keyPath(key);
                        if (packedBytes.empty())
                        {
                            static const char* kExts[] = { "png", "jpg", "jpeg", "bmp", "tga", "ktx", "dds" };
                            const bool hasExt = keyPath.has_extension();
                            if (!hasExt)
                            {
                                std::string dir = keyPath.has_parent_path() ? keyPath.parent_path().generic_string() : std::string();
                                std::string base = keyPath.filename().generic_string();
                                for (const char* ext : kExts)
                                {
                                    std::vector<std::string> extCandidates;
                                    if (!dir.empty())
                                        extCandidates.emplace_back(dir + "/" + base + "." + ext);
                                    extCandidates.emplace_back(std::string("Textures/") + base + "." + ext);
                                    extCandidates.emplace_back(base + "." + ext);

                                    for (const auto& k : extCandidates)
                                    {
                                        if (m_AssetPack.Read(k, packedBytes) && !packedBytes.empty())
                                            break;
                                    }
                                    if (!packedBytes.empty())
                                        break;
                                }
                            }
                        }
                        // As a last attempt, search by filename anywhere in the pack (with or without extension)
                        if (packedBytes.empty())
                        {
                            std::string baseName = keyPath.filename().generic_string();
                            std::string keyFound = m_AssetPack.FindFirstByFilename(baseName);
                            if (keyFound.empty())
                            {
                                static const char* kExts[] = { "png", "jpg", "jpeg", "bmp", "tga", "ktx", "dds" };
                                std::string stem = keyPath.filename().stem().generic_string();
                                for (const char* ext : kExts)
                                {
                                    keyFound = m_AssetPack.FindFirstByFilename(stem + "." + ext);
                                    if (!keyFound.empty()) break;
                                }
                            }
                            if (!keyFound.empty())
                                m_AssetPack.Read(keyFound, packedBytes);
                        }
                        if (!packedBytes.empty())
                        {
                            loadedFromPack = true;
                        }
                    }
                }
                catch (...)
                {
                }
            }

            // Attempt to load using stb_image
            stbi_set_flip_vertically_on_load(options.FlipVertically ? 1 : 0);
            int w = 0, h = 0, comp = 0;
            unsigned char* data = nullptr;
            if (loadedFromPack)
            {
                data = stbi_load_from_memory(packedBytes.data(), static_cast<int>(packedBytes.size()), &w, &h, &comp, options.DesiredChannels);
            }
            else
            {
                data = stbi_load(filePath.c_str(), &w, &h, &comp, options.DesiredChannels);
            }
            if (!data || w <= 0 || h <= 0)
            {
                const char* reason = stbi_failure_reason();
                if (reason) VX_CORE_WARN("AssetSystem: stbi_load failed for '{}': {}. Trying memory load...", filePath, reason);

                // Try wide/UTF-8 safe path via std::filesystem::path and load from memory
                try
                {
                    std::filesystem::path path(filePath);
                    std::error_code fec;
                    if (std::filesystem::exists(path, fec) && !std::filesystem::is_directory(path, fec))
                    {
                        std::ifstream fin(path, std::ios::binary);
                        if (fin)
                        {
                            fin.seekg(0, std::ios::end);
                            std::streamsize fsize = fin.tellg();
                            fin.seekg(0, std::ios::beg);
                            if (fsize > 0)
                            {
                                std::vector<unsigned char> fileBytes(static_cast<size_t>(fsize));
                                fin.read(reinterpret_cast<char*>(fileBytes.data()), fsize);
                                int mw = 0, mh = 0, mcomp = 0;
                                unsigned char* mdata = stbi_load_from_memory(fileBytes.data(), static_cast<int>(fileBytes.size()), &mw, &mh, &mcomp, options.DesiredChannels);
                                if (mdata && mw > 0 && mh > 0)
                                {
                                    w = mw; h = mh; comp = 4; data = mdata;
                                }
                                else
                                {
                                    const char* mreason = stbi_failure_reason();
                                    if (mreason) VX_CORE_WARN("AssetSystem: stbi_load_from_memory failed for '{}': {}", filePath, mreason);
                                }
                            }
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    VX_CORE_WARN("AssetSystem: Exception loading image '{}': {}", filePath, e.what());
                }
            }

            if (data && w > 0 && h > 0)
            {
                width = static_cast<uint32_t>(w);
                height = static_cast<uint32_t>(h);
                int actualChannels = options.DesiredChannels > 0 ? options.DesiredChannels : comp;
                pixels.assign(data, data + (width * height * actualChannels));
                stbi_image_free(data);
                VX_CORE_INFO("AssetSystem: Loaded image for '{}' ({}x{}, channels={})", name, width, height, actualChannels);
            }
            else
            {
                // Fallback solid magenta error texture
                width = 256; height = 256;
                pixels.resize(width * height * 4);
                for (uint32_t y = 0; y < height; ++y)
                {
                    for (uint32_t x = 0; x < width; ++x)
                    {
                        uint8_t r = 255; // Magenta red
                        uint8_t g = 0;   // Magenta green
                        uint8_t b = 255; // Magenta blue
                        uint8_t a = 255; // Full alpha
                        size_t idx = static_cast<size_t>(y) * width * 4ull + static_cast<size_t>(x) * 4ull;
                        pixels[idx + 0] = r;
                        pixels[idx + 1] = g;
                        pixels[idx + 2] = b;
                        pixels[idx + 3] = a;
                    }
                }
                VX_CORE_WARN("AssetSystem: Failed to decode image '{}' from '{}', using solid magenta error texture", name, filePath);
            }

            setProgress(0.5f);

            // Create GPU texture
            Texture2D::CreateInfo ci{};
            ci.Width = width;
            ci.Height = height;
            ci.Format = options.Format;
            ci.MinFilter = TextureFilter::Linear;
            ci.MagFilter = TextureFilter::Linear;
            ci.WrapS = TextureWrap::Repeat;
            ci.WrapT = TextureWrap::Repeat;
            ci.GenerateMips = true;
            ci.InitialData = pixels.data();
            ci.InitialDataSize = static_cast<uint64_t>(pixels.size());

            auto texture = Texture2D::Create(ci);
            if (!texture)
            {
                VX_CORE_ERROR("AssetSystem: Failed to create GPU texture for '{}' ({}x{})", name, width, height);
            }

            setProgress(0.9f);

            // Store into asset entry
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                auto it = m_Assets.find(id);
                if (it != m_Assets.end())
                {
                    auto* t = dynamic_cast<TextureAsset*>(it->second.assetPtr.get());
                    if (t)
                    {
                        t->SetTexture(texture);
                        t->SetState(AssetState::Loaded);
                        t->SetProgress(1.0f);
                        VX_CORE_INFO("AssetSystem: Texture '{}' loaded and ready", name);
                    }
                }
            }

            setProgress(1.0f);
            co_return;
        }();

        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_PendingTasks.emplace_back(std::move(task));
        }

        return AssetHandle<TextureAsset>(this, id);
    }

    Result<void> AssetSystem::BuildShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath, const std::filesystem::path& outputDir)
    {
        // Basic stub: compile and write SPIR-V blobs to outputDir for packaging
        ShaderCompiler compiler;
        ShaderCompileOptions options; // default or load from config
        // Select target profile based on active graphics API (match runtime)
        switch (GetGraphicsAPI())
        {
            case GraphicsAPI::OpenGL:   options.TargetProfile = "opengl";     break;
            case GraphicsAPI::Vulkan:   options.TargetProfile = "vulkan1.1";  break;
            case GraphicsAPI::DirectX11:
            case GraphicsAPI::DirectX12:
            case GraphicsAPI::Metal:
            default:                    options.TargetProfile = "opengl";     break;
        }
        if (auto* renderer = GetRenderer())
        {
            if (auto* ctx = renderer->GetContext())
            {
                const auto& info = ctx->GetInfo();
                options.HardwareCaps.MaxTextureUnits = info.MaxCombinedTextureUnits;
                options.HardwareCaps.MaxSamples = std::max(info.SampleCount, info.MaxSamples);
                options.HardwareCaps.SupportsGeometry = info.SupportsGeometryShaders;
                options.HardwareCaps.SupportsCompute = info.SupportsComputeShaders;
                options.HardwareCaps.SupportsMultiDrawIndirect = info.SupportsMultiDrawIndirect;
                options.HardwareCaps.SRGBFramebufferCapable = info.SRGBFramebufferCapable;
            }
        }
        auto vs = compiler.CompileFromFile(vertexPath, options);
        auto fs = compiler.CompileFromFile(fragmentPath, options);
        if (!vs.IsSuccess() || !fs.IsSuccess())
        {
            return Result<void>(ErrorCode::CompilationFailed, "Shader build failed");
        }
        std::filesystem::create_directories(outputDir);
        auto vout = outputDir / (name + ".vert.spv");
        auto fout = outputDir / (name + ".frag.spv");
        std::ofstream ov(vout, std::ios::binary);
        ov.write(reinterpret_cast<const char*>(vs.GetValue().SpirV.data()), vs.GetValue().SpirV.size() * sizeof(uint32_t));
        std::ofstream of(fout, std::ios::binary);
        of.write(reinterpret_cast<const char*>(fs.GetValue().SpirV.data()), fs.GetValue().SpirV.size() * sizeof(uint32_t));
        return Result<void>();
    }

    Result<std::filesystem::path> AssetSystem::BuildAssetsPack(const BuildAssetsOptions& options)
    {
        namespace fs = std::filesystem;
        std::error_code ec;

        fs::path assetsRoot = m_DevAssetsAvailable ? m_DevAssetsRoot : m_AssetsRoot;
        fs::path outputPack = options.OutputPackPath;
        if (outputPack.empty())
            outputPack = assetsRoot.parent_path() / "Assets.vxpack";

        // Optionally precompile shaders to Assets/Cache/Shaders
        fs::path cacheDir   = assetsRoot / "Cache" / "Shaders";
        if (options.PrecompileShaders && fs::exists(assetsRoot, ec))
        {
            fs::create_directories(cacheDir, ec);
            // Scan entire assets tree for vertex shaders and pair with fragment in same directory
            for (auto& p : fs::recursive_directory_iterator(assetsRoot, ec))
            {
                if (p.is_regular_file(ec) && p.path().extension() == ".vert")
                {
                    fs::path vs = p.path();
                    fs::path fp = vs;
                    fp.replace_extension(".frag");
                    if (fs::exists(fp, ec))
                    {
                        std::string shaderName = vs.stem().string();
                        auto res = BuildShader(shaderName, vs.string(), fp.string(), cacheDir);
                        if (res.IsError())
                        {
                            VX_CORE_WARN("BuildShader failed for {}: {}", shaderName, res.GetErrorMessage());
                        }
                    }
                }
            }
        }

        // Create writer and add assets (recursively include files under Assets root)
        AssetPackWriter writer;
        if (fs::exists(assetsRoot, ec) && fs::is_directory(assetsRoot, ec))
        {
            for (auto& p : fs::recursive_directory_iterator(assetsRoot, ec))
            {
                if (!p.is_regular_file(ec))
                    continue;
                auto ext = p.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                // Optionally skip raw shader sources unless requested
                if (!options.IncludeShaderSources && (ext == ".vert" || ext == ".frag"))
                    continue;
                auto rel = fs::relative(p.path(), assetsRoot, ec).generic_string();
                writer.AddFile(rel, p.path());
            }
        }

        // Ensure output directory exists
        fs::create_directories(outputPack.parent_path(), ec);
        if (!writer.WriteToFile(outputPack))
        {
            return Result<fs::path>(ErrorCode::FileCorrupted, "Failed to write asset pack");
        }

        VX_CORE_INFO("Asset pack written: {}", outputPack.string());
        return Result<fs::path>(outputPack);
    }
    Task<void> AssetSystem::CompileShaderTask(UUID id, std::string name, std::string vertexPath, std::string fragmentPath, ShaderCompileOptions options, ProgressCallback progress, bool isReload)
    {
        // Small staged progress model: 0.0-0.1 read, 0.1-0.8 compile, 0.8-1.0 create
        auto setProgress = [&](float p) 
        {
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                auto it = m_Assets.find(id);
                if (it != m_Assets.end())
                    it->second.assetPtr->SetProgress(p);
            }

            if (progress) progress(p);
        };

        setProgress(0.05f);

        ShaderCompiler compiler;
#if defined(VX_DIST)
        // In distribution builds, do not create on-disk cache directories
        compiler.SetCachingEnabled(false);
#else
        // Keep caching enabled so reloads update on-disk cache with fresh SPIR-V
        compiler.SetCachingEnabled(true, (m_AssetsRoot / "Cache/Shaders").string());
#endif

        setProgress(0.10f);

        // Try using precompiled SPIR-V from asset pack: prefer manifest mapping when available,
        // else attempt conventional Cache/Shaders/<name>.(vert|frag).spv
        if (m_AssetPackAvailable)
        {
            std::vector<uint8_t> vsBytesRaw, fsBytesRaw;
            std::string vsKey = std::string("Cache/Shaders/") + name + ".vert.spv";
            std::string fsKey = std::string("Cache/Shaders/") + name + ".frag.spv";
            bool haveVS = m_AssetPack.Read(vsKey, vsBytesRaw);
            bool haveFS = m_AssetPack.Read(fsKey, fsBytesRaw);
            if (haveVS && haveFS && (vsBytesRaw.size() % 4 == 0) && (fsBytesRaw.size() % 4 == 0))
            {
                std::vector<uint32_t> vsSpv(vsBytesRaw.size() / 4);
                std::vector<uint32_t> fsSpv(fsBytesRaw.size() / 4);
                memcpy(vsSpv.data(), vsBytesRaw.data(), vsBytesRaw.size());
                memcpy(fsSpv.data(), fsBytesRaw.data(), fsBytesRaw.size());

                // Reflect
                ShaderReflection refl;
                auto vsReflRes = refl.Reflect(vsSpv, ShaderStage::Vertex);
                auto fsReflRes = refl.Reflect(fsSpv, ShaderStage::Fragment);
                if (vsReflRes.IsSuccess() && fsReflRes.IsSuccess())
                {
                    ShaderReflectionData reflection = ShaderReflection::CombineReflections({ vsReflRes.GetValue(), fsReflRes.GetValue() });

                    // Create GPU shader
                    auto shader = GPUShader::Create(name);
                    std::unordered_map<ShaderStage, std::vector<uint32_t>> stages;
                    stages[ShaderStage::Vertex] = std::move(vsSpv);
                    stages[ShaderStage::Fragment] = std::move(fsSpv);
                    auto createRes = shader->Create(stages, reflection);
                    if (createRes.IsSuccess())
                    {
                        setProgress(0.95f);
                        {
                            std::lock_guard<std::mutex> lock(m_Mutex);
                            auto it = m_Assets.find(id);
                            if (it != m_Assets.end())
                            {
                                auto* shaderAsset = dynamic_cast<ShaderAsset*>(it->second.assetPtr.get());
                                if (shaderAsset)
                                {
                                    shaderAsset->SetShader(std::shared_ptr<GPUShader>(std::move(shader)));
                                    shaderAsset->SetReflection(reflection);
                                    shaderAsset->SetState(AssetState::Loaded);
                                    shaderAsset->SetProgress(1.0f);
                                    m_ShaderReloading.erase(id);
                                }
                            }
                        }
                        setProgress(1.0f);
                        co_return; // done using precompiled
                    }
                }
            }
        }

        // If running a Dist build and no precompiled SPIR-V was found in the asset pack,
        // try to load via manifest from pack; if still not found, use fallback instead of compiling.
#if defined(VX_DIST)
        if (m_AssetPackAvailable)
        {
            // Attempt to load a manifest directly from the pack and enqueue again
            try
            {
                using json = nlohmann::json;
                std::vector<uint8_t> manifestBytes;
                std::string manifestKey = std::string("Shaders/") + name + ".json";
                if (m_AssetPack.Read(manifestKey, manifestBytes))
                {
                    json j = json::parse(manifestBytes.begin(), manifestBytes.end());
                    std::string vertRel = j.value("vertex", std::string{});
                    std::string fragRel = j.value("fragment", std::string{});
                    if (!vertRel.empty() && !fragRel.empty())
                    {
                        // Try again: LoadShader will re-enter CompileShaderTask; pack path will be tried first next time
                        auto h = LoadShader(name, vertRel, fragRel, options, {});
                        (void)h;
                    }
                }
            }
            catch (...) {}

            // Already attempted pack above; if it failed, use fallback
            VX_CORE_WARN("AssetSystem: Dist build: missing precompiled shader '{}' in asset pack. Using fallback shader.", name);
            EnsureFallbackShader();
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                auto it = m_Assets.find(id);
                if (it != m_Assets.end())
                {
                    auto* shaderAsset = dynamic_cast<ShaderAsset*>(it->second.assetPtr.get());
                    if (shaderAsset && m_FallbackShader && m_FallbackShader->IsValid())
                    {
                        shaderAsset->SetShader(m_FallbackShader);
                        shaderAsset->SetReflection({});
                        shaderAsset->SetIsFallback(true);
                        shaderAsset->SetState(AssetState::Loaded);
                        shaderAsset->SetProgress(1.0f);
                        m_ShaderReloading.erase(id);
                        co_return;
                    }
                }
            }
            // No fallback available; mark failed
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                auto it = m_Assets.find(id);
                if (it != m_Assets.end())
                {
                    it->second.assetPtr->SetState(AssetState::Failed);
                    it->second.assetPtr->SetProgress(1.0f);
                }
            }
            m_ShaderReloading.erase(id);
            co_return;
        }
#endif

        // Resolve relative paths against AssetsRoot for robustness
        std::string resolvedVS = vertexPath;
        std::string resolvedFS = fragmentPath;
        try
        {
            namespace fs = std::filesystem;
            auto strip_assets_prefix = [](const fs::path& p) -> fs::path {
                if (p.empty()) return p;
                auto it = p.begin();
                if (it != p.end() && it->string() == "Assets")
                {
                    ++it;
                    fs::path acc;
                    for (; it != p.end(); ++it) acc /= *it;
                    return acc;
                }
                return p;
                };

            if (!vertexPath.empty())
            {
                fs::path vp(vertexPath);
                if (vp.is_relative())
                {
                    vp = strip_assets_prefix(vp);
                    // Prefer dev assets root when available for hot-reload
                    if (m_DevAssetsAvailable)
                        resolvedVS = (m_DevAssetsRoot / vp).string();
                    else
                        resolvedVS = (m_AssetsRoot / vp).string();
                }
            }
            if (!fragmentPath.empty())
            {
                fs::path fp(fragmentPath);
                if (fp.is_relative())
                {
                    fp = strip_assets_prefix(fp);
                    if (m_DevAssetsAvailable)
                        resolvedFS = (m_DevAssetsRoot / fp).string();
                    else
                        resolvedFS = (m_AssetsRoot / fp).string();
                }
            }
        }
        catch (const std::exception& e)
        {
            VX_CORE_WARN("AssetSystem: Exception while resolving shader paths: {}", e.what());
        }

        VX_CORE_INFO("AssetSystem: %s shader '%s'\n  VS: %s\n  FS: %s",
            isReload ? "Recompiling" : "Compiling", name.c_str(), resolvedVS.c_str(), resolvedFS.c_str());

        // Compile asynchronously (run concurrently). If asset pack has precompiled SPIR-V in Cache/Shaders, try load-from-cache path first.
        auto vsTask = compiler.CompileFromFileAsync(resolvedVS, options, CoroutinePriority::Low);
        auto fsTask = compiler.CompileFromFileAsync(resolvedFS, options, CoroutinePriority::Low);

        // Await completion
        auto vsRes = co_await vsTask;
        auto fsRes = co_await fsTask;

        if (!vsRes.IsSuccess() || !fsRes.IsSuccess())
        {
            VX_CORE_ERROR("AssetSystem: Shader compilation failed for '{}': VS={} FS={}", name,
                vsRes.IsSuccess(), fsRes.IsSuccess());
            if (!vsRes.IsSuccess())
                VX_CORE_ERROR("  VS error: {}", vsRes.GetErrorMessage());
            if (!fsRes.IsSuccess())
                VX_CORE_ERROR("  FS error: {}", fsRes.GetErrorMessage());

            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_Assets.find(id);
            if (it != m_Assets.end())
            {
                auto* shaderAsset = dynamic_cast<ShaderAsset*>(it->second.assetPtr.get());
                if (!isReload && shaderAsset && m_FallbackShader && m_FallbackShader->IsValid())
                {
                    shaderAsset->SetShader(m_FallbackShader);
                    shaderAsset->SetReflection({});
                    shaderAsset->SetIsFallback(true);
                    shaderAsset->SetState(AssetState::Loaded);
                    shaderAsset->SetProgress(1.0f);
                    VX_CORE_WARN("AssetSystem: Using fallback shader for '{}'", name);
                }
                else if (!isReload)
                {
                    it->second.assetPtr->SetState(AssetState::Failed);
                    it->second.assetPtr->SetProgress(1.0f);
                }
                // On reload failure, keep existing shader and state as-is
            }
            m_ShaderReloading.erase(id);
            co_return;
        }

        setProgress(0.80f);

        // Create GPUShader
        auto shader = GPUShader::Create(name);
        std::unordered_map<ShaderStage, std::vector<uint32_t>> stages;
        stages[ShaderStage::Vertex] = vsRes.GetValue().SpirV;
        stages[ShaderStage::Fragment] = fsRes.GetValue().SpirV;

        ShaderReflectionData reflection = ShaderReflection::CombineReflections({ vsRes.GetValue().Reflection, fsRes.GetValue().Reflection });

        auto createRes = shader->Create(stages, reflection);
        if (!createRes.IsSuccess())
        {
            VX_CORE_ERROR("AssetSystem: Failed to create GPU shader '{}'", name);
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_Assets.find(id);
            if (it != m_Assets.end())
            {
                if (!isReload)
                {
                    it->second.assetPtr->SetState(AssetState::Failed);
                    it->second.assetPtr->SetProgress(1.0f);
                }
                // On reload failure, keep previous good shader
            }
            m_ShaderReloading.erase(id);
            co_return;
        }

        setProgress(0.95f);

        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_Assets.find(id);
            if (it != m_Assets.end())
            {
                auto* shaderAsset = dynamic_cast<ShaderAsset*>(it->second.assetPtr.get());
                if (shaderAsset)
                {
                    // Swap-in the newly created program
                    shaderAsset->SetShader(std::shared_ptr<GPUShader>(std::move(shader)));
                    shaderAsset->SetReflection(reflection);
                    shaderAsset->SetState(AssetState::Loaded);
                    shaderAsset->SetProgress(1.0f);

                    // Update tracked last-write times after successful (re)compile
                    auto srcIt = m_ShaderSources.find(id);
                    if (srcIt != m_ShaderSources.end())
                    {
                        std::error_code ec;
                        if (!resolvedVS.empty() && std::filesystem::exists(resolvedVS, ec))
                            srcIt->second.VertexLastWrite = std::filesystem::last_write_time(resolvedVS, ec);
                        if (!resolvedFS.empty() && std::filesystem::exists(resolvedFS, ec))
                            srcIt->second.FragmentLastWrite = std::filesystem::last_write_time(resolvedFS, ec);
                    }
                    // Mark reloading done
                    m_ShaderReloading.erase(id);
                }
            }
        }

        setProgress(1.0f);
        co_return;
    }

    void AssetSystem::EnsureFallbackShader()
    {
        // Very small GLSL passthrough shader compiled via ShaderCompiler
        static const char* kVS = R"(
            #version 450 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aUV;
            layout(location = 2) in vec3 aNormal;
            layout(location = 3) in vec3 aTangent;
            layout(location = 0) uniform mat4 u_ViewProjection; // uses 0..3
            layout(location = 4) uniform mat4 u_Model;          // uses 4..7
            void main(){ gl_Position = u_ViewProjection * u_Model * vec4(aPos,1.0); }
        )";
        static const char* kFS = R"(
            #version 450 core
            layout(location = 0) out vec4 FragColor;
            layout(location = 8) uniform vec4 u_Color;
            void main(){ FragColor = u_Color; }
        )";

        ShaderCompiler compiler;
        ShaderCompileOptions options; options.GenerateDebugInfo = false; options.TargetProfile = "opengl";
        if (auto* renderer = GetRenderer())
        {
            // Map GraphicsContextInfo into options.HardwareCaps for fallback too
            auto* ctx = renderer->GetContext();
            if (ctx)
            {
                const auto& info = ctx->GetInfo();
                options.HardwareCaps.MaxTextureUnits = info.MaxCombinedTextureUnits;
                options.HardwareCaps.MaxSamples = std::max(info.SampleCount, info.MaxSamples);
                options.HardwareCaps.SupportsGeometry = info.SupportsGeometryShaders;
                options.HardwareCaps.SupportsCompute = info.SupportsComputeShaders;
                options.HardwareCaps.SupportsMultiDrawIndirect = info.SupportsMultiDrawIndirect;
                options.HardwareCaps.SRGBFramebufferCapable = info.SRGBFramebufferCapable;
                // Provide commonly useful macros
                options.Macros["VX_SUPPORTS_GEOMETRY"] = info.SupportsGeometryShaders ? "1" : "0";
                options.Macros["VX_SUPPORTS_COMPUTE"] = info.SupportsComputeShaders ? "1" : "0";
                options.Macros["VX_SUPPORTS_MDI"] = info.SupportsMultiDrawIndirect ? "1" : "0";
                options.Macros["VX_MAX_TEX_UNITS"] = std::to_string(info.MaxCombinedTextureUnits);
                options.Macros["VX_SRGB_FB"] = info.SRGBFramebufferCapable ? "1" : "0";
            }
        }
        auto vs = compiler.CompileFromSource(kVS, ShaderStage::Vertex, options, "Fallback.vert");
        auto fs = compiler.CompileFromSource(kFS, ShaderStage::Fragment, options, "Fallback.frag");
        if (!vs.IsSuccess() || !fs.IsSuccess())
        {
            VX_CORE_WARN("AssetSystem: Failed to compile fallback shader. Rendering may fail if assets are missing.");
            return;
        }
        auto shader = GPUShader::Create("FallbackShader");
        std::unordered_map<ShaderStage, std::vector<uint32_t>> stages;
        stages[ShaderStage::Vertex] = vs.GetValue().SpirV;
        stages[ShaderStage::Fragment] = fs.GetValue().SpirV;
        ShaderReflectionData reflection = ShaderReflection::CombineReflections({ vs.GetValue().Reflection, fs.GetValue().Reflection });
        if (shader->Create(stages, reflection).IsSuccess())
        {
            m_FallbackShader = std::shared_ptr<GPUShader>(std::move(shader));
            VX_CORE_INFO("AssetSystem: Fallback shader created");
        }
    }

    void AssetSystem::CheckForShaderHotReloads()
    {
        // Collect candidates without holding the mutex while doing filesystem IO
        std::vector<std::tuple<UUID, std::string, std::string, ShaderCompileOptions>> toReload;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            for (const auto& [id, srcInfo] : m_ShaderSources)
            {
                if (m_ShaderReloading.find(id) != m_ShaderReloading.end())
                    continue;

                std::error_code ec1, ec2;
                std::filesystem::path vp = srcInfo.VertexPath;
                std::filesystem::path fp = srcInfo.FragmentPath;

                // Resolve relative paths; prefer DevAssets root when available to match editing location
                if (vp.is_relative()) vp = (m_DevAssetsAvailable ? (m_DevAssetsRoot / vp) : (m_AssetsRoot / vp));
                if (fp.is_relative()) fp = (m_DevAssetsAvailable ? (m_DevAssetsRoot / fp) : (m_AssetsRoot / fp));

                bool vsChanged = false, fsChanged = false;
                if (!vp.empty() && std::filesystem::exists(vp, ec1))
                {
                    auto cur = std::filesystem::last_write_time(vp, ec1);
                    if (!ec1 && (srcInfo.VertexLastWrite == std::filesystem::file_time_type{} || cur > srcInfo.VertexLastWrite))
                        vsChanged = true;
                }
                if (!fp.empty() && std::filesystem::exists(fp, ec2))
                {
                    auto cur = std::filesystem::last_write_time(fp, ec2);
                    if (!ec2 && (srcInfo.FragmentLastWrite == std::filesystem::file_time_type{} || cur > srcInfo.FragmentLastWrite))
                        fsChanged = true;
                }

                if (vsChanged || fsChanged)
                {
                    // Lookup name for nice logging
                    auto it = m_Assets.find(id);
                    std::string name = (it != m_Assets.end() && it->second.assetPtr) ? it->second.assetPtr->GetName() : std::string("Shader");
                    toReload.emplace_back(id, srcInfo.VertexPath, srcInfo.FragmentPath, srcInfo.Options);
                    m_ShaderReloading.insert(id);
                    VX_CORE_INFO("AssetSystem: Detected shader source change for '%s' (VS changed=%d, FS changed=%d)", name.c_str(), vsChanged ? 1 : 0, fsChanged ? 1 : 0);
                }
            }
        }

        // Schedule recompiles
        for (auto& item : toReload)
        {
            UUID id; std::string vs, fs; ShaderCompileOptions opts;
            std::tie(id, vs, fs, opts) = std::move(item);

            std::string name;
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                auto it = m_Assets.find(id);
                if (it != m_Assets.end() && it->second.assetPtr)
                    name = it->second.assetPtr->GetName();
            }

            auto task = CompileShaderTask(id, name, vs, fs, opts, {}, true);
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_PendingTasks.emplace_back(std::move(task));
        }
    }

    void AssetSystem::ScheduleUnload(const UUID& id)
    {
        // If already scheduled, push the deadline out
        const double deadline = Time::GetUnscaledTime() + m_UnloadGracePeriodSeconds;
        auto it = m_IdToScheduledTime.find(id);
        if (it != m_IdToScheduledTime.end())
        {
            it->second = deadline;
            // Update the deque entry lazily; processing consults the map for truth
            return;
        }
        m_IdToScheduledTime[id] = deadline;
        m_PendingUnloads.push_back(PendingUnload{ id, deadline });
    }

    void AssetSystem::CancelScheduledUnload(const UUID& id)
    {
        auto it = m_IdToScheduledTime.find(id);
        if (it == m_IdToScheduledTime.end()) return;
        m_IdToScheduledTime.erase(it);
        // Do not scan/erase from deque here to keep O(1); Update() will skip stale entries
    }

    bool AssetSystem::CanUnloadNow_NoLock(const UUID& id) const
    {
        // Preconditions: mutex held
        // Must still exist, refcount must be zero, and no loaded asset depends on it with refs
        auto refIt = m_Refs.find(id);
        if (refIt != m_Refs.end() && refIt->second != 0)
            return false;
        auto a = m_Assets.find(id);
        if (a == m_Assets.end())
            return false;

        // Check reverse dependencies by scanning assets (small counts, OK). If scale increases, maintain reverse map.
        for (const auto& [otherId, entry] : m_Assets)
        {
            if (!entry.assetPtr)
                continue;
            if (otherId == id)
                continue;
            // If other asset is loaded (or loading) and has refs, and depends on id, do not unload
            const auto state = entry.assetPtr->GetState();
            if (state != AssetState::Loaded && state != AssetState::Loading)
                continue;
            auto refsIt = m_Refs.find(otherId);
            const uint32_t otherRefs = refsIt == m_Refs.end() ? 0u : refsIt->second;
            if (otherRefs == 0)
                continue;
            for (const auto& dep : entry.assetPtr->GetDependencies())
            {
                if (dep == id)
                    return false;
            }
        }
        return true;
    }

    void AssetSystem::PerformUnload_NoLock(const UUID& id)
    {
        auto it = m_Assets.find(id);
        if (it == m_Assets.end())
            return;

        auto& entry = it->second;
        if (entry.assetPtr)
        {
            if (auto* shaderAsset = dynamic_cast<ShaderAsset*>(entry.assetPtr.get()))
            {
                if (auto& shaderRef = shaderAsset->GetShader())
                {
                    shaderRef->Destroy();
                }
                shaderAsset->SetShader(nullptr);
                shaderAsset->SetReflection({});
                shaderAsset->SetIsFallback(false);
            }
            entry.assetPtr->SetState(AssetState::Unloaded);
            entry.assetPtr->SetProgress(0.0f);
        }
        m_NameToUUID.erase(entry.Name);
        // Stop hot-reload tracking for this shader if present
        m_ShaderSources.erase(id);
        m_ShaderReloading.erase(id);
        // Remove ref entry last
        m_Refs.erase(id);
        m_Assets.erase(it);
        VX_CORE_TRACE("AssetSystem: Unloaded asset '{}' (timed)", entry.Name);
    }
}