#pragma once

#include "vxpch.h"
#include "Core/Configuration.h"
#include "Core/Window.h"
#include "Engine/Renderer/GraphicsContext.h"

namespace Vortex
{
    class RenderSystem; // forward declaration to avoid heavy include
    // Type-safe wrapper around the Configuration system for engine-specific settings
    class EngineConfig
    {
    public:
        // Color structure for config values
        struct Color
        {
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
            
            Color() = default;
            Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
            
            // Convert from JSON
            static Color FromJson(const Configuration::Json& json)
            {
                if (json.is_object())
                {
                    return Color{
                        json.value("r", 0.0f),
                        json.value("g", 0.0f),
                        json.value("b", 0.0f),
                        json.value("a", 1.0f)
                    };
                }
                return Color{};
            }
            
            // Convert to JSON
            Configuration::Json ToJson() const
            {
                return Configuration::Json{
                    {"r", r}, {"g", g}, {"b", b}, {"a", a}
                };
            }
        };

    public:
        static EngineConfig& Get();
        
        // Find the config directory automatically using SDL
        static std::string FindConfigDirectory();
        
        // Initialize the configuration system
        Result<void> Initialize(const std::string& configDirectory = "Config/Engine");
        void Shutdown();
        
        // Save user preferences
        Result<void> SaveUserPreferences();
        
        // Check for config file changes and reload if necessary
        bool ReloadChangedConfigs();

        // Apply current configuration values to live systems (after reload)
        void ApplyWindowSettings(Window* window);
        void ApplyRenderSettings(RenderSystem* renderSystem);

        // Query init state
        bool IsInitialized() const { return m_Initialized; }

        // Engine Settings
        std::string GetEngineName() const;
        std::string GetEngineVersion() const;
        std::string GetLogLevel() const;
        bool GetEnableAsserts() const;
        int GetMaxFrameRate() const;
        
        void SetLogLevel(const std::string& level, bool saveToUserPrefs = true);
        void SetMaxFrameRate(int fps, bool saveToUserPrefs = true);

        // Window Settings
        std::string GetWindowTitle() const;
        int GetWindowWidth() const;
        int GetWindowHeight() const;
        bool GetWindowFullscreen() const;
        bool GetWindowResizable() const;
        bool GetWindowMaximized() const;
        bool GetWindowCentered() const;
        int GetWindowMinWidth() const;
        int GetWindowMinHeight() const;
        bool GetWindowAlwaysOnTop() const;
        bool GetWindowDecorated() const;
        bool GetWindowStartHidden() const;
        
        void SetWindowTitle(const std::string& title, bool saveToUserPrefs = true);
        void SetWindowSize(int width, int height, bool saveToUserPrefs = true);
        void SetWindowFullscreen(bool fullscreen, bool saveToUserPrefs = true);

        // Create WindowProperties from config
        WindowProperties CreateWindowProperties() const;

        // Renderer Settings
        std::string GetRendererAPI() const;
        std::string GetVSyncMode() const;
        int GetMSAASamples() const;
        int GetAnisotropicFiltering() const;
        std::string GetTextureQuality() const;
        std::string GetShadowQuality() const;
        bool GetPostProcessing() const;
        float GetGamma() const;
        Color GetClearColor() const;
        
        void SetRendererAPI(const std::string& api, bool saveToUserPrefs = true);
        void SetVSyncMode(const std::string& mode, bool saveToUserPrefs = true);
        void SetMSAASamples(int samples, bool saveToUserPrefs = true);
        void SetClearColor(const Color& color, bool saveToUserPrefs = true);

        // Convert VSync string to enum
        VSyncMode GetVSyncModeEnum() const;

        // Performance Settings
        bool GetEnableProfiling() const;
        bool GetEnableGPUDebug() const;
        size_t GetMemoryPoolSize() const;
        int GetMaxConcurrentThreads() const;
        bool GetEnableMultithreading() const;
        
        void SetEnableProfiling(bool enable, bool saveToUserPrefs = true);
        void SetEnableGPUDebug(bool enable, bool saveToUserPrefs = true);

        // Direct access to underlying configuration (for advanced usage)
        Configuration& GetConfiguration() { return Configuration::Get(); }

    private:
        EngineConfig() = default;
        
        // Helper to set a value and optionally save to user preferences
        template<typename T>
        void SetConfigValue(const std::string& path, const T& value, bool saveToUserPrefs);
        
        // Helper to get config value with type conversion
        template<typename T>
        T GetConfigValue(const std::string& path, const T& defaultValue) const;

    private:
        bool m_Initialized = false;
        std::string m_ConfigDirectory;
        std::filesystem::path m_UserPrefsPath;
    };

    // Template implementations
    template<typename T>
    void EngineConfig::SetConfigValue(const std::string& path, const T& value, bool saveToUserPrefs)
    {
        auto& config = Configuration::Get();
        
        if (saveToUserPrefs)
        {
            config.Set(path, value, "UserPreferences", true, 1000);
        }
        else
        {
            config.Set(path, value, "RuntimeOverrides", true, 900);
        }
    }

    template<typename T>
    T EngineConfig::GetConfigValue(const std::string& path, const T& defaultValue) const
    {
        return Configuration::Get().GetAs<T>(path, defaultValue);
    }

    // Specialization for Color
    template<>
    inline void EngineConfig::SetConfigValue<EngineConfig::Color>(const std::string& path, const EngineConfig::Color& value, bool saveToUserPrefs)
    {
        auto& config = Configuration::Get();
        auto json = value.ToJson();
        
        if (saveToUserPrefs)
        {
            config.Set(path, json, "UserPreferences", true, 1000);
        }
        else
        {
            config.Set(path, json, "RuntimeOverrides", true, 900);
        }
    }
}