#include "vxpch.h"
#include "AssetSystem.h"
#include "Core/FileSystem.h"
#include "Core/EngineConfig.h"
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

        // Clean up finished tasks
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_PendingTasks.begin();
        while (it != m_PendingTasks.end())
        {
            if (it->IsCompleted())
                it = m_PendingTasks.erase(it);
            else
                ++it;
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
        if (it != m_Assets.end() && it->second.Asset)
            it->second.Asset->AddRef();
    }

    void AssetSystem::Release(const UUID& id)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Refs.find(id);
        if (it == m_Refs.end()) return;
        if (it->second > 0)
            it->second -= 1;
        auto a = m_Assets.find(id);
        if (a != m_Assets.end() && a->second.Asset)
            a->second.Asset->ReleaseRef();
        if (it->second == 0)
        {
            // TODO: Optionally schedule unload after delay taking dependencies into account
        }
    }

    bool AssetSystem::IsLoaded(const UUID& id) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Assets.find(id);
        if (it == m_Assets.end()) return false;
        return it->second.Asset->GetState() == AssetState::Loaded;
    }

    float AssetSystem::GetProgress(const UUID& id) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Assets.find(id);
        if (it == m_Assets.end()) return 0.0f;
        return it->second.Asset->GetProgress();
    }

    UUID AssetSystem::RegisterAsset(const std::shared_ptr<Asset>& asset)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        UUID id = asset->GetId();
        AssetEntry entry; entry.Asset = asset; entry.Name = asset->GetName();
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
        ov.write(reinterpret_cast<const char*>(vs.GetValue().SpirV.data()), vs.GetValue().SpirV.size()*sizeof(uint32_t));
        std::ofstream of(fout, std::ios::binary);
        of.write(reinterpret_cast<const char*>(fs.GetValue().SpirV.data()), fs.GetValue().SpirV.size()*sizeof(uint32_t));
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
                    it->second.Asset->SetProgress(p);
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
                auto* shaderAsset = dynamic_cast<ShaderAsset*>(it->second.Asset.get());
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
                    it->second.Asset->SetState(AssetState::Failed);
                    it->second.Asset->SetProgress(1.0f);
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

        ShaderReflectionData reflection = ShaderReflection::CombineReflections({vsRes.GetValue().Reflection, fsRes.GetValue().Reflection});

        auto createRes = shader->Create(stages, reflection);
        if (!createRes.IsSuccess())
        {
            VX_CORE_ERROR("AssetSystem: Failed to create GPU shader '{}'", name);
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_Assets.find(id);
            if (it != m_Assets.end())
            {
                it->second.Asset->SetState(AssetState::Failed);
                it->second.Asset->SetProgress(1.0f);
            }
            co_return;
        }

        setProgress(0.95f);

        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_Assets.find(id);
            if (it != m_Assets.end())
            {
                auto* shaderAsset = dynamic_cast<ShaderAsset*>(it->second.Asset.get());
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
        ShaderReflectionData reflection = ShaderReflection::CombineReflections({vs.GetValue().Reflection, fs.GetValue().Reflection});
        if (shader->Create(stages, reflection).IsSuccess())
        {
            m_FallbackShader = std::shared_ptr<GPUShader>(std::move(shader));
            VX_CORE_INFO("AssetSystem: Fallback shader created");
        }
    }
}