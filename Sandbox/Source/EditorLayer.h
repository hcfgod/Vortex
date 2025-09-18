#pragma once

#include <Vortex.h>

using namespace Vortex;

class EditorLayer : public Layer
{
public:
	EditorLayer();
	~EditorLayer() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnImGuiRender() override;
	bool OnEvent(Event& event) override;

public:
	FrameBufferRef GetFramebuffer() const { return m_Framebuffer; }
	uint32_t GetViewportWidth() const { return m_ViewportWidth; }
	uint32_t GetViewportHeight() const { return m_ViewportHeight; }

private:
	void EnsureFramebuffer(uint32_t width, uint32_t height);
	void SetupEditorCamera();

private:
	FrameBufferRef m_Framebuffer;
	UniformBufferRef m_CameraUBO;
	uint32_t m_ViewportWidth = 1280;
	uint32_t m_ViewportHeight = 720;
	bool m_ViewportHovered = false;
	bool m_ViewportFocused = false;

	// Editor camera for editor mode
	std::shared_ptr<EditorCamera> m_EditorCamera;
};
