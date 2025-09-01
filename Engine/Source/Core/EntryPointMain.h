#pragma once

/**
 * @file EntryPointMain.h
 * @brief Traditional main() entry point for desktop platforms
 * 
 * This entry point uses the traditional main() function for desktop platforms
 * like Windows, Linux, and macOS where we own the main loop.
 */

#include "Application.h"
#include "Debug/Log.h"
#include "Engine/Engine.h"
#include "Core/EngineConfig.h"
#include "Core/Bootstrap.h"

extern Vortex::Application* Vortex::CreateApplication();

/**
 * @brief Traditional main entry point for desktop platforms
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @return Exit code (0 for success, non-zero for error)
 */
int main(int argc, char** argv)
{
    try
    {
        // Initialize early subsystems (configuration and logging)
        std::string configDir = Vortex::Bootstrap::InitializeEarlySubsystems(argc, argv);

        VX_CORE_INFO("Starting Vortex Engine (Desktop Entry Point)");

        // Initialize EngineConfig proper (now that logging exists), using the same directory
        auto ecInit = Vortex::EngineConfig::Get().Initialize(configDir);
        if (ecInit.IsError()) 
        {
            VX_CORE_WARN("EngineConfig initialization reported: {}", ecInit.GetErrorMessage());
        }

        // Create and initialize engine
        auto engine = new Vortex::Engine();
        if (!engine)
        {
            VX_CORE_CRITICAL("Failed to create engine!");
            return 1;
        }
        
        auto result = engine->Initialize();
        if (result.IsError())
        {
            VX_CORE_CRITICAL("Engine initialization failed: {0}", result.GetErrorMessage());
            delete engine;
            return 1;
        } 

        // Create the client application (engine is ready)
        auto app = Vortex::CreateApplication();
        if (!app)
        {
            VX_CORE_CRITICAL("Failed to create application!");
            engine->Shutdown();
            delete engine;
            return 1;
        }

        // Run the application (handles main loop internally)
        app->Run(engine);

        // Clean up in reverse order
        delete app;

        engine->Shutdown();
        delete engine;

        VX_CORE_INFO("Vortex Engine shutdown complete");
        
        // Shutdown logging system last
        Vortex::Log::Shutdown();
        return 0;
    }
    catch (const std::exception& e)
    {
        VX_CORE_CRITICAL("Unhandled exception in main: {0}", e.what());
        return 1;
    }
    catch (...)
    {
        VX_CORE_CRITICAL("Unknown unhandled exception in main");
        return 1;
    }
}