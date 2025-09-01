#pragma once

/**
 * @file EntryPoint.h
 * @brief Platform-specific entry point routing
 * 
 * This file routes to the appropriate entry point based on the target platform:
 * - Windows/Linux/macOS: Traditional main() function
 * - iOS/Android/Console platforms: SDL3 main callbacks
 */

#include "SDL3/SDL_platform_defines.h"

// Determine which entry point to use based on platform
#if defined(VX_PLATFORM_WINDOWS) || defined(SDL_PLATFORM_LINUX) || defined(SDL_PLATFORM_MACOS)
    // Desktop platforms use traditional main()
    #include "EntryPointMain.h"
#else
    // Mobile and console platforms use SDL3 main callbacks
    #include "EntryPointSDL.h"
#endif