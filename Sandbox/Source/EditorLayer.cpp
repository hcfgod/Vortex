#include "EditorLayer.h"
#include <imgui.h>
#include "Engine/Systems/ImGuiInterop.h"
#include "Engine/Systems/SystemAccessors.h"

using namespace Vortex;

EditorLayer::EditorLayer() : Layer("EditorLayer", LayerType::UI, LayerPriority::High)
{
}

void EditorLayer::OnAttach()
{
	EnsureFramebuffer(m_ViewportWidth, m_ViewportHeight);

	// Simple camera UBO (view-projection only for now)
	struct CameraData { glm::mat4 VP; };
	CameraData cd{ glm::mat4(1.0f) };
	m_CameraUBO = UniformBuffer::Create(sizeof(CameraData), 0, &cd);

	// Setup editor camera
	SetupEditorCamera();
}

void EditorLayer::OnDetach()
{
	m_Framebuffer.reset();
	m_CameraUBO.reset();
}

void EditorLayer::OnUpdate()
{
	// Editor camera updates are handled automatically by the CameraSystem
	// The EditorCamera::OnUpdate() method handles all input processing
	
	// Handle camera switching based on engine mode
	auto* app = Application::Get();
	if (app && app->GetEngine())
	{
		auto* cameraSystem = Sys<CameraSystem>();
		if (cameraSystem)
		{
			auto currentMode = app->GetEngine()->GetRunMode();
			auto activeCamera = cameraSystem->GetActiveCamera();
			
			if (currentMode == Engine::RunMode::Edit)
			{
				// In Edit mode, ensure EditorCamera is active
				if (!activeCamera || dynamic_cast<EditorCamera*>(activeCamera.get()) == nullptr)
				{
					cameraSystem->SetActiveCamera(m_EditorCamera);
					VX_INFO("[EditorLayer] Switched to EditorCamera (Edit mode)");
				}
			}
			else if (currentMode == Engine::RunMode::PlayInEditor || currentMode == Engine::RunMode::Production)
			{
				// In Play mode, switch to gameplay camera if available
				auto cameras = cameraSystem->GetCameras();
				for (auto& cam : cameras)
				{
					if (dynamic_cast<PerspectiveCamera*>(cam.get()) != nullptr)
					{
						cameraSystem->SetActiveCamera(cam);
						VX_INFO("[EditorLayer] Switched to PerspectiveCamera (Play mode)");
						break;
					}
				}
			}
		}
	}
}

void EditorLayer::OnRender()
{
	// Redirect scene rendering into our framebuffer so other layers don't know about it
	if (m_Framebuffer)
	{
		if (auto* rs = Sys<RenderSystem>())
		{
			rs->SetSceneRenderTarget(m_Framebuffer);
		}
	}
}

void EditorLayer::OnImGuiRender()
{
	ImGui::SetNextWindowSize(ImVec2((float)m_ViewportWidth, (float)m_ViewportHeight), ImGuiCond_FirstUseEver);
    // Match ImGui window background to engine clear color
    if (auto* rs = Sys<RenderSystem>())
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
			if (auto* rs = Sys<RenderSystem>())
			{
				rs->SetSceneRenderTarget(m_Framebuffer);
			}

			// Update editor camera viewport size
			if (m_EditorCamera)
			{
				m_EditorCamera->SetViewportSize(newW, newH);
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
			ImGuiViewportInput::SetViewportRect(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
			ImGuiViewportInput::SetHoverFocus(m_ViewportHovered, m_ViewportFocused);
			ImGui::Image(texId, avail, ImVec2(0,1), ImVec2(1,0));
		}
	}
	ImGui::End();

    // Pop style if we pushed it
    if (Sys<RenderSystem>())
    {
        ImGui::PopStyleColor();
    }

	// Editor camera controls panel
	if (ImGui::Begin("Editor Camera Controls"))
	{
		ImGui::Text("=== Editor Camera ===");
		
		auto* cameraSystem = Sys<CameraSystem>();
		if (cameraSystem)
		{
			auto activeCamera = cameraSystem->GetActiveCamera();
			if (auto* editorCam = dynamic_cast<EditorCamera*>(activeCamera.get()))
			{
				ImGui::Text("Editor Camera Active");
				
				glm::vec3 pos = editorCam->GetPosition();
				ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
				
				ImGui::Text("Projection: %s", 
						   editorCam->GetProjectionType() == EditorCamera::ProjectionType::Perspective ? 
						   "Perspective" : "Orthographic");
				
				if (editorCam->GetProjectionType() == EditorCamera::ProjectionType::Orthographic)
				{
					float orthoSize = editorCam->GetOrthoSize();
					if (ImGui::SliderFloat("Ortho Size", &orthoSize, 0.1f, 100.0f))
					{
						editorCam->SetOrthoSize(orthoSize);
					}
				}
				
				if (ImGui::Button("Toggle Projection"))
				{
					editorCam->ToggleProjection();
				}
				
				ImGui::Separator();
				ImGui::Text("Controls:");
				ImGui::Text("  WASD: Move");
				ImGui::Text("  Q/E: Up/Down");
				ImGui::Text("  Right-click + drag: Look");
				ImGui::Text("  Shift + Left-click + drag: Zoom");
				ImGui::Text("  Middle-click + Shift + drag: Pan");
				ImGui::Text("  Mouse wheel: Zoom");
			}
			else
			{
				ImGui::Text("Editor Camera Not Active");
			}
		}
	}
	ImGui::End();
}

bool EditorLayer::OnEvent(Event& event)
{
	// Forward events only if viewport is hovered/focused (block others if desired)
	if (!m_ViewportHovered && !m_ViewportFocused)
		return false;
	return false;
}

void EditorLayer::EnsureFramebuffer(uint32_t width, uint32_t height)
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

void EditorLayer::SetupEditorCamera()
{
	VX_INFO("[EditorLayer] Setting up editor camera...");

	// Get the CameraSystem through the system manager
	auto* cameraSystem = Sys<CameraSystem>();
	if (!cameraSystem)
	{
		VX_ERROR("[EditorLayer] CameraSystem not available!");
		return;
	}

	// Create an editor camera
	m_EditorCamera = std::make_shared<EditorCamera>(
		45.0f,    // FOV
		16.0f/9.0f, // Aspect ratio
		0.1f,     // Near clip
		1000.0f   // Far clip
	);

	// Register the editor camera with the system
	cameraSystem->Register(m_EditorCamera);
	
	// Set it as the active camera for editor mode
	cameraSystem->SetActiveCamera(m_EditorCamera);

	VX_INFO("[EditorLayer] Editor camera created and registered successfully");
	VX_INFO("[EditorLayer] Camera position: ({:.1f}, {:.1f}, {:.1f})", 
		   m_EditorCamera->GetPosition().x, m_EditorCamera->GetPosition().y, m_EditorCamera->GetPosition().z);
}
