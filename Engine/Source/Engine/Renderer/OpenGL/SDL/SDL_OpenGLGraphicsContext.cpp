#include <vxpch.h>
#include "SDL_OpenGLGraphicsContext.h"
#include "Core/Debug/Log.h"
#include "Core/Debug/Assert.h"
#include <SDL3/SDL.h>
#include <glad/gl.h>

namespace Vortex
{
    // OpenGL debug callback function
    static void GLAPIENTRY OpenGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                             GLsizei length, const GLchar* message, const void* userParam)
    {
        // Filter out non-significant error/warning codes
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

        std::string sourceStr;
        switch (source)
        {
            case GL_DEBUG_SOURCE_API:             sourceStr = "API"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   sourceStr = "Window System"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader Compiler"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     sourceStr = "Third Party"; break;
            case GL_DEBUG_SOURCE_APPLICATION:     sourceStr = "Application"; break;
            case GL_DEBUG_SOURCE_OTHER:           sourceStr = "Other"; break;
            default:                              sourceStr = "Unknown"; break;
        }

        std::string typeStr;
        switch (type)
        {
            case GL_DEBUG_TYPE_ERROR:               typeStr = "Error"; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated Behaviour"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "Undefined Behaviour"; break;
            case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Portability"; break;
            case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Performance"; break;
            case GL_DEBUG_TYPE_MARKER:              typeStr = "Marker"; break;
            case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "Push Group"; break;
            case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "Pop Group"; break;
            case GL_DEBUG_TYPE_OTHER:               typeStr = "Other"; break;
            default:                                typeStr = "Unknown"; break;
        }

        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH:
                VX_CORE_ERROR("OpenGL Debug ({} - {}): {}", sourceStr, typeStr, message);
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                VX_CORE_WARN("OpenGL Debug ({} - {}): {}", sourceStr, typeStr, message);
                break;
            case GL_DEBUG_SEVERITY_LOW:
                VX_CORE_INFO("OpenGL Debug ({} - {}): {}", sourceStr, typeStr, message);
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                VX_CORE_TRACE("OpenGL Debug ({} - {}): {}", sourceStr, typeStr, message);
                break;
            default:
                VX_CORE_INFO("OpenGL Debug ({} - {}): {}", sourceStr, typeStr, message);
                break;
        }
    }

    // ============================================================================
    // Constructor/Destructor
    // ============================================================================

    SDL_OpenGLGraphicsContext::SDL_OpenGLGraphicsContext(SDL_Window* window) : m_Window(window)
    {
        VX_CORE_ASSERT(window, "Window cannot be null!");
    }

    SDL_OpenGLGraphicsContext::~SDL_OpenGLGraphicsContext()
    {
        if (m_Context)
        {
            VX_CORE_INFO("Destroying OpenGL context");
            SDL_GL_DestroyContext(m_Context);
            m_Context = nullptr;
        }
    }

    // ============================================================================
    // GraphicsContext Interface Implementation
    // ============================================================================

    Result<void> SDL_OpenGLGraphicsContext::Initialize(const GraphicsContextProps& props)
    {
        if (m_Initialized)
        {
            VX_CORE_WARN("OpenGL context already initialized");
            return Result<void>();
        }

        VX_CORE_INFO("Initializing OpenGL context...");

        // Store properties
        m_Props = props;

        // Try to create OpenGL context with fallback versions
        struct OpenGLVersion { int major, minor; };
        const std::vector<OpenGLVersion> versions = { {4, 6}, {4, 5}, {4,4}, {4, 3}, {4, 2}, {4, 1}, {4, 0}, {3, 3} };
        
        for (const auto& version : versions)
        {
            // Set OpenGL attributes before creating context
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version.major);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, version.minor);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            
            #ifdef VX_DEBUG
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
            #endif

            // Set buffer attributes
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

            // Try to create OpenGL context with this version
            m_Context = SDL_GL_CreateContext(m_Window);
            if (m_Context)
            {
                VX_CORE_INFO("OpenGL context created with version {}.{}", version.major, version.minor);
                break;
            }
            else
            {
                VX_CORE_WARN("Failed to create OpenGL {}.{} context: {}", 
                    version.major, version.minor, SDL_GetError());
                SDL_ClearError();
            }
        }
        
        if (!m_Context)
        {
            std::string error = "Failed to create OpenGL context with any supported version";
            VX_CORE_ERROR(error);
            VX_CORE_ERROR("Window pointer: {}", static_cast<void*>(m_Window));
            return Result<void>(ErrorCode::RendererInitFailed, error);
        }

        // Make context current
        auto makeCurrentResult = MakeCurrent();
        if (!makeCurrentResult.IsSuccess())
        {
            return makeCurrentResult;
        }

        // Initialize GLAD
        if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
        {
            VX_CORE_ERROR("Failed to initialize GLAD!");
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to initialize GLAD");
        }

        // Mark initialized before querying GL strings (GetOpenGL* checks IsValid)
        m_Initialized = true;

        // Log OpenGL information
        VX_CORE_INFO("OpenGL Context Initialized:");
        VX_CORE_INFO("  Vendor: {}", GetOpenGLVendor());
        VX_CORE_INFO("  Renderer: {}", GetOpenGLRenderer());
        VX_CORE_INFO("  Version: {}", GetOpenGLVersion());

        // Setup debug callback if available
        #ifdef VX_DEBUG
            SetupDebugCallback();
        #endif

        // Enable depth testing by default
        glEnable(GL_DEPTH_TEST);

        // Enable backface culling by default
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        VX_CORE_INFO("OpenGL context initialized successfully");

        return Result<void>();
    }

    Result<void> SDL_OpenGLGraphicsContext::Present()
    {
        if (!m_Initialized || !m_Context)
        {
            return Result<void>(ErrorCode::InvalidState, "Context not initialized");
        }

        SDL_GL_SwapWindow(m_Window);
        return Result<void>();
    }

    Result<void> SDL_OpenGLGraphicsContext::Resize(uint32_t width, uint32_t height)
    {
        if (!m_Initialized || !m_Context)
        {
            return Result<void>(ErrorCode::InvalidState, "Context not initialized");
        }

        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
        
        // Clear cached framebuffer size since viewport changed
        m_FramebufferSizeCached = false;
        m_FramebufferWidth = width;
        m_FramebufferHeight = height;

        return Result<void>();
    }

    Result<void> SDL_OpenGLGraphicsContext::SetVSync(bool enabled)
    {
        // Legacy method - map to new VSync mode system
        return SetVSyncMode(enabled ? VSyncMode::Enabled : VSyncMode::Disabled);
    }

    Result<void> SDL_OpenGLGraphicsContext::SetVSyncMode(VSyncMode mode)
    {
        if (!m_Initialized || !m_Context)
        {
            return Result<void>(ErrorCode::InvalidState, "Context not initialized");
        }

        // Convert VSync mode to SDL interval
        int interval = VSyncModeToSDLInterval(mode);
        
        VX_CORE_TRACE("Attempting to set VSync mode {} (interval={})", VSyncModeToString(mode), interval);
        
        // Try to set the desired mode
        if (!SDL_GL_SetSwapInterval(interval))
        {
            const char* sdlError = SDL_GetError();
            std::string error = fmt::format("Failed to set VSync mode {} (interval={}): {}", 
                VSyncModeToString(mode), interval, sdlError ? sdlError : "Unknown");
            VX_CORE_WARN(error);
            SDL_ClearError();
            
            // For adaptive mode, try fallback to regular VSync
            if (mode == VSyncMode::Adaptive && SDL_GL_SetSwapInterval(1))
            {
                m_CurrentVSyncMode = VSyncMode::Enabled;
                m_VSyncEnabled = true; // Legacy compatibility
                VX_CORE_INFO("VSync mode set to Enabled (adaptive fallback)");
                return Result<void>();
            }
            
            // If we can't set the desired mode, try to at least disable VSync
            if (mode != VSyncMode::Disabled && SDL_GL_SetSwapInterval(0))
            {
                m_CurrentVSyncMode = VSyncMode::Disabled;
                m_VSyncEnabled = false; // Legacy compatibility
                VX_CORE_WARN("VSync disabled as fallback");
            }
            
            // Return success but with a warning - VSync setting failures are non-fatal
            return Result<void>();
        }

        // Success - update state
        m_CurrentVSyncMode = mode;
        m_VSyncEnabled = (mode != VSyncMode::Disabled); // Legacy compatibility

        return Result<void>();
    }

    bool SDL_OpenGLGraphicsContext::IsValid() const
    {
        return m_Initialized && m_Context != nullptr && m_Window != nullptr;
    }

    Result<void> SDL_OpenGLGraphicsContext::Shutdown()
    {
        if (m_Context)
        {
            VX_CORE_INFO("Shutting down OpenGL context");

            // Detach context from current thread before destruction for safety
            // On some platforms/drivers this avoids teardown crashes.
            if (m_Window)
            {
                SDL_GL_MakeCurrent(m_Window, nullptr);
            }

            SDL_GL_DestroyContext(m_Context);
            m_Context = nullptr;
        }
        m_Initialized = false;
        return Result<void>();
    }

    const GraphicsContextInfo& SDL_OpenGLGraphicsContext::GetInfo() const
    {
        if (IsValid())
        {
            // Update info with current OpenGL state
            const_cast<GraphicsContextInfo&>(m_Info).RendererName = GetOpenGLRenderer();
            const_cast<GraphicsContextInfo&>(m_Info).RendererVersion = GetOpenGLVersion();
            
            if (glGetString(GL_SHADING_LANGUAGE_VERSION))
            {
                const_cast<GraphicsContextInfo&>(m_Info).ShadingLanguageVersion = 
                    reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
            }
            
            // Get OpenGL limits
            GLint maxTextureSize;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
            const_cast<GraphicsContextInfo&>(m_Info).MaxTextureSize = static_cast<uint32_t>(maxTextureSize);
            
            GLint maxVertexAttribs;
            glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
            const_cast<GraphicsContextInfo&>(m_Info).MaxVertexAttributes = static_cast<uint32_t>(maxVertexAttribs);

            // Default framebuffer bits and samples (use SDL for depth/stencil to avoid GL enum availability issues)
            int depthAttr = 0, stencilAttr = 0;
            SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depthAttr);
            SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencilAttr);
            const_cast<GraphicsContextInfo&>(m_Info).DepthBits = static_cast<uint32_t>(depthAttr);
            const_cast<GraphicsContextInfo&>(m_Info).StencilBits = static_cast<uint32_t>(stencilAttr);

            GLint samples = 0;
            glGetIntegerv(GL_SAMPLES, &samples);
            const_cast<GraphicsContextInfo&>(m_Info).SampleCount = static_cast<uint32_t>(samples);

            // sRGB capability (extension check)
            bool hasSRGB = (GLAD_GL_ARB_framebuffer_sRGB != 0) || (GLAD_GL_EXT_framebuffer_sRGB != 0);
            const_cast<GraphicsContextInfo&>(m_Info).SRGBFramebufferCapable = hasSRGB;
            
            // Check for shader support
            const_cast<GraphicsContextInfo&>(m_Info).SupportsGeometryShaders = (GLAD_GL_VERSION_3_2 != 0);
            const_cast<GraphicsContextInfo&>(m_Info).SupportsComputeShaders = (GLAD_GL_VERSION_4_3 != 0);
        }
        
        return m_Info;
    }

    void SDL_OpenGLGraphicsContext::GetFramebufferSize(uint32_t& width, uint32_t& height) const
    {
        if (!m_FramebufferSizeCached && IsValid())
        {
            CacheFramebufferSize();
        }
        width = m_FramebufferWidth;
        height = m_FramebufferHeight;
    }

    // ============================================================================
    // OpenGL-Specific Methods
    // ============================================================================

    Result<void> SDL_OpenGLGraphicsContext::MakeCurrent()
    {
        if (!m_Context || !m_Window)
        {
            return Result<void>(ErrorCode::NullPointer, "Context or window is null");
        }

        // If this context is already current on this thread, nothing to do
        if (SDL_GL_GetCurrentContext() == m_Context && SDL_GL_GetCurrentWindow() == m_Window)
        {
            return Result<void>();
        }

        // Diagnostics: report current before attempting to switch
        SDL_GLContext currentBefore = SDL_GL_GetCurrentContext();
        SDL_Window* windowBefore = SDL_GL_GetCurrentWindow();
        VX_CORE_TRACE("Before MakeCurrent: window={}, ctx={}, currentWin={}, currentCtx={}",
            static_cast<void*>(m_Window), static_cast<void*>(m_Context),
            static_cast<void*>(windowBefore), static_cast<void*>(currentBefore));

        if (SDL_GL_MakeCurrent(m_Window, m_Context) != 0)
        {
            const char* sdlError = SDL_GetError();
            std::string error = fmt::format("Failed to make OpenGL context current: {}",
                sdlError ? sdlError : "Unknown SDL error");
            VX_CORE_ERROR(error);
            VX_CORE_ERROR("MakeCurrent failed: window={}, ctx={}", static_cast<void*>(m_Window), static_cast<void*>(m_Context));

            // Clear the SDL error for next operation
            SDL_ClearError();

            return Result<void>(ErrorCode::RendererInitFailed, error);
        }

        // Diagnostics: report current after switching
        SDL_GLContext currentAfter = SDL_GL_GetCurrentContext();
        SDL_Window* windowAfter = SDL_GL_GetCurrentWindow();
        VX_CORE_TRACE("After MakeCurrent: window={}, ctx={}, currentWin={}, currentCtx={}",
            static_cast<void*>(m_Window), static_cast<void*>(m_Context),
            static_cast<void*>(windowAfter), static_cast<void*>(currentAfter));

        return Result<void>();
    }

    std::string SDL_OpenGLGraphicsContext::GetOpenGLVersion() const
    {
        if (!IsValid()) return "Unknown";
        
        const GLubyte* version = glGetString(GL_VERSION);
        return version ? reinterpret_cast<const char*>(version) : "Unknown";
    }

    std::string SDL_OpenGLGraphicsContext::GetOpenGLRenderer() const
    {
        if (!IsValid()) return "Unknown";
        
        const GLubyte* renderer = glGetString(GL_RENDERER);
        return renderer ? reinterpret_cast<const char*>(renderer) : "Unknown";
    }

    std::string SDL_OpenGLGraphicsContext::GetOpenGLVendor() const
    {
        if (!IsValid()) return "Unknown";
        
        const GLubyte* vendor = glGetString(GL_VENDOR);
        return vendor ? reinterpret_cast<const char*>(vendor) : "Unknown";
    }

    bool SDL_OpenGLGraphicsContext::IsExtensionSupported(const std::string& extension) const
    {
        if (!IsValid()) return false;

        GLint numExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

        for (GLint i = 0; i < numExtensions; ++i)
        {
            const GLubyte* ext = glGetStringi(GL_EXTENSIONS, i);
            if (ext && extension == reinterpret_cast<const char*>(ext))
            {
                return true;
            }
        }

        return false;
    }

    // ============================================================================
    // Private Methods
    // ============================================================================

    void SDL_OpenGLGraphicsContext::CacheFramebufferSize() const
    {
        if (!IsValid()) return;

        // Get viewport size as framebuffer size
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        
        m_FramebufferWidth = static_cast<uint32_t>(viewport[2]);
        m_FramebufferHeight = static_cast<uint32_t>(viewport[3]);
        m_FramebufferSizeCached = true;
    }

    void SDL_OpenGLGraphicsContext::SetupDebugCallback()
    {
        if (!IsValid()) return;

        if (GLAD_GL_ARB_debug_output)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(OpenGLDebugCallback, nullptr);
            
            // Enable all debug messages initially
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            
            VX_CORE_INFO("OpenGL debug callback enabled");
        }
        else
        {
            VX_CORE_WARN("OpenGL debug callback not available");
        }
    }

    std::vector<VSyncMode> SDL_OpenGLGraphicsContext::GetSupportedVSyncModes() const
    {
        if (!m_VSyncModesCached && IsValid())
        {
            CacheSupportedVSyncModes();
        }
        return m_SupportedVSyncModes;
    }

    void SDL_OpenGLGraphicsContext::CacheSupportedVSyncModes() const
    {
        if (!IsValid())
        {
            m_SupportedVSyncModes.clear();
            m_VSyncModesCached = true;
            return;
        }

        m_SupportedVSyncModes.clear();
        
        // Test each VSync mode to see if it's supported
        // We always support disabled mode
        m_SupportedVSyncModes.push_back(VSyncMode::Disabled);
        
        // Test standard VSync (interval = 1)
        if (TestVSyncModeSupport(VSyncMode::Enabled))
        {
            m_SupportedVSyncModes.push_back(VSyncMode::Enabled);
        }
        
        // Test adaptive VSync (interval = -1)
        if (TestVSyncModeSupport(VSyncMode::Adaptive))
        {
            m_SupportedVSyncModes.push_back(VSyncMode::Adaptive);
        }
        
        // Fast mode is typically the same as adaptive for OpenGL
        // but we'll test it separately in case platform differs
        if (TestVSyncModeSupport(VSyncMode::Fast))
        {
            m_SupportedVSyncModes.push_back(VSyncMode::Fast);
        }
        
        // Mailbox mode is not directly supported in OpenGL/SDL, 
        // but we can map it to enabled mode if regular VSync works
        if (std::find(m_SupportedVSyncModes.begin(), m_SupportedVSyncModes.end(), VSyncMode::Enabled) != m_SupportedVSyncModes.end())
        {
            m_SupportedVSyncModes.push_back(VSyncMode::Mailbox);
        }
        
        m_VSyncModesCached = true;
        
        VX_CORE_INFO("Detected {} supported VSync modes", m_SupportedVSyncModes.size());
        for (VSyncMode mode : m_SupportedVSyncModes)
        {
            VX_CORE_TRACE("  - {}", VSyncModeToString(mode));
        }
    }

    int SDL_OpenGLGraphicsContext::VSyncModeToSDLInterval(VSyncMode mode) const
    {
        switch (mode)
        {
            case VSyncMode::Disabled:   return 0;   // No VSync
            case VSyncMode::Enabled:    return 1;   // Standard VSync
            case VSyncMode::Adaptive:   return -1;  // Adaptive VSync
            case VSyncMode::Fast:       return -1;  // Fast VSync (maps to adaptive in SDL)
            case VSyncMode::Mailbox:    return 1;   // Mailbox mode (maps to standard VSync in OpenGL)
            default:
                VX_CORE_WARN("Unknown VSync mode {}, defaulting to disabled", static_cast<int>(mode));
                return 0;
        }
    }

    bool SDL_OpenGLGraphicsContext::TestVSyncModeSupport(VSyncMode mode) const
    {
        if (!IsValid()) return false;
        
        // Store current swap interval to restore later
        int currentInterval = 0;
        if (!SDL_GL_GetSwapInterval(&currentInterval))
        {
            // If we can't get current interval, assume 0 (disabled)
            currentInterval = 0;
        }
        
        // Test the desired mode
        int testInterval = VSyncModeToSDLInterval(mode);
        bool success = SDL_GL_SetSwapInterval(testInterval);
        
        // Restore original interval
        SDL_GL_SetSwapInterval(currentInterval);
        
        // Clear any SDL errors from the test
        SDL_ClearError();
        
        return success;
    }
}