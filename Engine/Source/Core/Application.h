#pragma once

#include "Events/EventSystem.h"
#include "Events/ApplicationEvent.h"
#include "Events/WindowEvent.h"
#include "Events/InputEvent.h"
#include <memory>
#include "Engine/Engine.h"

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
		
		// Static access for layers and other systems
		static Application* Get() { return s_Instance; }
		
		// Public accessors for layers
		Engine* GetEngine() const { return m_Engine; }
		Window* GetWindow() const { return m_Window.get(); }

		// ===== Layer API (delegates to Engine) =====
		class LayerStack& GetLayerStack();
		const class LayerStack& GetLayerStack() const;

		class Layer* PushLayer(std::unique_ptr<class Layer> layer);
		template<typename T, typename... Args>
		T* PushLayer(Args&&... args)
		{
			VX_CORE_ASSERT(m_Engine, "Engine must be set before pushing layers");
			return m_Engine->template PushLayer<T>(std::forward<Args>(args)...);
		}

		bool PopLayer(class Layer* layer);
		bool PopLayer(const std::string& name);
		size_t PopLayersByType(int type);

		virtual void Initialize() {}
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

private:
		// Static instance for global access
		static Application* s_Instance;

		// Core members
		Engine* m_Engine = nullptr;
		std::unique_ptr<Window> m_Window;

	// Engine manages layers now
		
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