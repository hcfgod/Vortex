#include "vxpch.h"
#include "Core/EngineConfig.h"
#include "Core/Debug/Log.h"
#include "Core/FileSystem.h"
#include "Engine/Systems/RenderSystem.h"

namespace Vortex
{
    std::string EngineConfig::FindConfigDirectory()
    {
        // Use the FileSystem utility to find the config directory
        std::string configPath = FileSystem::FindConfigDirectory("Config/Engine");
        
        // Log the result
        if (configPath != "Config/Engine") 
        {
            VX_CORE_INFO("Found config directory: {0}", configPath);
        } 
        else 
        {
            VX_CORE_WARN("Could not find Config/Engine directory, using relative path");
            if (auto exeDir = FileSystem::GetExecutableDirectory()) 
            {
                VX_CORE_WARN("Executable directory: {0}", exeDir.value().string());
            }
        }
        
        return configPath;
    }

    EngineConfig& EngineConfig::Get()
    {
        static EngineConfig s_Instance;
        return s_Instance;
    }

    Result<void> EngineConfig::Initialize(const std::string& configDirectory)
    {
        if (m_Initialized)
        {
            VX_CORE_WARN("EngineConfig::Initialize called when already initialized");
            return Result<void>();
        }

        m_ConfigDirectory = configDirectory;
        m_UserPrefsPath = std::filesystem::path(configDirectory) / "UserPreferences.json";

        auto& config = Configuration::Get();
        std::string error;

        // Load layers in priority order (lower priority first)
        
        // 1. Engine defaults (priority 100)
        auto defaultsPath = std::filesystem::path(configDirectory) / "EngineDefaults.json";
        if (!config.LoadLayerFromFile(defaultsPath, "EngineDefaults", 100, &error, true))
        {
            VX_CORE_ERROR("Failed to load engine defaults: {0}", error);
            return Result<void>(ErrorCode::ConfigurationError, "Failed to load EngineDefaults.json: " + error);
        }

        // 2. Engine overrides (priority 200) - optional
        auto enginePath = std::filesystem::path(configDirectory) / "Engine.json";
        auto devPath = std::filesystem::path(configDirectory) / "Development.json"; // backward compatibility
        if (std::filesystem::exists(enginePath) || std::filesystem::exists(devPath))
        {
            const auto& pathToUse = std::filesystem::exists(enginePath) ? enginePath : devPath;
            const char* layerName = std::filesystem::exists(enginePath) ? "Engine" : "Development";
            if (!config.LoadLayerFromFile(pathToUse, layerName, 200, &error, true))
            {
                VX_CORE_WARN("Failed to load engine config overrides: {0}", error);
                // Non-fatal, continue
            }
            else
            {
                VX_CORE_INFO("Loaded engine configuration overrides");
            }
        }

        // 3. User preferences (priority 1000) - optional, will be created if doesn't exist
        if (std::filesystem::exists(m_UserPrefsPath))
        {
            if (!config.LoadLayerFromFile(m_UserPrefsPath, "UserPreferences", 1000, &error, true))
            {
                VX_CORE_WARN("Failed to load user preferences: {0}", error);
                // Non-fatal, continue
            }
            else
            {
                VX_CORE_INFO("Loaded user preferences");
            }
        }
        else
        {
            // Create empty user preferences layer
            config.AddOrReplaceLayer("UserPreferences", 1000);
            VX_CORE_INFO("Created empty user preferences layer");
        }

        m_Initialized = true;
        VX_CORE_INFO("Engine configuration initialized successfully");
        VX_CORE_INFO("  - Engine: v{0}", GetEngineVersion());
        VX_CORE_INFO("  - Window: {0} ({1}x{2})", GetWindowTitle(), GetWindowWidth(), GetWindowHeight());
        VX_CORE_INFO("  - Renderer: {0}, VSync: {1}", GetRendererAPI(), GetVSyncMode());

        return Result<void>();
    }

    void EngineConfig::Shutdown()
    {
        if (!m_Initialized)
            return;

        // Save user preferences before shutdown
        auto result = SaveUserPreferences();
        if (!result)
        {
            VX_CORE_WARN("Failed to save user preferences on shutdown: {0}", result.GetErrorMessage());
        }

        Configuration::Get().Clear();
        m_Initialized = false;
        VX_CORE_INFO("Engine configuration shut down");
    }

    Result<void> EngineConfig::SaveUserPreferences()
    {
        if (!m_Initialized)
            return Result<void>(ErrorCode::InvalidState, "EngineConfig not initialized");

        std::string error;
        auto& config = Configuration::Get();
        
        if (!config.SaveLayerToFile(m_UserPrefsPath, "UserPreferences", &error))
        {
            VX_CORE_ERROR("Failed to save user preferences: {0}", error);
            return Result<void>(ErrorCode::ConfigurationError, "Failed to save user preferences: " + error);
        }

        VX_CORE_TRACE("Saved user preferences to {0}", m_UserPrefsPath.string());
        return Result<void>();
    }

    bool EngineConfig::ReloadChangedConfigs()
    {
        if (!m_Initialized)
            return false;

        std::vector<std::string> reloadedLayers;
        bool changed = Configuration::Get().ReloadChangedFiles(&reloadedLayers);
        
        if (changed)
        {
            VX_CORE_INFO("Reloaded {0} configuration layer(s)", reloadedLayers.size());
        }
        
        return changed;
    }

    void EngineConfig::ApplyWindowSettings(Window* window)
    {
        if (!window)
            return;

        // Title
        const std::string newTitle = GetWindowTitle();
        if (window->GetTitle() != newTitle)
            window->SetTitle(newTitle);

        // Size (only if changed and not fullscreen)
        if (!GetWindowFullscreen())
        {
            int curW = window->GetWidth();
            int curH = window->GetHeight();
            const int cfgW = GetWindowWidth();
            const int cfgH = GetWindowHeight();
            if (cfgW > 0 && cfgH > 0 && (cfgW != curW || cfgH != curH))
                window->SetSize(cfgW, cfgH);
        }

        // Fullscreen
        const bool cfgFullscreen = GetWindowFullscreen();
        if (window->IsFullscreen() != cfgFullscreen)
            window->SetFullscreen(cfgFullscreen);
    }

    void EngineConfig::ApplyRenderSettings(RenderSystem* renderSystem)
    {
        if (!renderSystem)
            return;

        auto settings = renderSystem->GetRenderSettings();

        // VSync
        settings.VSync = GetVSyncModeEnum();

        // Clear color
        const auto cc = GetClearColor();
        settings.ClearColor = glm::vec4(cc.r, cc.g, cc.b, cc.a);

        renderSystem->SetRenderSettings(settings);
    }

    WindowProperties EngineConfig::CreateWindowProperties() const
    {
        WindowProperties props;
        
        // Use defaults if config system isn't initialized
        if (!m_Initialized)
        {
            VX_CORE_WARN("EngineConfig not initialized, using default window properties");
            props.Title = "Vortex Application";
            props.Width = 1280;
            props.Height = 720;
            props.Fullscreen = false;
            props.Resizable = true;
            props.HighDPI = true;
            return props;
        }
        
        // Load from configuration
        props.Title = GetWindowTitle();
        props.Width = GetWindowWidth();
        props.Height = GetWindowHeight();
        props.Fullscreen = GetWindowFullscreen();
        props.Resizable = GetWindowResizable();
        props.HighDPI = true; // Default for this property not in config yet
        return props;
    }

    VSyncMode EngineConfig::GetVSyncModeEnum() const
    {
        const std::string mode = GetVSyncMode();
        
        if (mode == "Disabled") return VSyncMode::Disabled;
        else if (mode == "Enabled") return VSyncMode::Enabled;
        else if (mode == "Adaptive") return VSyncMode::Adaptive;
        else if (mode == "Fast") return VSyncMode::Fast;
        else if (mode == "Mailbox") return VSyncMode::Mailbox;
        
        return VSyncMode::Enabled; // default
    }

    // Engine Settings
    std::string EngineConfig::GetEngineName() const
    {
        // Use window title as the application/engine name
        return GetWindowTitle();
    }

    std::string EngineConfig::GetEngineVersion() const
    {
        return GetConfigValue("Engine.Version", std::string("1.0.0"));
    }

    std::string EngineConfig::GetLogLevel() const
    {
        return GetConfigValue("Engine.LogLevel", std::string("Info"));
    }

    bool EngineConfig::GetEnableAsserts() const
    {
        return GetConfigValue("Engine.EnableAsserts", true);
    }

    int EngineConfig::GetMaxFrameRate() const
    {
        return GetConfigValue("Engine.MaxFrameRate", 0);
    }

    void EngineConfig::SetLogLevel(const std::string& level, bool saveToUserPrefs)
    {
        SetConfigValue("Engine.LogLevel", level, saveToUserPrefs);
    }

    void EngineConfig::SetMaxFrameRate(int fps, bool saveToUserPrefs)
    {
        SetConfigValue("Engine.MaxFrameRate", fps, saveToUserPrefs);
    }

    // Window Settings
    std::string EngineConfig::GetWindowTitle() const
    {
        return GetConfigValue("Window.Title", std::string("Vortex Application"));
    }

    int EngineConfig::GetWindowWidth() const
    {
        return GetConfigValue("Window.Width", 1280);
    }

    int EngineConfig::GetWindowHeight() const
    {
        return GetConfigValue("Window.Height", 720);
    }

    bool EngineConfig::GetWindowFullscreen() const
    {
        return GetConfigValue("Window.Fullscreen", false);
    }

    bool EngineConfig::GetWindowResizable() const
    {
        return GetConfigValue("Window.Resizable", true);
    }

    bool EngineConfig::GetWindowMaximized() const
    {
        return GetConfigValue("Window.Maximized", false);
    }

    bool EngineConfig::GetWindowCentered() const
    {
        return GetConfigValue("Window.Centered", true);
    }

    int EngineConfig::GetWindowMinWidth() const
    {
        return GetConfigValue("Window.MinWidth", 640);
    }

    int EngineConfig::GetWindowMinHeight() const
    {
        return GetConfigValue("Window.MinHeight", 480);
    }

    bool EngineConfig::GetWindowAlwaysOnTop() const
    {
        return GetConfigValue("Window.AlwaysOnTop", false);
    }

    bool EngineConfig::GetWindowDecorated() const
    {
        return GetConfigValue("Window.Decorated", true);
    }

    bool EngineConfig::GetWindowStartHidden() const
    {
        return GetConfigValue("Window.StartHidden", false);
    }

    void EngineConfig::SetWindowTitle(const std::string& title, bool saveToUserPrefs)
    {
        SetConfigValue("Window.Title", title, saveToUserPrefs);
    }

    void EngineConfig::SetWindowSize(int width, int height, bool saveToUserPrefs)
    {
        SetConfigValue("Window.Width", width, saveToUserPrefs);
        SetConfigValue("Window.Height", height, saveToUserPrefs);
    }

    void EngineConfig::SetWindowFullscreen(bool fullscreen, bool saveToUserPrefs)
    {
        SetConfigValue("Window.Fullscreen", fullscreen, saveToUserPrefs);
    }

    // Renderer Settings
    std::string EngineConfig::GetRendererAPI() const
    {
        return GetConfigValue("Renderer.API", std::string("OpenGL"));
    }

    std::string EngineConfig::GetVSyncMode() const
    {
        return GetConfigValue("Renderer.VSync", std::string("Enabled"));
    }

    int EngineConfig::GetMSAASamples() const
    {
        return GetConfigValue("Renderer.MSAASamples", 4);
    }

    int EngineConfig::GetAnisotropicFiltering() const
    {
        return GetConfigValue("Renderer.AnisotropicFiltering", 16);
    }

    std::string EngineConfig::GetTextureQuality() const
    {
        return GetConfigValue("Renderer.TextureQuality", std::string("High"));
    }

    std::string EngineConfig::GetShadowQuality() const
    {
        return GetConfigValue("Renderer.ShadowQuality", std::string("High"));
    }

    bool EngineConfig::GetPostProcessing() const
    {
        return GetConfigValue("Renderer.PostProcessing", true);
    }

    float EngineConfig::GetGamma() const
    {
        return GetConfigValue("Renderer.Gamma", 2.2f);
    }

    EngineConfig::Color EngineConfig::GetClearColor() const
    {
        auto colorJson = Configuration::Get().Get("Renderer.ClearColor");
        return Color::FromJson(colorJson);
    }

    void EngineConfig::SetRendererAPI(const std::string& api, bool saveToUserPrefs)
    {
        SetConfigValue("Renderer.API", api, saveToUserPrefs);
    }

    void EngineConfig::SetVSyncMode(const std::string& mode, bool saveToUserPrefs)
    {
        SetConfigValue("Renderer.VSync", mode, saveToUserPrefs);
    }

    void EngineConfig::SetMSAASamples(int samples, bool saveToUserPrefs)
    {
        SetConfigValue("Renderer.MSAASamples", samples, saveToUserPrefs);
    }

    void EngineConfig::SetClearColor(const Color& color, bool saveToUserPrefs)
    {
        SetConfigValue("Renderer.ClearColor", color, saveToUserPrefs);
    }

    // Performance Settings
    bool EngineConfig::GetEnableProfiling() const
    {
        return GetConfigValue("Performance.EnableProfiling", false);
    }

    bool EngineConfig::GetEnableGPUDebug() const
    {
        return GetConfigValue("Performance.EnableGPUDebug", false);
    }

    size_t EngineConfig::GetMemoryPoolSize() const
    {
        return GetConfigValue("Performance.MemoryPoolSize", static_cast<size_t>(134217728));
    }

    int EngineConfig::GetMaxConcurrentThreads() const
    {
        return GetConfigValue("Performance.MaxConcurrentThreads", 0);
    }

    bool EngineConfig::GetEnableMultithreading() const
    {
        return GetConfigValue("Performance.EnableMultithreading", true);
    }

    void EngineConfig::SetEnableProfiling(bool enable, bool saveToUserPrefs)
    {
        SetConfigValue("Performance.EnableProfiling", enable, saveToUserPrefs);
    }

    void EngineConfig::SetEnableGPUDebug(bool enable, bool saveToUserPrefs)
    {
        SetConfigValue("Performance.EnableGPUDebug", enable, saveToUserPrefs);
    }
}