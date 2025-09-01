#include "vxpch.h"
#include "RendererAPI.h"
#include "Core/Debug/Log.h"

#ifdef VX_OPENGL_SUPPORT
    #include "OpenGL/OpenGLRendererAPI.h"
#endif

#ifdef VX_DIRECTX11_SUPPORT
    #include "DirectX/DX11RendererAPI.h"
#endif

#ifdef VX_DIRECTX12_SUPPORT
    #include "DirectX/DX12RendererAPI.h"
#endif

#ifdef VX_VULKAN_SUPPORT
    #include "Vulkan/VulkanRendererAPI.h"
#endif

namespace Vortex
{
    // ============================================================================
    // RendererAPI Factory Methods
    // ============================================================================

    std::unique_ptr<RendererAPI> RendererAPI::Create(GraphicsAPI api)
    {
        switch (api)
        {
            #ifdef VX_OPENGL_SUPPORT
                case GraphicsAPI::OpenGL:
                    return std::make_unique<OpenGLRendererAPI>();
            #endif

            #ifdef VX_DIRECTX11_SUPPORT
                case GraphicsAPI::DirectX11:
                    return std::make_unique<DX11RendererAPI>();
            #endif

            #ifdef VX_DIRECTX12_SUPPORT
                case GraphicsAPI::DirectX12:
                    return std::make_unique<DX12RendererAPI>();
            #endif

            #ifdef VX_VULKAN_SUPPORT
                case GraphicsAPI::Vulkan:
                    return std::make_unique<VulkanRendererAPI>();
            #endif

            case GraphicsAPI::None:
                default:
                VX_CORE_ERROR("Unsupported graphics API: {}", static_cast<int>(api));
                return nullptr;
        }
    }

    std::unique_ptr<RendererAPI> RendererAPI::Create()
    {
        // Default preference order based on platform and availability
        #if defined(VX_PLATFORM_WINDOWS)
            // On Windows, prefer DirectX 12 > DirectX 11 > OpenGL > Vulkan
            #ifdef VX_DIRECTX12_SUPPORT
                auto renderer = Create(GraphicsAPI::DirectX12);
                if (renderer) return renderer;
            #endif

            #ifdef VX_DIRECTX11_SUPPORT
                auto renderer = Create(GraphicsAPI::DirectX11);
                if (renderer) return renderer;
            #endif

            #ifdef VX_OPENGL_SUPPORT
                auto renderer = Create(GraphicsAPI::OpenGL);
                if (renderer) return renderer;
            #endif

            #ifdef VX_VULKAN_SUPPORT
                return Create(GraphicsAPI::Vulkan);
            #endif

        #elif defined(VX_PLATFORM_MACOS)
            // On macOS, prefer OpenGL (Metal not yet implemented)
            #ifdef VX_OPENGL_SUPPORT
                return Create(GraphicsAPI::OpenGL);
            #endif

        #else
            // On Linux and other platforms, prefer OpenGL > Vulkan
            #ifdef VX_OPENGL_SUPPORT
                auto renderer = Create(GraphicsAPI::OpenGL);
                if (renderer) return renderer;
            #endif

            #ifdef VX_VULKAN_SUPPORT
                return Create(GraphicsAPI::Vulkan);
            #endif
        #endif

        VX_CORE_ERROR("No supported graphics API found!");
        return nullptr;
    }

    // ============================================================================
    // RendererAPIManager Implementation
    // ============================================================================

    RendererAPIManager& RendererAPIManager::GetInstance()
    {
        static RendererAPIManager instance;
        return instance;
    }

    Result<void> RendererAPIManager::Initialize(GraphicsAPI api, GraphicsContext* context)
    {
        if (m_RendererAPI)
        {
            VX_CORE_WARN("RendererAPIManager already initialized. Shutting down existing renderer.");
            auto shutdownResult = Shutdown();
            if (!shutdownResult.IsSuccess())
            {
                VX_CORE_ERROR("Failed to shutdown existing renderer: {}", shutdownResult.GetErrorMessage());
                return Result<void>(ErrorCode::EngineShutdownFailed, shutdownResult.GetErrorMessage());
            }
        }

        if (!context)
        {
            VX_CORE_ERROR("Graphics context cannot be null");
            return Result<void>(ErrorCode::NullPointer, "Graphics context is null");
        }

        // Create the appropriate renderer API
        m_RendererAPI = RendererAPI::Create(api);
        if (!m_RendererAPI)
        {
            VX_CORE_ERROR("Failed to create renderer API for graphics API: {}", static_cast<int>(api));
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to create renderer API");
        }

        // Initialize the renderer
        auto initResult = m_RendererAPI->Initialize(context);
        if (!initResult.IsSuccess())
        {
            VX_CORE_ERROR("Failed to initialize renderer API: {}", initResult.GetErrorMessage());
            m_RendererAPI.reset();
            return Result<void>(initResult.GetErrorCode(), initResult.GetErrorMessage());
        }

        m_CurrentAPI = api;
        VX_CORE_INFO("Renderer API initialized successfully: {}", m_RendererAPI->GetName());

        return Result<void>();
    }

    Result<void> RendererAPIManager::Shutdown()
    {
        if (!m_RendererAPI)
        {
            // Already shut down or never initialized. This is fine in idempotent shutdown flows.
            VX_CORE_TRACE("RendererAPIManager already shutdown or not initialized");
            return Result<void>();
        }

        VX_CORE_INFO("Shutting down renderer API: {}", m_RendererAPI->GetName());

        auto shutdownResult = m_RendererAPI->Shutdown();
        m_RendererAPI.reset();
        m_CurrentAPI = GraphicsAPI::None;

        if (!shutdownResult.IsSuccess())
        {
            VX_CORE_ERROR("Error during renderer shutdown: {}", shutdownResult.GetErrorMessage());
            return shutdownResult;
        }

        VX_CORE_INFO("Renderer API shutdown completed");
        return Result<void>();
    }

    // ============================================================================
    // Global Access Functions
    // ============================================================================

    RendererAPI* GetRenderer()
    {
        return RendererAPIManager::GetInstance().GetRenderer();
    }

    Result<void> InitializeRenderer(GraphicsAPI api, GraphicsContext* context)
    {
        return RendererAPIManager::GetInstance().Initialize(api, context);
    }

    Result<void> ShutdownRenderer()
    {
        return RendererAPIManager::GetInstance().Shutdown();
    }
}