#pragma once

#include "Engine/Systems/EngineSystem.h"
#include "Engine/Renderer/GraphicsContext.h"

union SDL_Event;

namespace Vortex
{
    class Window;

    class ImGuiSystem : public EngineSystem
    {
    public:
        ImGuiSystem();
        ~ImGuiSystem() override;

        // EngineSystem interface
        Result<void> Initialize() override;
        Result<void> Render() override;
        Result<void> Shutdown() override;

        // Attach platform/window + graphics context (call after RenderSystem has initialized its context)
        Result<void> AttachPlatform(Window* window, GraphicsContext* context);

    #ifdef VX_USE_SDL
        // Feed SDL events to ImGui backend
        void ProcessSDLEvent(const SDL_Event& event);
    #endif

        bool IsBackendReady() const { return m_BackendsInitialized; }

        // Frame control (Engine orchestrates ImGui layering)
        void BeginFrame();
        void EndFrameAndRender();
        
        // Render the dockspace (called manually by Engine for proper ordering)
        void RenderDockspace();

    private:
    private:
        bool m_ImGuiInitialized = false;
        bool m_BackendsInitialized = false;
        Window* m_Window = nullptr;              // non-owning
        GraphicsContext* m_GraphicsContext = nullptr; // non-owning
    };
}


