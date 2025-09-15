#pragma once

/**
 * @file EntryPointSDL.h
 * @brief SDL3 main callbacks entry point for mobile and console platforms
 * 
 * This entry point uses SDL3's new main callback system instead of traditional main().
 * It's used on platforms like iOS, Android, Nintendo Switch, etc. where the platform
 * owns the main loop and we provide callbacks.
 */

#define SDL_MAIN_USE_CALLBACKS 1

#include "Application.h"
#include "Debug/Log.h"
#include "Engine/Engine.h"
#include "Core/EngineConfig.h"
#include "Core/Bootstrap.h"
#include "Engine/Time/Time.h"
#include "Events/ApplicationEvent.h"
#include "Events/WindowEvent.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include <Engine/Systems/RenderSystem.h>
#include <memory>

extern Vortex::Application* Vortex::CreateApplication();

namespace Vortex
{
    namespace Internal
    {
        static std::unique_ptr<Engine> s_Engine = nullptr;
        static std::unique_ptr<Application> s_Application = nullptr;
    }
}

/**
 * @brief SDL3 App Init callback - called once at startup
 * @param appstate Pointer to store application state
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @return SDL_APP_CONTINUE to continue, SDL_APP_FAILURE to quit with error
 */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    // Initialize early subsystems (configuration and logging)
    std::string configDir = Vortex::Bootstrap::InitializeEarlySubsystems(argc, argv);

    VX_CORE_INFO("Starting Vortex Engine (SDL3 Entry Point)");
   
    // Initialize EngineConfig now that logging exists
    auto ecInit = Vortex::EngineConfig::Get().Initialize(configDir);
    if (ecInit.IsError()) 
    {
        VX_CORE_WARN("EngineConfig initialization reported: {}", ecInit.GetErrorMessage());
    }
    
    try
    {
        // Create and initialize engine
        Vortex::Internal::s_Engine = std::make_unique<Vortex::Engine>();
        if (!Vortex::Internal::s_Engine)
        {
            VX_CORE_CRITICAL("Failed to create engine!");
            return SDL_APP_FAILURE;
        }
        
        auto result = Vortex::Internal::s_Engine->Initialize();
        if (result.IsError())
        {
            VX_CORE_CRITICAL("Engine initialization failed: {0}", result.GetErrorMessage());
            Vortex::Internal::s_Engine.reset();
            return SDL_APP_FAILURE;
        }

        // Create the client application
        Vortex::Internal::s_Application = std::unique_ptr<Vortex::Application>(Vortex::CreateApplication());
        if (!Vortex::Internal::s_Application)
        {
            VX_CORE_CRITICAL("Failed to create application!");
            Vortex::Internal::s_Engine->Shutdown();
            Vortex::Internal::s_Engine.reset();
            return SDL_APP_FAILURE;
        }

        // Initialize the application (without calling Run - SDL3 will manage the loop)
        Vortex::Internal::s_Application->Initialize();
        
        // CRITICAL: Perform the setup steps that Application::Run() normally does
        // This attaches the window to RenderSystem, sets up event subscriptions, etc.
        Vortex::Internal::s_Application->SetupWithEngine(Vortex::Internal::s_Engine.get());
        
        // Dispatch application started event manually (since we're not using Application::Run)
        VX_DISPATCH_EVENT(Vortex::ApplicationStartedEvent());
        
        VX_CORE_INFO("SDL3 App Init completed successfully");
        *appstate = nullptr; // We manage our own state
        return SDL_APP_CONTINUE;
    }
    catch (const std::exception& e)
    {
        VX_CORE_CRITICAL("Exception during SDL3 App Init: {0}", e.what());
        return SDL_APP_FAILURE;
    }
}

/**
 * @brief SDL3 App Iterate callback - called every frame
 * @param appstate Application state pointer from SDL_AppInit
 * @return SDL_APP_CONTINUE to continue, SDL_APP_SUCCESS to quit successfully
 */
SDL_AppResult SDL_AppIterate(void* appstate)
{
    if (!Vortex::Internal::s_Engine || !Vortex::Internal::s_Application)
    {
        return SDL_APP_FAILURE;
    }

    try
    {
        // Hot-reload configuration and apply updates
        if (Vortex::EngineConfig::Get().ReloadChangedConfigs())
        {
            // Apply window changes
            Vortex::EngineConfig::Get().ApplyWindowSettings(Vortex::Internal::s_Application->GetWindow());

            // Apply renderer changes
            if (auto* rs = Vortex::Internal::s_Engine->GetSystemManager().GetSystem<Vortex::RenderSystem>())
            {
                Vortex::EngineConfig::Get().ApplyRenderSettings(rs);
                if (auto* wnd = Vortex::Internal::s_Application->GetWindow())
                {
                    rs->OnWindowResized(static_cast<uint32_t>(wnd->GetWidth()), static_cast<uint32_t>(wnd->GetHeight()));
                }
            }
        }

        // Check if engine is still running
        if (!Vortex::Internal::s_Engine->IsRunning())
        {
            return SDL_APP_SUCCESS;
        }

        // Update engine systems first
        auto updateResult = Vortex::Internal::s_Engine->Update();
        if (updateResult.IsError())
        {
            VX_CORE_ERROR("Engine update failed: {0}", updateResult.GetErrorMessage());
        }

        // Update application
        Vortex::Internal::s_Application->Update();

        // Render engine systems first (Engine dispatches its own events now)
        auto renderResult = Vortex::Internal::s_Engine->Render();
        if (renderResult.IsError())
        {
            VX_CORE_ERROR("Engine render failed: {0}", renderResult.GetErrorMessage());
        }
        
        // Render application
        Vortex::Internal::s_Application->Render();

        return SDL_APP_CONTINUE;
    }
    catch (const std::exception& e)
    {
        VX_CORE_ERROR("Exception during SDL3 App Iterate: {0}", e.what());
        return SDL_APP_FAILURE;
    }
}

/**
 * @brief SDL3 App Event callback - called for each SDL event
 * @param appstate Application state pointer from SDL_AppInit
 * @param event SDL event to process
 * @return SDL_APP_CONTINUE to continue, SDL_APP_SUCCESS to quit successfully
 */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (!Vortex::Internal::s_Application || !event)
    {
        return SDL_APP_CONTINUE;
    }

    try
    {
        // Convert SDL events to Vortex events and dispatch them
        // (Application::Run normally does this in its main loop)
        if (Vortex::Internal::s_Application)
        {
            // Use Application's event conversion logic
            // Note: We need to access ConvertSDLEventToVortexEvent, but it's private
            // For now, let the application handle the raw SDL event
            Vortex::Internal::s_Application->ProcessEvent(*event);
        }
        
        // Handle quit event
        if (event->type == SDL_EVENT_QUIT)
        {
            VX_CORE_INFO("Quit event received");
            if (Vortex::Internal::s_Engine)
            {
                Vortex::Internal::s_Engine->Stop();
            }
            return SDL_APP_SUCCESS;
        }
        
        // Handle window close event
        if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        {
            VX_CORE_INFO("Window close requested");
            VX_DISPATCH_EVENT(Vortex::WindowCloseEvent());
            if (Vortex::Internal::s_Engine)
            {
                Vortex::Internal::s_Engine->Stop();
            }
            return SDL_APP_SUCCESS;
        }

        return SDL_APP_CONTINUE;
    }
    catch (const std::exception& e)
    {
        VX_CORE_ERROR("Exception during SDL3 App Event: {0}", e.what());
        return SDL_APP_FAILURE;
    }
}

/**
 * @brief SDL3 App Quit callback - called when quitting
 * @param appstate Application state pointer from SDL_AppInit
 * @param result The result that caused the quit
 */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    VX_CORE_INFO("SDL3 App Quit - Shutting down Vortex Engine");
    
    try
    {
        // Dispatch application shutdown event (Application::Run normally does this)
        if (Vortex::EventSystem::IsInitialized())
        {
            VX_DISPATCH_EVENT(Vortex::ApplicationShutdownEvent());
        }
        
        // Clean up in reverse order (automatic with smart pointers)
        if (Vortex::Internal::s_Application)
        {
            Vortex::Internal::s_Application->Shutdown();
            Vortex::Internal::s_Application.reset();
        }

        // Application/Engine lifetimes: Application destructor will shutdown the Engine; avoid double shutdown
        if (Vortex::Internal::s_Engine)
        {
            Vortex::Internal::s_Engine.reset();
        }
        
        VX_CORE_INFO("SDL3 App Quit completed");
        
        // Shutdown logging system last
        Vortex::Log::Shutdown();
    }
    catch (const std::exception& e)
    {
        VX_CORE_ERROR("Exception during SDL3 App Quit: {0}", e.what());
    }
}