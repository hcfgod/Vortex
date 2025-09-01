#include "vxpch.h"
#include "Bootstrap.h"
#include "Configuration.h"
#include "Debug/Log.h"
#include "FileSystem.h"
#include <filesystem>

namespace Vortex
{
    bool Bootstrap::s_Initialized = false;
    std::string Bootstrap::s_ConfigDirectory;

    std::string Bootstrap::InitializeEarlySubsystems(int argc, char** argv)
    {
        if (s_Initialized)
        {
            return s_ConfigDirectory;
        }

        // 1. Find configuration directory
        s_ConfigDirectory = FileSystem::FindConfigDirectory("Config/Engine");

        // 2. Load configuration layers
        if (!LoadConfigurationLayers(s_ConfigDirectory))
        {
            std::cerr << "[BOOTSTRAP ERROR] Failed to load critical configuration files" << std::endl;
            return s_ConfigDirectory; // Continue with defaults
        }

        // 3. Initialize logging from configuration
        if (!InitializeLoggingFromConfig())
        {
            std::cerr << "[BOOTSTRAP ERROR] Failed to initialize logging system" << std::endl;
            return s_ConfigDirectory; // Continue without logging
        }

        s_Initialized = true;
        return s_ConfigDirectory;
    }

    bool Bootstrap::LoadConfigurationLayers(const std::string& configDir)
    {
        auto& cfg = Configuration::Get();
        std::string err;
        bool success = true;

        // 1. Load engine defaults (critical)
        auto defaultsPath = std::filesystem::path(configDir) / "EngineDefaults.json";
        if (!cfg.LoadLayerFromFile(defaultsPath, "EngineDefaults", 100, &err, true))
        {
            std::cerr << "[CONFIG ERROR] Failed to load EngineDefaults.json: " << err << std::endl;
            success = false;
        }

        // 2. Load engine overrides (optional)
        auto enginePath = std::filesystem::path(configDir) / "Engine.json";
        auto devPath = std::filesystem::path(configDir) / "Development.json";
        
        if (std::filesystem::exists(enginePath))
        {
            if (!cfg.LoadLayerFromFile(enginePath, "Engine", 200, &err, true))
            {
                std::cerr << "[CONFIG WARNING] Failed to load Engine.json: " << err << std::endl;
            }
        }
        else if (std::filesystem::exists(devPath))
        {
            if (!cfg.LoadLayerFromFile(devPath, "Development", 200, &err, true))
            {
                std::cerr << "[CONFIG WARNING] Failed to load Development.json: " << err << std::endl;
            }
        }

        // 3. Load user preferences (optional)
        auto userPrefsPath = std::filesystem::path(configDir) / "UserPreferences.json";
        if (std::filesystem::exists(userPrefsPath))
        {
            if (!cfg.LoadLayerFromFile(userPrefsPath, "UserPreferences", 1000, &err, true))
            {
                std::cerr << "[CONFIG WARNING] Failed to load UserPreferences.json: " << err << std::endl;
            }
        }
        else
        {
            cfg.AddOrReplaceLayer("UserPreferences", 1000);
        }

        return success;
    }

    bool Bootstrap::InitializeLoggingFromConfig()
    {
        auto& cfg = Configuration::Get();

        try
        {
            // Build LogConfig from configuration
            LogConfig logCfg{};
            logCfg.EnableAsync       = cfg.GetAs<bool>("Logging.EnableAsync", true);
            logCfg.AsyncQueueSize    = cfg.GetAs<size_t>("Logging.AsyncQueueSize", 8192);
            logCfg.AsyncThreadCount  = cfg.GetAs<size_t>("Logging.AsyncThreadCount", 1);
            logCfg.EnableConsole     = cfg.GetAs<bool>("Logging.EnableConsole", true);
            logCfg.ConsoleColors     = cfg.GetAs<bool>("Logging.ConsoleColors", true);
            logCfg.EnableFileLogging = cfg.GetAs<bool>("Logging.EnableFileLogging", true);
            logCfg.LogDirectory      = cfg.GetAs<std::string>("Logging.LogDirectory", std::string("logs"));
            logCfg.EnableRotation    = cfg.GetAs<bool>("Logging.EnableRotation", true);
            logCfg.DailyRotation     = cfg.GetAs<bool>("Logging.DailyRotation", false);
            logCfg.MaxFiles          = cfg.GetAs<size_t>("Logging.MaxFiles", 5);
            logCfg.AutoFlush         = cfg.GetAs<bool>("Logging.AutoFlush", false);
            logCfg.MaxFileSize       = cfg.GetAs<size_t>("Logging.MaxFileSizeBytes", 10ull * 1024ull * 1024ull);
            logCfg.LogLevel          = cfg.GetAs<spdlog::level::level_enum>("Logging.Level", spdlog::level::info);

            // Handle MB units
            size_t maxMB = cfg.GetAs<size_t>("Logging.MaxFileSizeMB", 0);
            if (maxMB > 0) logCfg.MaxFileSize = maxMB * 1024ull * 1024ull;
            
            int flushSec = cfg.GetAs<int>("Logging.FlushIntervalSeconds", 3);
            logCfg.FlushInterval = std::chrono::seconds(flushSec);

            // Initialize logging with configured settings
            Log::Init(logCfg);
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[LOGGING ERROR] Failed to initialize logging: " << e.what() << std::endl;
            return false;
        }
    }
}