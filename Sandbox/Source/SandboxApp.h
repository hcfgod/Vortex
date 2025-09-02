#pragma once
#include "Vortex.h"
#include "ExampleGameLayer.h"

class Sandbox : public Vortex::Application
{
public:
	Sandbox();

	~Sandbox();

	void Initialize() override;

	void Shutdown() override;

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

	bool OnGamepadConnected(const Vortex::GamepadConnectedEvent& event) override;

	bool OnGamepadDisconnected(const Vortex::GamepadDisconnectedEvent& event) override;

	bool OnGamepadButtonPressed(const Vortex::GamepadButtonPressedEvent& event) override;

	bool OnGamepadButtonReleased(const Vortex::GamepadButtonReleasedEvent& event) override;

	bool OnGamepadAxis(const Vortex::GamepadAxisEvent& event) override;

private:
	Vortex::SubscriptionID m_CustomSubscription = 0;
	uint64_t m_FrameCount = 0;
	ExampleGameLayer* m_ExampleLayer = nullptr;
};