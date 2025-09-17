#include "ImGuiViewportLayer.h"
#include <imgui.h>
#include "Engine/Systems/ImGuiInterop.h"

using namespace Vortex;

ImGuiViewportLayer::ImGuiViewportLayer()
	: Layer("ImGuiViewport", LayerType::UI, LayerPriority::Normal)
{
}

void ImGuiViewportLayer::OnAttach()
{
	EnsureFramebuffer(m_ViewportWidth, m_ViewportHeight);

	// Simple camera UBO (view-projection only for now)
	struct CameraData { glm::mat4 VP; };
	CameraData cd{ glm::mat4(1.0f) };
	m_CameraUBO = UniformBuffer::Create(sizeof(CameraData), 0, &cd);
}

void ImGuiViewportLayer::OnDetach()
{
	m_Framebuffer.reset();
	m_CameraUBO.reset();
}

void ImGuiViewportLayer::OnUpdate()
{
}

void ImGuiViewportLayer::OnRender()
{
	// Redirect scene rendering into our framebuffer so other layers don't know about it
	if (m_Framebuffer)
	{
		if (auto* rs = Vortex::Sys<Vortex::RenderSystem>())
		{
			rs->SetSceneRenderTarget(m_Framebuffer);
		}
	}
}

void ImGuiViewportLayer::OnImGuiRender()
{
	ImGui::SetNextWindowSize(ImVec2((float)m_ViewportWidth, (float)m_ViewportHeight), ImGuiCond_FirstUseEver);
    // Match ImGui window background to engine clear color
    if (auto* rs = Vortex::Sys<Vortex::RenderSystem>())
    {
        const auto& s = rs->GetRenderSettings();
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(s.ClearColor.r, s.ClearColor.g, s.ClearColor.b, s.ClearColor.a));
    }

    if (ImGui::Begin("Viewport"))
	{
		ImVec2 avail = ImGui::GetContentRegionAvail();
		uint32_t newW = (uint32_t)std::max(1.0f, avail.x);
		uint32_t newH = (uint32_t)std::max(1.0f, avail.y);
		if (newW != m_ViewportWidth || newH != m_ViewportHeight)
		{
			EnsureFramebuffer(newW, newH);
			// Inform RenderSystem immediately so next PreRender uses the correct viewport size
			if (auto* rs = Vortex::Sys<Vortex::RenderSystem>())
			{
				rs->SetSceneRenderTarget(m_Framebuffer);
			}
		}

		m_ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
		m_ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		// Display the framebuffer's first color attachment
		if (m_Framebuffer && m_Framebuffer->GetColorAttachment())
		{
			ImTextureID texId = (ImTextureID)(uintptr_t)m_Framebuffer->GetColorAttachment()->GetRendererID();
			// Remember the top-left of the image region in main viewport space
			ImVec2 windowPos = ImGui::GetWindowPos();
			ImVec2 cursorPos = ImGui::GetCursorScreenPos(); // position where image will be drawn
			ImVec2 topLeft = cursorPos;
			ImVec2 bottomRight = ImVec2(cursorPos.x + avail.x, cursorPos.y + avail.y);
			Vortex::ImGuiViewportInput::SetViewportRect(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
			Vortex::ImGuiViewportInput::SetHoverFocus(m_ViewportHovered, m_ViewportFocused);
			ImGui::Image(texId, avail, ImVec2(0,1), ImVec2(1,0));
		}
	}
	ImGui::End();

    // Pop style if we pushed it
    if (Vortex::Sys<Vortex::RenderSystem>())
    {
        ImGui::PopStyleColor();
    }
}

bool ImGuiViewportLayer::OnEvent(Event& event)
{
	// Forward events only if viewport is hovered/focused (block others if desired)
	if (!m_ViewportHovered && !m_ViewportFocused)
		return false;
	return false;
}

void ImGuiViewportLayer::EnsureFramebuffer(uint32_t width, uint32_t height)
{
	m_ViewportWidth = width;
	m_ViewportHeight = height;

	FrameBufferSpec spec{};
	spec.Width = width;
	spec.Height = height;
	spec.ColorAttachmentCount = 1;
	spec.HasDepth = false;
	m_Framebuffer = FrameBuffer::Create(spec);
}


