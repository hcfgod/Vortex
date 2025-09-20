#pragma once

#include "Engine/Systems/EngineSystem.h"
#include "Engine/Renderer/GraphicsContext.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/RenderCommandQueue.h"
#include "Engine/Renderer/Shader/ShaderManager.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Core/Window.h"
#include "Core/Debug/ErrorCodes.h"

namespace Vortex
{
    struct RenderSettings
    {
        // Use RenderCommand::ClearFlags for defaults
        uint32_t ClearFlags = 0; // set at runtime based on capabilities
        glm::vec4 ClearColor = {0.2f, 0.2f, 0.2f, 1.0f};
        float ClearDepth = 1.0f;
        int32_t ClearStencil = 0;
        
        VSyncMode VSync = VSyncMode::Enabled;
    };

    class RenderSystem : public EngineSystem
    {
    public:
        RenderSystem();
        ~RenderSystem() override;

        // EngineSystem interface
        Result<void> Initialize() override;
        Result<void> Update() override { return Result<void>(); }
        Result<void> PreRender();
        Result<void> Render() override;
		Result<void> PostRender();
        Result<void> Shutdown() override;

        // Attach the main window so we can create a graphics context
        Result<void> AttachWindow(Window* window);

        bool IsReady() const { return m_Ready; }

        // Settings control
        void SetRenderSettings(const RenderSettings& settings) { m_Settings = settings; ApplySettings(); }
        const RenderSettings& GetRenderSettings() const { return m_Settings; }

        // Editor/viewport integration: set external scene render target
        void SetSceneRenderTarget(const FrameBufferRef& fb) { m_SceneTarget = fb; }

        // Notify the renderer of window size changes
        Result<void> OnWindowResized(uint32_t width, uint32_t height) { return HandleResize(width, height); }

        // Graphics context access (non-owning)
        GraphicsContext* GetGraphicsContext() { return m_GraphicsContext.get(); }
        const GraphicsContext* GetGraphicsContext() const { return m_GraphicsContext.get(); }

        // Current scene viewport size (FBO if set, else window backbuffer)
        glm::uvec2 GetCurrentViewportSize() const;

    private:
        Result<void> InitializeRendering();
        Result<void> HandleResize(uint32_t width, uint32_t height);
        void ApplySettings();
        void LoadRenderSettingsFromConfig();

    private:
        Window* m_Window = nullptr; // Non-owning
        std::unique_ptr<GraphicsContext> m_GraphicsContext;
        bool m_Ready = false;

        RenderSettings m_Settings;

        // Optional external framebuffer for scene rendering (editor viewport)
        FrameBufferRef m_SceneTarget;
    };
}