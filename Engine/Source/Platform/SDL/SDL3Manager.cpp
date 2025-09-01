#include "vxpch.h"
#include "SDL3Manager.h"
#include "Core/Debug/Log.h"
#include "SDL3/SDL_hints.h"

namespace Vortex
{
    bool SDL3Manager::s_Initialized = false;
    SDL_InitFlags SDL3Manager::s_InitializedSubsystems = 0;

    bool SDL3Manager::Initialize(SDL_InitFlags flags)
    {
        if (s_Initialized)
        {
            VX_CORE_WARN("SDL3Manager already initialized");
            return true;
        }

        VX_CORE_INFO("Initializing SDL3...");

        // Set optimal hints before initialization
        SetOptimalHints();

        // Initialize SDL3
        if (!SDL_Init(flags))
        {
            VX_CORE_ERROR("Failed to initialize SDL3: {0}", SDL_GetError());
            return false;
        }

        s_Initialized = true;
        s_InitializedSubsystems = flags;

        VX_CORE_INFO("SDL3 initialized successfully");
        VX_CORE_INFO("SDL Version: {0}", GetSDLVersion());

        return true;
    }

    void SDL3Manager::Shutdown()
    {
        if (!s_Initialized)
        {
            VX_CORE_WARN("SDL3Manager not initialized, nothing to shutdown");
            return;
        }

        VX_CORE_INFO("Shutting down SDL3...");

        // Proactively shut down initialized subsystems first
        if (s_InitializedSubsystems != 0)
        {
            SDL_QuitSubSystem(s_InitializedSubsystems);
            s_InitializedSubsystems = 0;
        }

        // Final cleanup
        SDL_Quit();
        s_Initialized = false;

        VX_CORE_INFO("SDL3 shutdown complete");
    }

    bool SDL3Manager::InitializeSubsystem(SDL_InitFlags flags)
    {
        if (!s_Initialized)
        {
            VX_CORE_ERROR("Cannot initialize subsystem: SDL3Manager not initialized");
            return false;
        }

        if (!SDL_InitSubSystem(flags))
        {
            VX_CORE_ERROR("Failed to initialize SDL subsystem {0}: {1}", static_cast<uint32_t>(flags), SDL_GetError());
            return false;
        }

        s_InitializedSubsystems |= flags;
        VX_CORE_INFO("SDL subsystem {0} initialized", static_cast<uint32_t>(flags));
        return true;
    }

    void SDL3Manager::ShutdownSubsystem(SDL_InitFlags flags)
    {
        if (!s_Initialized)
        {
            VX_CORE_WARN("Cannot shutdown subsystem: SDL3Manager not initialized");
            return;
        }

        SDL_QuitSubSystem(flags);
        s_InitializedSubsystems &= ~flags;
        VX_CORE_INFO("SDL subsystem {0} shutdown", static_cast<uint32_t>(flags));
    }

    void SDL3Manager::SetOptimalHints()
    {
        VX_CORE_INFO("Setting SDL3 hints for optimal performance and compatibility...");

        // Disable Windows GameInput to avoid crashes on some systems
        SDL_SetHint(SDL_HINT_WINDOWS_GAMEINPUT, "0");

        // Mouse and keyboard optimizations
        SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

        VX_CORE_INFO("SDL3 hints configured");
    }

    std::string SDL3Manager::GetSDLVersion()
    {
        int version = SDL_GetVersion();
        int major = SDL_VERSIONNUM_MAJOR(version);
        int minor = SDL_VERSIONNUM_MINOR(version);
        int patch = SDL_VERSIONNUM_MICRO(version);
        
        return fmt::format("{}.{}.{}", major, minor, patch);
    }
}
