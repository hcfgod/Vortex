#pragma once

#include "SDL3/SDL.h"

namespace Vortex
{
    /**
     * @brief Manages SDL3 initialization, shutdown, and global state
     * 
     * This class provides a safe, RAII-style wrapper around SDL's global state.
     * It handles initialization hints, subsystem management, and cleanup.
     */
    class SDL3Manager
    {
    public:
        /**
         * @brief Initialize SDL3 with specified subsystems
         * @param flags SDL initialization flags (e.g., SDL_INIT_VIDEO)
         * @return True if initialization succeeded, false otherwise
         */
        static bool Initialize(SDL_InitFlags flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        
        /**
         * @brief Shutdown SDL3 and cleanup resources
         */
        static void Shutdown();
        
        /**
         * @brief Check if SDL3 is currently initialized
         * @return True if initialized, false otherwise
         */
        static bool IsInitialized() { return s_Initialized; }
        
        /**
         * @brief Get the currently initialized subsystems
         * @return Bitmask of initialized subsystems
         */
        static SDL_InitFlags GetInitializedSubsystems() { return s_InitializedSubsystems; }
        
        /**
         * @brief Initialize additional SDL subsystems
         * @param flags Additional subsystem flags to initialize
         * @return True if successful, false otherwise
         */
        static bool InitializeSubsystem(SDL_InitFlags flags);
        
        /**
         * @brief Shutdown specific SDL subsystems
         * @param flags Subsystem flags to shutdown
         */
        static void ShutdownSubsystem(SDL_InitFlags flags);
        
        /**
         * @brief Set SDL hints for optimal performance and compatibility
         */
        static void SetOptimalHints();
        
        /**
         * @brief Get SDL version information
         * @return SDL version as a formatted string
         */
        static std::string GetSDLVersion();
        
    private:
        SDL3Manager() = delete;
        ~SDL3Manager() = delete;
        SDL3Manager(const SDL3Manager&) = delete;
        SDL3Manager& operator=(const SDL3Manager&) = delete;
        
        static bool s_Initialized;
        static SDL_InitFlags s_InitializedSubsystems;
    };
}
