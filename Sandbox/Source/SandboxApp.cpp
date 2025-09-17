#define VX_IMPLEMENT_MAIN
#include <Vortex.h>
#include "ExampleGameLayer.h"
#include "ImGuiViewportLayer.h"
#include "SandboxApp.h"

Sandbox::Sandbox()
{
	VX_INFO("Sandbox application created!");
}

Sandbox::~Sandbox()
{
	VX_INFO("Sandbox application destroyed!");
}

void Sandbox::Initialize()
{
	VX_INFO("Sandbox initialized!");

	// Push the ExampleGameLayer to the LayerStack
	m_ExampleLayer = PushLayer<ExampleGameLayer>();
	// Add an ImGui viewport layer that shows scene output
	PushLayer<ImGuiViewportLayer>();

	// Subscribe to additional events for demonstration
	m_CustomSubscription = VX_SUBSCRIBE_EVENT(Vortex::WindowResizeEvent, [this](const Vortex::WindowResizeEvent& e) -> bool 
	{
		VX_INFO("[Custom Handler] Window resized to {}x{}", e.GetWidth(), e.GetHeight());
		return false; // Don't consume the event
	});
}

void Sandbox::Shutdown()
{
	VX_INFO("Sandbox shutting down!");

	// Clean up our custom subscription
	if (m_CustomSubscription != 0)
	{
		VX_UNSUBSCRIBE_EVENT(m_CustomSubscription);
	}
}

// =============================================================================
// APPLICATION & WINDOW EVENT HANDLERS
// =============================================================================
//// Override event handlers to demonstrate the system
//bool OnAppInitialize(const Vortex::ApplicationStartedEvent& event) override
//{
//	VX_INFO("[Event] Application Initialize event received!");
//	return false; // Don't consume the event
//}
//
//bool OnAppUpdate(const Vortex::ApplicationUpdateEvent& event) override
//{
//	// Application update event processed silently
//	return false;
//}
//
//bool OnAppRender(const Vortex::ApplicationRenderEvent& event) override
//{
//	// Application render event processed silently
//	return false;
//}
//
//bool OnAppShutdown(const Vortex::ApplicationShutdownEvent& event) override
//{
//	VX_INFO("[Event] Application Shutdown event received!");
//	return false;
//}
//
//bool OnWindowClose(const Vortex::WindowCloseEvent& event) override
//{
//	VX_INFO("[Event] Window Close event received!");
//	return false; // Let the application handle the close
//}
//
//bool OnWindowResize(const Vortex::WindowResizeEvent& event) override
//{
//	VX_INFO("[Event] Window Resize event: {}x{}", event.GetWidth(), event.GetHeight());
//	return false;
//}
//
//bool OnWindowFocus(const Vortex::WindowFocusEvent& event) override
//{
//	VX_INFO("[Event] Window gained focus!");
//	return false;
//}
//
//bool OnWindowLostFocus(const Vortex::WindowLostFocusEvent& event) override
//{
//	VX_INFO("[Event] Window lost focus!");
//	return false;
//}

// =============================================================================
// INPUT EVENT HANDLERS
// =============================================================================
//bool OnKeyPressed(const Vortex::KeyPressedEvent& event) override
//{
//	VX_INFO("[Event] Key Pressed: {} {} (repeat: {})", 
//		static_cast<int>(event.GetKeyCode()),
//		Vortex::KeyCodeToString(event.GetKeyCode()),
//		event.IsRepeat());
//	
//	// Example: Exit on Escape key
//	if (event.GetKeyCode() == Vortex::KeyCode::Escape)
//	{
//		VX_INFO("Escape key pressed - requesting application exit!");
//		// Could trigger application shutdown here if desired
//	}
//	
//	return false;
//}
//
//bool OnKeyReleased(const Vortex::KeyReleasedEvent& event) override
//{
//	VX_INFO("[Event] Key Released: {} {}", 
//		static_cast<int>(event.GetKeyCode()),
//		Vortex::KeyCodeToString(event.GetKeyCode()));
//	return false;
//}
//
//bool OnKeyTyped(const Vortex::KeyTypedEvent& event) override
//{
//	VX_INFO("[Event] Key Typed: {} ('{}') ", 
//		event.GetCharacter(),
//		static_cast<char>(event.GetCharacter()));
//	return false;
//}
//
//bool OnMouseButtonPressed(const Vortex::MouseButtonPressedEvent& event) override
//{
//	VX_INFO("[Event] Mouse Button Pressed: {} {}", 
//		static_cast<int>(event.GetMouseButton()),
//		Vortex::MouseCodeToString(event.GetMouseButton()));
//	return false;
//}
//
//bool OnMouseButtonReleased(const Vortex::MouseButtonReleasedEvent& event) override
//{
//	VX_INFO("[Event] Mouse Button Released: {} {}", 
//		static_cast<int>(event.GetMouseButton()),
//		Vortex::MouseCodeToString(event.GetMouseButton()));
//	return false;
//}
//
//bool OnMouseMoved(const Vortex::MouseMovedEvent& event) override
//{
//	// Mouse movement processed silently (too frequent for logging)
//	//VX_INFO("[Event] Mouse Moved: X={:.1f}, Y={:.1f}", event.GetX(), event.GetY());
//	return false;
//}
//
//bool OnMouseScrolled(const Vortex::MouseScrolledEvent& event) override
//{
//	VX_INFO("[Event] Mouse Scrolled: X={:.1f}, Y={:.1f}", event.GetXOffset(), event.GetYOffset());
//	return false;
//}

bool Sandbox::OnGamepadConnected(const Vortex::GamepadConnectedEvent& event)
{
	VX_INFO("[Event] Gamepad Connected: ID {}", event.GetGamepadId());
	return false;
}

bool Sandbox::OnGamepadDisconnected(const Vortex::GamepadDisconnectedEvent& event)
{
	VX_INFO("[Event] Gamepad Disconnected: ID {}", event.GetGamepadId());
	return false;
}

bool Sandbox::OnGamepadButtonPressed(const Vortex::GamepadButtonPressedEvent& event)
{
	VX_INFO("[Event] Gamepad Button Pressed: Gamepad {} Button {}",
		event.GetGamepadId(), event.GetButton());
	return false;
}

bool Sandbox::OnGamepadButtonReleased(const Vortex::GamepadButtonReleasedEvent& event)
{
	VX_INFO("[Event] Gamepad Button Released: Gamepad {} Button {}",
		event.GetGamepadId(), event.GetButton());
	return false;
}

bool Sandbox::OnGamepadAxis(const Vortex::GamepadAxisEvent& event)
{
	// Only log significant axis movements to avoid spam
	if (std::abs(event.GetValue()) > 0.8f) // Higher threshold
	{
		VX_INFO("[Event] Gamepad Axis: Gamepad {} Axis {} Value {:.2f}",
			event.GetGamepadId(), event.GetAxis(), event.GetValue());
	}
	return false;
}

Vortex::Application* Vortex::CreateApplication()
{
	return new Sandbox();
}