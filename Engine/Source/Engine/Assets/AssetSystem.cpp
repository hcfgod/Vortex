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

        // Try to detect repository-level Assets directory for development hot reload
        m_DevAssetsAvailable = false;
        m_DevAssetsRoot.clear();
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

        // Defer fallback shader creation until a graphics context exists (after RenderSystem initializes)
        m_FallbackInitialized = false;

        m_Initialized = true;
        VX_CORE_INFO("AssetSystem initialized. AssetsRoot: {}", m_AssetsRoot.string());
        MarkInitialized();
        return Result<void>();
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

        static const char* exts[] = { "png", "jpg", "jpeg", "bmp", "tga", "ktx", "dds" };
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

    AssetHandle<ShaderAsset> AssetSystem::LoadShaderAsync(const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath,
        const ShaderCompileOptions& options,
        ProgressCallback onProgress)
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

    AssetHandle<ShaderAsset> AssetSystem::LoadShaderFromManifestAsync(const std::string& manifestPath,
        const ShaderCompileOptions& defaultOptions,
        ProgressCallback onProgress)
    {
        // Read JSON manifest
        using json = nlohmann::json;
        std::filesystem::path path = manifestPath;
        if (path.is_relative())
            path = m_AssetsRoot / path;

        std::ifstream f(path);
        if (!f.is_open())
        {
            VX_CORE_ERROR("AssetSystem: Shader manifest not found: {}", path.string());
            return {};
        }
        json j; f >> j;

        ShaderManifest manifest;
        manifest.Name = j.value("name", path.stem().string());
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

        // Resolve paths
        std::string vert = (m_AssetsRoot / manifest.VertexPath).string();
        std::string frag = (m_AssetsRoot / manifest.FragmentPath).string();
        return LoadShaderAsync(manifest.Name, vert, frag, options, std::move(onProgress));
    }

    AssetHandle<TextureAsset> AssetSystem::LoadTextureAsync(const std::string& name,
        const std::string& filePath,
        ProgressCallback onProgress)
    {
        // Create placeholder asset
        auto texAsset = std::make_shared<TextureAsset>(name);
        texAsset->SetState(AssetState::Loading);
        texAsset->SetProgress(0.0f);
        UUID id = RegisterAsset(texAsset);

        // Load from disk using stb_image if available; fallback to procedural checkerboard
        Task<void> task = [this, id, name, filePath, onProgress]() -> Task<void>
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

            // Attempt to load using stb_image
            stbi_set_flip_vertically_on_load(1);
            int w = 0, h = 0, comp = 0;
            unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &comp, 4);
            if (data && w > 0 && h > 0)
            {
                width = static_cast<uint32_t>(w);
                height = static_cast<uint32_t>(h);
                pixels.assign(data, data + (width * height * 4));
                stbi_image_free(data);
            }
            else
            {
                // Fallback checkerboard
                width = 256; height = 256;
                pixels.resize(width * height * 4);
                for (uint32_t y = 0; y < height; ++y)
                {
                    for (uint32_t x = 0; x < width; ++x)
                    {
                        bool check = (((x / 32) + (y / 32)) % 2) == 0;
                        uint8_t r = check ? 255 : 30;
                        uint8_t g = check ? 255 : 30;
                        uint8_t b = check ? 255 : 30;
                        uint8_t a = 255;
                        size_t idx = static_cast<size_t>(y) * width * 4ull + static_cast<size_t>(x) * 4ull;
                        pixels[idx + 0] = r;
                        pixels[idx + 1] = g;
                        pixels[idx + 2] = b;
                        pixels[idx + 3] = a;
                    }
                }
            }

            setProgress(0.5f);

            // Create GPU texture
            Texture2D::CreateInfo ci{};
            ci.Width = width;
            ci.Height = height;
            ci.Format = TextureFormat::RGBA8;
            ci.MinFilter = TextureFilter::Linear;
            ci.MagFilter = TextureFilter::Linear;
            ci.WrapS = TextureWrap::Repeat;
            ci.WrapT = TextureWrap::Repeat;
            ci.GenerateMips = true;
            ci.InitialData = pixels.data();
            ci.InitialDataSize = static_cast<uint64_t>(pixels.size());

            auto texture = Texture2D::Create(ci);

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


    Result<void> AssetSystem::BuildShader(const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath,
        const std::filesystem::path& outputDir)
    {
        // Basic stub: compile and write SPIR-V blobs to outputDir for packaging
        ShaderCompiler compiler;
        ShaderCompileOptions options; // default or load from config
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

    Task<void> AssetSystem::CompileShaderTask(UUID id,
        std::string name,
        std::string vertexPath,
        std::string fragmentPath,
        ShaderCompileOptions options,
        ProgressCallback progress,
        bool isReload)
    {
        // Small staged progress model: 0.0-0.1 read, 0.1-0.8 compile, 0.8-1.0 create
        auto setProgress = [&](float p) {
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
        // Keep caching enabled so reloads update on-disk cache with fresh SPIR-V
        compiler.SetCachingEnabled(true, (m_AssetsRoot / "Cache/Shaders").string());

        setProgress(0.10f);

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

        // Compile asynchronously (run concurrently)
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