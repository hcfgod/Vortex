#include "vxpch.h"
#include "GraphicsContext.h"
#include "Core/Debug/Log.h"
#include <algorithm>

#ifdef VX_OPENGL_SUPPORT
    #ifdef VX_USE_SDL
        #include "OpenGL/SDL/SDL_OpenGLGraphicsContext.h"
        #include <SDL3/SDL.h>
    #endif
#endif

namespace Vortex
{
    namespace
    {
        // Global graphics API selection (defaults to OpenGL on desktop if available)
        static GraphicsAPI s_CurrentGraphicsAPI =
        #if defined(VX_OPENGL_SUPPORT)
            GraphicsAPI::OpenGL;
        #else
            GraphicsAPI::None;
        #endif
    }

    // -----------------------------------------------------------------------------
    // Global API selection helpers
    // -----------------------------------------------------------------------------

    GraphicsAPI GetGraphicsAPI()
    {
        return s_CurrentGraphicsAPI;
    }

    void SetGraphicsAPI(GraphicsAPI api)
    {
        if (s_CurrentGraphicsAPI == api)
            return;

        VX_CORE_INFO("Graphics API set to {}", GraphicsAPIToString(api));
        s_CurrentGraphicsAPI = api;
    }

    const char* GraphicsAPIToString(GraphicsAPI api)
    {
        switch (api)
        {
            case GraphicsAPI::None:     return "None";
            case GraphicsAPI::OpenGL:   return "OpenGL";
            case GraphicsAPI::DirectX11:return "DirectX 11";
            case GraphicsAPI::DirectX12:return "DirectX 12";
            case GraphicsAPI::Vulkan:   return "Vulkan";
            case GraphicsAPI::Metal:    return "Metal";
            default:                    return "Unknown";
        }
    }

    const char* VSyncModeToString(VSyncMode mode)
    {
        switch (mode)
        {
            case VSyncMode::Disabled:   return "Disabled";
            case VSyncMode::Enabled:    return "Enabled";
            case VSyncMode::Adaptive:   return "Adaptive";
            case VSyncMode::Fast:       return "Fast";
            case VSyncMode::Mailbox:    return "Mailbox";
            default:                    return "Unknown";
        }
    }

    VSyncMode StringToVSyncMode(const std::string& modeStr)
    {
        std::string lowerStr = modeStr;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
        
        if (lowerStr == "disabled" || lowerStr == "off" || lowerStr == "0")
            return VSyncMode::Disabled;
        else if (lowerStr == "enabled" || lowerStr == "on" || lowerStr == "1")
            return VSyncMode::Enabled;
        else if (lowerStr == "adaptive")
            return VSyncMode::Adaptive;
        else if (lowerStr == "fast")
            return VSyncMode::Fast;
        else if (lowerStr == "mailbox")
            return VSyncMode::Mailbox;
        else
        {
            VX_CORE_WARN("Unknown VSync mode string '{}', defaulting to Disabled", modeStr);
            return VSyncMode::Disabled;
        }
    }

    // -----------------------------------------------------------------------------
    // GraphicsContext factory (minimal placeholder for now)
    // -----------------------------------------------------------------------------

    std::unique_ptr<GraphicsContext> GraphicsContext::Create()
    {
        // Note: This factory cannot create a context without a native window handle.
        // For OpenGL (current supported backend), the OpenGLGraphicsContext requires an SDL_Window*.
        // Preferred flow is: create window -> construct API-specific context with the window -> Initialize(props).
#ifdef VX_OPENGL_SUPPORT
        if (s_CurrentGraphicsAPI == GraphicsAPI::OpenGL)
        {
            VX_CORE_WARN("GraphicsContext::Create() called without a window handle. Returning nullptr.");
            return nullptr;
        }
#endif
        VX_CORE_ERROR("GraphicsContext::Create() not implemented for API: {}", GraphicsAPIToString(s_CurrentGraphicsAPI));
        return nullptr;
    }
}

