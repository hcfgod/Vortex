#include "vxpch.h"
#include "RenderSystem.h"
#include "Core/Debug/Log.h"
#include "Core/EngineConfig.h"
#include "Engine/Renderer/GraphicsContext.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/RenderCommandQueue.h"
#include "Core/Window.h"

#ifdef VX_OPENGL_SUPPORT
    #ifdef VX_USE_SDL
        #include "Engine/Renderer/OpenGL/SDL/SDL_OpenGLGraphicsContext.h"
    #endif
#endif

namespace Vortex
{
    RenderSystem::RenderSystem() : EngineSystem("RenderSystem", SystemPriority::Core)
    {
        VX_CORE_INFO("RenderSystem created");
    }

    RenderSystem::~RenderSystem()
    {
        // Ensure shutdown if not already done
        if (IsInitialized())
        {
            Shutdown();
        }

        VX_CORE_INFO("RenderSystem destroyed");
    }

    void RenderSystem::ApplySettings()
    {
        if (!m_GraphicsContext)
            return;

        // Apply VSync using the VSyncMode system
        auto vsMode = m_GraphicsContext->SetVSyncMode(m_Settings.VSync);
        if (vsMode.IsError())
        {
            VX_CORE_WARN("Failed to apply VSync mode {}: {}", 
                VSyncModeToString(m_Settings.VSync), vsMode.GetErrorMessage());
        }
        else
        {
            VX_CORE_INFO("VSync mode set to {}", VSyncModeToString(m_Settings.VSync));
        }
    }

    void RenderSystem::LoadRenderSettingsFromConfig()
    {
        auto& config = EngineConfig::Get();
        
        try
        {
            // Load VSync mode from configuration
            m_Settings.VSync = config.GetVSyncModeEnum();
            
            // Load clear color from configuration
            auto configColor = config.GetClearColor();
            m_Settings.ClearColor = glm::vec4(configColor.r, configColor.g, configColor.b, configColor.a);
            
            // Load other settings (using defaults for now, can be extended)
            m_Settings.ClearDepth = 1.0f;
            m_Settings.ClearStencil = 0;
            
            VX_CORE_INFO("Render settings loaded from configuration:");
            VX_CORE_INFO("  - VSync: {}", VSyncModeToString(m_Settings.VSync));
            VX_CORE_INFO("  - Clear Color: ({:.2f}, {:.2f}, {:.2f}, {:.2f})", 
                         m_Settings.ClearColor.r, m_Settings.ClearColor.g, 
                         m_Settings.ClearColor.b, m_Settings.ClearColor.a);
        }
        catch (...)
        {
            VX_CORE_WARN("Failed to load render settings from configuration, using defaults");
            // Use default settings
            m_Settings.VSync = VSyncMode::Enabled;
            m_Settings.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
            m_Settings.ClearDepth = 1.0f;
            m_Settings.ClearStencil = 0;
        }
    }

    Result<void> RenderSystem::Initialize()
    {
        // We intentionally don't create the graphics context here because the Window
        // may not be created yet (Application constructs it after Engine init).
        // We'll complete initialization once a window is attached.
        MarkInitialized();
        VX_CORE_INFO("RenderSystem initialized (awaiting window attachment)");
        return Result<void>();
    }

    Result<void> RenderSystem::AttachWindow(Window* window)
    {
        if (!IsInitialized())
        {
            VX_CORE_WARN("RenderSystem not initialized before attaching window; proceeding");
            MarkInitialized();
        }

        if (!window || !window->IsValid())
        {
            return Result<void>(ErrorCode::InvalidParameter, "Invalid window provided to RenderSystem");
        }

        m_Window = window;
        VX_CORE_INFO("RenderSystem attached to window '{}'", window->GetTitle());
        return InitializeRendering();
    }

    Result<void> RenderSystem::InitializeRendering()
    {
        if (!m_Window)
        {
            return Result<void>(ErrorCode::InvalidState, "No window attached to RenderSystem");
        }

        if (m_Ready)
        {
            VX_CORE_WARN("RenderSystem rendering already initialized");
            return Result<void>();
        }

        const auto& props = m_Window->GetProperties();
        GraphicsAPI api = GetGraphicsAPI();

        #ifdef VX_OPENGL_SUPPORT
            #ifdef VX_USE_SDL
                if (api == GraphicsAPI::OpenGL)
                {
                    // Create an OpenGL graphics context bound to our SDL window
                    m_GraphicsContext = std::make_unique<SDL_OpenGLGraphicsContext>(static_cast<SDL_Window*>(m_Window->GetNativeHandle()));

                    GraphicsContextProps ctxProps;
                    ctxProps.WindowHandle = m_Window->GetNativeHandle();
                    ctxProps.WindowWidth = static_cast<uint32_t>(props.Width);
                    ctxProps.WindowHeight = static_cast<uint32_t>(props.Height);

                    #ifdef VX_DEBUG
                        ctxProps.DebugMode = true;
                    #else
                        ctxProps.DebugMode = false;
                    #endif

                    ctxProps.MSAASamples = 1;
                    ctxProps.ApplicationName = props.Title;

                    auto ctxInit = m_GraphicsContext->Initialize(ctxProps);
                    if (ctxInit.IsError())
                    {
                        VX_CORE_ERROR("Failed to initialize GraphicsContext: {}", ctxInit.GetErrorMessage());
                        m_GraphicsContext.reset();
                        return ctxInit;
                    }

                    // Initialize the global RendererAPI with our context
                    auto rinit = InitializeRenderer(api, m_GraphicsContext.get());
                    if (rinit.IsError())
                    {
                        VX_CORE_ERROR("Failed to initialize RendererAPI: {}", rinit.GetErrorMessage());
                        m_GraphicsContext->Shutdown();
                        m_GraphicsContext.reset();
                        return rinit;
                    }

                    // Initialize the global render command queue
                    RenderCommandQueue::Config queueCfg{};
                    auto rqinit = InitializeGlobalRenderQueue(m_GraphicsContext.get(), queueCfg);
                    VX_LOG_ERROR(rqinit);

                    // Set initial viewport to window size via the queue
                    GetRenderCommandQueue().SetViewport(0, 0, static_cast<uint32_t>(props.Width), static_cast<uint32_t>(props.Height), true);

                    // Load render settings from configuration
                    LoadRenderSettingsFromConfig();

                    // Initialize default render settings based on capabilities
                    const auto& info = m_GraphicsContext->GetInfo();
                    m_Settings.ClearFlags = Vortex::ClearCommand::Color | Vortex::ClearCommand::Depth;
                    if (info.StencilBits > 0)
                        m_Settings.ClearFlags |= Vortex::ClearCommand::Stencil;

                    // Apply the loaded settings
                    ApplySettings();

                    m_Ready = true;
                    VX_CORE_INFO("RenderSystem rendering initialized (API: {})", GraphicsAPIToString(api));
                    return Result<void>();
                }
            #endif
        #endif

        VX_CORE_ERROR("Unsupported or disabled GraphicsAPI: {}", GraphicsAPIToString(api));
        return Result<void>(ErrorCode::RendererInitFailed, "Unsupported GraphicsAPI");
    }

    Result<void> RenderSystem::HandleResize(uint32_t width, uint32_t height)
    {
        if (!m_Ready || !m_GraphicsContext)
            return Result<void>();

        auto r = m_GraphicsContext->Resize(width, height);
        if (r.IsError())
            return r;

        GetRenderCommandQueue().SetViewport(0, 0, width, height, true);
        return Result<void>();
    }

    Result<void> RenderSystem::Render()
    {
        if (!m_Ready)
            return Result<void>();

        // Conditional clears based on settings and capabilities
        uint32_t flags = m_Settings.ClearFlags;
        if (m_GraphicsContext && m_GraphicsContext->GetInfo().StencilBits == 0)
        {
            flags &= ~Vortex::ClearCommand::Stencil;
        }

        GetRenderCommandQueue().Clear(flags, m_Settings.ClearColor, m_Settings.ClearDepth, m_Settings.ClearStencil);

        // Process queued commands on the render thread
        auto proc = GetRenderCommandQueue().ProcessCommands();
        VX_LOG_ERROR(proc);

        // Present the frame
        if (m_GraphicsContext)
        {
            auto presentResult = m_GraphicsContext->Present();
            if (presentResult.IsError())
            {
                VX_CORE_WARN("Failed to present frame: {}", presentResult.GetErrorMessage());
                return presentResult;
            }
        }
        
        return Result<void>();
    }

    Result<void> RenderSystem::Shutdown()
    {
        // Idempotent: if nothing is initialized and no renderer/context remains, skip
        if (!IsInitialized() && m_GraphicsContext == nullptr && GetRenderer() == nullptr)
        {
            VX_CORE_TRACE("RenderSystem already shutdown");
            return Result<void>();
        }

        // Shutdown render queue first (flush commands)
        auto rqsd = ShutdownGlobalRenderQueue();
        VX_LOG_ERROR(rqsd);

        // Shutdown RendererAPI first
        auto rshutdown = ShutdownRenderer();
        VX_LOG_ERROR(rshutdown);

        if (m_GraphicsContext)
        {
            auto cshutdown = m_GraphicsContext->Shutdown();
            VX_LOG_ERROR(cshutdown);
            m_GraphicsContext.reset();
        }

        m_Ready = false;
        m_Window = nullptr;
        MarkShutdown();
        VX_CORE_INFO("RenderSystem shutdown complete");
        return Result<void>();
    }
}