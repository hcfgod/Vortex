#pragma once

#include "vxpch.h"
#include "Events/EventSystem.h"
#include "Events/ApplicationEvent.h"
#include "Events/WindowEvent.h"
#include "Events/InputEvent.h"
#include "Core/Layer/LayerStack.h"
#include <memory>

#ifdef VX_USE_SDL
	union SDL_Event;
#endif

namespace Vortex 
{
	class Engine;
	class Window;

class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run(Engine* engine);

		// ===== Layer API (integrates with LayerStack) =====
		LayerStack& GetLayerStack() { return m_LayerStack; }
		const LayerStack& GetLayerStack() const { return m_LayerStack; }

		Layer* PushLayer(std::unique_ptr<Layer> layer) { return m_LayerStack.PushLayer(std::move(layer)); }
		template<typename T, typename... Args>
		T* PushLayer(Args&&... args) { return m_LayerStack.PushLayer<T>(std::forward<Args>(args)...); }
		bool PopLayer(Layer* layer) { return m_LayerStack.PopLayer(layer); }
		bool PopLayer(const std::string& name) { return m_LayerStack.PopLayer(name); }
		size_t PopLayersByType(LayerType type) { return m_LayerStack.PopLayersByType(type); }

		virtual void Initialize() {}
		virtual void Update() {}
		virtual void Render() {}
		virtual void Shutdown() {}
	
		/**
		 * @brief Setup the application with an engine instance (shared by all entry points)
		 * @param engine The engine instance to attach to
		 * 
		 * This method performs the common setup steps needed by both Run() and SDL callbacks:
		 * - Sets up event subscriptions
		 * - Attaches window to RenderSystem
		 * - Enables input handling
		 */
		void SetupWithEngine(Engine* engine);
		
		#ifdef VX_USE_SDL
		virtual void ProcessEvent(const SDL_Event& event) {}
		#endif
	
		#pragma region ClientEventOverrides
		// Event handlers for application lifecycle
		virtual bool OnAppInitialize(const ApplicationStartedEvent& event) { return false; }
		virtual bool OnAppUpdate(const ApplicationUpdateEvent& event) { return false; }
		virtual bool OnAppRender(const ApplicationRenderEvent& event) { return false; }
		virtual bool OnAppShutdown(const ApplicationShutdownEvent& event) { return false; }

		// Event handlers for window events
		virtual bool OnWindowClose(const WindowCloseEvent& event) { return false; }
		virtual bool OnWindowResize(const WindowResizeEvent& event) { return false; }
		virtual bool OnWindowFocus(const WindowFocusEvent& event) { return false; }
		virtual bool OnWindowLostFocus(const WindowLostFocusEvent& event) { return false; }

		// Event handlers for input events
		virtual bool OnKeyPressed(const KeyPressedEvent& event) { return false; }
		virtual bool OnKeyReleased(const KeyReleasedEvent& event) { return false; }
		virtual bool OnKeyTyped(const KeyTypedEvent& event) { return false; }
		virtual bool OnMouseButtonPressed(const MouseButtonPressedEvent& event) { return false; }
		virtual bool OnMouseButtonReleased(const MouseButtonReleasedEvent& event) { return false; }
		virtual bool OnMouseMoved(const MouseMovedEvent& event) { return false; }
		virtual bool OnMouseScrolled(const MouseScrolledEvent& event) { return false; }
		virtual bool OnGamepadConnected(const GamepadConnectedEvent& event) { return false; }
		virtual bool OnGamepadDisconnected(const GamepadDisconnectedEvent& event) { return false; }
		virtual bool OnGamepadButtonPressed(const GamepadButtonPressedEvent& event) { return false; }
		virtual bool OnGamepadButtonReleased(const GamepadButtonReleasedEvent& event) { return false; }
		virtual bool OnGamepadAxis(const GamepadAxisEvent& event) { return false; }
		#pragma endregion

	protected:
		Engine* GetEngine() { return m_Engine; }
		Window* GetWindow() { return m_Window.get(); }

private:
		// Core members
		Engine* m_Engine = nullptr;
		std::unique_ptr<Window> m_Window;

		// Layer management
		LayerStack m_LayerStack;
		
		// Event subscription management
		std::vector<SubscriptionID> m_EventSubscriptions;
		
		// Helper methods
		void SetupEventSubscriptions();
		void CleanupEventSubscriptions();
		
		#ifdef VX_USE_SDL
			void ConvertSDLEventToVortexEvent(const SDL_Event& sdlEvent);
		#endif
	};

	// To be defined in CLIENT
	Application* CreateApplication();
}