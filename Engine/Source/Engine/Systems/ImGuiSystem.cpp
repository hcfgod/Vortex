#include "vxpch.h"
#include "ImGuiSystem.h"
#include "Core/Debug/Log.h"
#include "Core/Window.h"
#include "Engine/Renderer/GraphicsContext.h"
#include "Engine/Renderer/RenderCommandQueue.h"

#include <imgui.h>

#ifdef VX_USE_SDL
    #include <SDL3/SDL.h>
    #include <backends/imgui_impl_sdl3.h>
#endif

#ifdef VX_OPENGL_SUPPORT
    #include <backends/imgui_impl_opengl3.h>
    #include "Engine/Renderer/OpenGL/SDL/SDL_OpenGLGraphicsContext.h"
#endif

namespace Vortex
{
    ImGuiSystem::ImGuiSystem() : EngineSystem("ImGuiSystem", SystemPriority::High) {}
    ImGuiSystem::~ImGuiSystem() {}

    Result<void> ImGuiSystem::Initialize()
    {
        // Create ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // multi-viewport

        // Style
        ImGui::StyleColorsDark();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 6.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        m_ImGuiInitialized = true;
        MarkInitialized();
        VX_CORE_INFO("ImGuiSystem initialized (context created)");
        return Result<void>();
    }

    Result<void> ImGuiSystem::AttachPlatform(Window* window, GraphicsContext* context)
    {
        m_Window = window;
        m_GraphicsContext = context;

        if (!m_ImGuiInitialized)
            return Result<void>(ErrorCode::InvalidState, "ImGui context not initialized");

#ifdef VX_USE_SDL
        if (!window)
            return Result<void>(ErrorCode::InvalidParameter, "Window is null");
        SDL_Window* sdlWindow = static_cast<SDL_Window*>(window->GetNativeHandle());
        if (!sdlWindow)
            return Result<void>(ErrorCode::InvalidState, "Invalid SDL window");
        void* glCtxPtr = nullptr;
#ifdef VX_OPENGL_SUPPORT
        if (auto* oglCtx = dynamic_cast<SDL_OpenGLGraphicsContext*>(m_GraphicsContext))
        {
            glCtxPtr = oglCtx->GetGLContext();
        }
#endif
        if (!ImGui_ImplSDL3_InitForOpenGL(sdlWindow, glCtxPtr))
            return Result<void>(ErrorCode::RendererInitFailed, "ImGui_ImplSDL3_InitForOpenGL failed");
#endif

#ifdef VX_OPENGL_SUPPORT
        if (!ImGui_ImplOpenGL3_Init("#version 460"))
            return Result<void>(ErrorCode::RendererInitFailed, "ImGui_ImplOpenGL3_Init failed");
#endif

        m_BackendsInitialized = true;
        VX_CORE_INFO("ImGui backends initialized (SDL3 + OpenGL3)");
        return Result<void>();
    }

    void ImGuiSystem::BeginFrame()
    {
        if (!m_BackendsInitialized) return;
#ifdef VX_OPENGL_SUPPORT
        ImGui_ImplOpenGL3_NewFrame();
#endif
#ifdef VX_USE_SDL
        ImGui_ImplSDL3_NewFrame();
#endif
        ImGui::NewFrame();
    }

    void ImGuiSystem::EndFrameAndRender()
    {
        if (!m_BackendsInitialized) return;
        ImGui::Render();
#ifdef VX_OPENGL_SUPPORT
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            // Backup current GL context and window before ImGui renders additional platform windows
#ifdef VX_USE_SDL
#ifdef VX_OPENGL_SUPPORT
            SDL_Window* backupWindow = SDL_GL_GetCurrentWindow();
            SDL_GLContext backupContext = SDL_GL_GetCurrentContext();
#endif
#endif

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            // Restore previous GL context to keep engine render state consistent
#ifdef VX_USE_SDL
#ifdef VX_OPENGL_SUPPORT
            if (backupWindow && backupContext)
            {
                SDL_GL_MakeCurrent(backupWindow, backupContext);
            }
#endif
#endif
        }
    }

    Result<void> ImGuiSystem::Render()
    {
        if (!m_BackendsInitialized) return Result<void>();

        // Central dockspace and basic menu bar for tooling (no Begin/End frame here)
        ImGuiIO& io = ImGui::GetIO();
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        // Use main viewport position/size so docking guides align correctly in windowed mode
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar |
                                 ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("###VortexDockspace", nullptr, flags);
        ImGui::PopStyleVar(3);

        // Dockspace
        ImGuiID dockspaceID = ImGui::GetID("VortexDockspaceID");
        ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit"))
                {
                    // no-op here; client can handle
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Docking Enabled", nullptr, (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        return Result<void>();
    }

    Result<void> ImGuiSystem::Shutdown()
    {
        if (m_BackendsInitialized)
        {
#ifdef VX_OPENGL_SUPPORT
            ImGui_ImplOpenGL3_Shutdown();
#endif
#ifdef VX_USE_SDL
            ImGui_ImplSDL3_Shutdown();
#endif
            m_BackendsInitialized = false;
        }

        if (m_ImGuiInitialized)
        {
            ImGui::DestroyContext();
            m_ImGuiInitialized = false;
        }

        MarkShutdown();
        VX_CORE_INFO("ImGuiSystem shutdown complete");
        return Result<void>();
    }

#ifdef VX_USE_SDL
    void ImGuiSystem::ProcessSDLEvent(const SDL_Event& event)
    {
        if (!m_BackendsInitialized) return;
        ImGui_ImplSDL3_ProcessEvent(&event);
    }
#endif
}


