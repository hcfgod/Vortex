#include "vxpch.h"
#include "Window.h"
#include "Debug/Log.h"

#ifdef VX_USE_SDL
    #include "Platform/SDL/SDLWindow.h"
#endif
// TODO: Add other platform-specific includes (e.g. GLFW) as needed

namespace Vortex
{
    std::unique_ptr<Window> CreatePlatformWindow(const WindowProperties& props)
    {
        // For now, we only support SDL windows
        // In the future, we can add conditionals here to choose different implementations:
        // - GLFW
        // - Native platform windows (Win32, Cocoa, X11/Wayland)
        #ifdef VX_USE_SDL
            return std::make_unique<SDLWindow>(props);
        #endif

		VX_CORE_ERROR("No supported windowing platform defined!");
		return nullptr; // Unsupported platform
    }
}