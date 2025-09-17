#pragma once

#include <Vortex.h>

class ImGuiViewportLayer : public Vortex::Layer
{
public:
	ImGuiViewportLayer();
	~ImGuiViewportLayer() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnImGuiRender() override;
	bool OnEvent(Vortex::Event& event) override;

public:
	Vortex::FrameBufferRef GetFramebuffer() const { return m_Framebuffer; }
	uint32_t GetViewportWidth() const { return m_ViewportWidth; }
	uint32_t GetViewportHeight() const { return m_ViewportHeight; }

private:
	void EnsureFramebuffer(uint32_t width, uint32_t height);

private:
	Vortex::FrameBufferRef m_Framebuffer;
	Vortex::UniformBufferRef m_CameraUBO;
	uint32_t m_ViewportWidth = 1280;
	uint32_t m_ViewportHeight = 720;
	bool m_ViewportHovered = false;
	bool m_ViewportFocused = false;
};


