#include "vxpch.h"
#include "AssetSystem.h"
#include "Core/FileSystem.h"
#include "Core/EngineConfig.h"
#include "Engine/Time/Time.h"
#include "Engine/Renderer/Shader/ShaderReflection.h"
#include "Engine/Renderer/RendererAPI.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace Vortex
{
    AssetSystem::AssetSystem()
        : EngineSystem("AssetSystem", SystemPriority::Core)
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

        // Clean up finished tasks and process pending unloads
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_PendingTasks.begin();
        while (it != m_PendingTasks.end())
        {
            if (it->IsCompleted())
                it = m_PendingTasks.erase(it);
            else
                ++it;
        }

        // Process delayed unloads respecting grace period and dependencies
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
        auto task = CompileShaderTask(id, name, vertexPath, fragmentPath, options, std::move(onProgress));
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_PendingTasks.emplace_back(std::move(task));
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
        ProgressCallback progress)
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
                    resolvedVS = (m_AssetsRoot / vp).string();
                }
            }
            if (!fragmentPath.empty())
            {
                fs::path fp(fragmentPath);
                if (fp.is_relative())
                {
                    fp = strip_assets_prefix(fp);
                    resolvedFS = (m_AssetsRoot / fp).string();
                }
            }
        }
        catch (const std::exception& e)
        {
            VX_CORE_WARN("AssetSystem: Exception while resolving shader paths: {}", e.what());
        }

        VX_CORE_INFO("AssetSystem: Compiling shader '{}'\n  VS: {}\n  FS: {}", name, resolvedVS, resolvedFS);

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
                if (shaderAsset && m_FallbackShader && m_FallbackShader->IsValid())
                {
                    shaderAsset->SetShader(m_FallbackShader);
                    shaderAsset->SetReflection({});
                    shaderAsset->SetIsFallback(true);
                    shaderAsset->SetState(AssetState::Loaded);
                    shaderAsset->SetProgress(1.0f);
                    VX_CORE_WARN("AssetSystem: Using fallback shader for '{}'", name);
                }
                else
                {
                    it->second.assetPtr->SetState(AssetState::Failed);
                    it->second.assetPtr->SetProgress(1.0f);
                }
            }
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
                it->second.assetPtr->SetState(AssetState::Failed);
                it->second.assetPtr->SetProgress(1.0f);
            }
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
                    shaderAsset->SetShader(std::shared_ptr<GPUShader>(std::move(shader)));
                    shaderAsset->SetReflection(reflection);
                    shaderAsset->SetState(AssetState::Loaded);
                    shaderAsset->SetProgress(1.0f);
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
        // Remove ref entry last
        m_Refs.erase(id);
        m_Assets.erase(it);
        VX_CORE_TRACE("AssetSystem: Unloaded asset '{}' (timed)", entry.Name);
    }
}