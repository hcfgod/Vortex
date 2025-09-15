#include "vxpch.h"
#include "Application.h"
#include "Core/Debug/Log.h"
#include "Core/EngineConfig.h"
#include "Core/Window.h"
#include "Engine/Engine.h"
#include "Engine/Systems/RenderSystem.h"
#include "Engine/Time/Time.h"
#include "Events/EventSystem.h"
#include "Events/ApplicationEvent.h"
#include "Events/WindowEvent.h"
#include "Events/InputEvent.h"

#ifdef VX_USE_SDL
	#include "Platform/SDL/SDL3Manager.h"
#endif

namespace Vortex
{
	// Static instance definition
	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		VX_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		
		VX_CORE_INFO("Application Created");

		#ifdef VX_USE_SDL
			if (!SDL3Manager::Initialize(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK))
			{
				VX_CORE_CRITICAL("Failed to initialize SDL3Manager!");
				return;
			}
		#endif

		// Create the main window using configuration
		auto windowProps = EngineConfig::Get().CreateWindowProperties();
		
		m_Window = Vortex::CreatePlatformWindow(windowProps);
		if (m_Window && !m_Window->IsValid())
		{
			VX_CORE_CRITICAL("Failed to create main window!");
			return;
		}
	}

	Application::~Application()
{
	// Clear static instance
	if (s_Instance == this)
		s_Instance = nullptr;
	
	// Clean up event subscriptions before engine shutdown so EventSystem is valid
	CleanupEventSubscriptions();
	
	// Shut down the engine so layers/systems clean up while RenderSystem/queue are alive
	if (m_Engine)
	{
		auto engShutdown = m_Engine->Shutdown();
		VX_LOG_ERROR(engShutdown);
	}
		
	#ifdef VX_USE_SDL
			// Stop SDL text input if active
			if (m_Window && m_Window->IsValid())
			{
				if (SDL_TextInputActive(static_cast<SDL_Window*>(m_Window->GetNativeHandle())))
				{
					SDL_StopTextInput(static_cast<SDL_Window*>(m_Window->GetNativeHandle()));
				}
			}
	#endif
		
		// Destroy window after engine shutdown
		m_Window.reset();
		
		#ifdef VX_USE_SDL
			// Shutdown SDL3
			SDL3Manager::Shutdown();
		#endif // VX_USE_SDL
		
		VX_CORE_INFO("Application Destroyed");
	}

	void Application::Run(Engine* engine)
	{	
		SetupWithEngine(engine);

		// Dispatch application initialize event
		VX_DISPATCH_EVENT(ApplicationStartedEvent());

		Initialize();

		VX_CORE_INFO("Starting application main loop...");

		// Main loop - simplified to delegate to Engine
		#ifdef VX_USE_SDL
			SDL_Event event;
		#endif

		while (m_Engine->IsRunning())
		{
			#ifdef VX_USE_SDL
				// Handle SDL events
				while (SDL_PollEvent(&event))
				{
					// Convert SDL events to Vortex events and dispatch/forward to layers
					ConvertSDLEventToVortexEvent(event);

					// Let the window process its events
					if (m_Window && m_Window->ProcessEvent(event))
					{
						// Window handled the event
						if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
						{
							// Forward to layers first; if not consumed, dispatch globally
							{
								WindowCloseEvent e;
								if (!m_Engine->GetLayerStack().OnEvent(e))
								{
									VX_DISPATCH_EVENT(e);
								}
							}
							m_Engine->Stop();
						}
					}
				}
			#endif

			// Hot-reload configuration and apply updated settings
			if (EngineConfig::Get().ReloadChangedConfigs())
			{
				// Apply window updates (title/size/fullscreen)
				EngineConfig::Get().ApplyWindowSettings(m_Window.get());

				// Apply renderer updates (VSync/clear color)
				if (auto* rs = m_Engine->GetSystemManager().GetSystem<RenderSystem>())
				{
					EngineConfig::Get().ApplyRenderSettings(rs);
				}
			}

			// Handle application-level SDL events (client override)
			ProcessEvent(event);

			// Engine handles systems and layers update
			auto engineUpdateResult = m_Engine->Update();
			if (engineUpdateResult.IsError())
			{
				VX_CORE_ERROR("Engine update failed: {0}", engineUpdateResult.GetErrorMessage());
			}


			// Engine handles layers and systems rendering
			auto engineRenderResult = m_Engine->Render();
			if (engineRenderResult.IsError())
			{
				VX_CORE_ERROR("Engine render failed: {0}", engineRenderResult.GetErrorMessage());
			}
		}

		// Dispatch application shutdown event (only if EventSystem still initialized)
		if (EventSystem::IsInitialized())
		{
			VX_DISPATCH_EVENT(ApplicationShutdownEvent());
		}
		
		Shutdown();

		VX_CORE_INFO("Application main loop finished");
	}
	
	void Application::SetupWithEngine(Engine* engine)
	{
		m_Engine = engine;
		
		// Setup event subscriptions now that EventSystem is available
		SetupEventSubscriptions();

		// Attach window to the RenderSystem so it can create the graphics context
		if (auto* renderSystem = m_Engine->GetSystemManager().GetSystem<RenderSystem>())
		{
			auto rs = renderSystem->AttachWindow(m_Window.get());
			VX_LOG_ERROR(rs);
		}
		else
		{
			VX_CORE_WARN("RenderSystem not found; rendering will be disabled");
		}

		if (!m_Window || !m_Window->IsValid())
		{
			VX_CORE_CRITICAL("No valid window available for application!");
			return;
		}
		
		#ifdef VX_USE_SDL
			// Enable SDL text input for keyboard text events
			SDL_StartTextInput(static_cast<SDL_Window*>(m_Window->GetNativeHandle()));
		#endif
		
		VX_CORE_INFO("Application setup with engine completed");
	}
	
	void Application::SetupEventSubscriptions()
	{
		// Subscribe to application lifecycle events
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(ApplicationStartedEvent, this, &Application::OnAppInitialize)
		);
		
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(ApplicationUpdateEvent, this, &Application::OnAppUpdate)
		);
		
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(ApplicationRenderEvent, this, &Application::OnAppRender)
		);
		
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(ApplicationShutdownEvent, this, &Application::OnAppShutdown)
		);
		
		// Subscribe to window events
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(WindowCloseEvent, this, &Application::OnWindowClose)
		);
		
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(WindowResizeEvent, this, &Application::OnWindowResize)
		);
		
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(WindowFocusEvent, this, &Application::OnWindowFocus)
		);
		
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(WindowLostFocusEvent, this, &Application::OnWindowLostFocus)
		);
		
		// Subscribe to input events (keyboard)
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(KeyPressedEvent, this, &Application::OnKeyPressed)
		);
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(KeyReleasedEvent, this, &Application::OnKeyReleased)
		);
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(KeyTypedEvent, this, &Application::OnKeyTyped)
		);
		
		// Subscribe to input events (mouse)
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(MouseButtonPressedEvent, this, &Application::OnMouseButtonPressed)
		);
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(MouseButtonReleasedEvent, this, &Application::OnMouseButtonReleased)
		);
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(MouseMovedEvent, this, &Application::OnMouseMoved)
		);
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(MouseScrolledEvent, this, &Application::OnMouseScrolled)
		);
		
		// Subscribe to input events (gamepad)
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(GamepadConnectedEvent, this, &Application::OnGamepadConnected)
		);
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(GamepadDisconnectedEvent, this, &Application::OnGamepadDisconnected)
		);
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(GamepadButtonPressedEvent, this, &Application::OnGamepadButtonPressed)
		);
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(GamepadButtonReleasedEvent, this, &Application::OnGamepadButtonReleased)
		);
		m_EventSubscriptions.push_back(
			VX_SUBSCRIBE_EVENT_METHOD(GamepadAxisEvent, this, &Application::OnGamepadAxis)
		);
		
		VX_CORE_INFO("Application event subscriptions ready ({} handlers)", m_EventSubscriptions.size());
	}
	
	void Application::CleanupEventSubscriptions()
	{
		// Unsubscribe from all events
		for (SubscriptionID id : m_EventSubscriptions)
		{
			VX_UNSUBSCRIBE_EVENT(id);
		}
		
		m_EventSubscriptions.clear();
		VX_CORE_INFO("Application event subscriptions cleanup complete");
	}
	
	#ifdef VX_USE_SDL
	void Application::ConvertSDLEventToVortexEvent(const SDL_Event& sdlEvent)
	{
		// Convert SDL events to Vortex events and dispatch them
		switch (sdlEvent.type)
		{
			// =============================================================================
			// WINDOW EVENTS
			// =============================================================================
			case SDL_EVENT_WINDOW_RESIZED:
			{
				uint32_t newW = static_cast<uint32_t>(sdlEvent.window.data1);
				uint32_t newH = static_cast<uint32_t>(sdlEvent.window.data2);
				{
				WindowResizeEvent e(newW, newH);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				// Inform the RenderSystem so it can update viewport/context (always)
				if (auto* rs = m_Engine->GetSystemManager().GetSystem<RenderSystem>())
				{
					rs->OnWindowResized(newW, newH);
				}
				break;
			}
			
			case SDL_EVENT_WINDOW_FOCUS_GAINED:
			{
			WindowFocusEvent e;
			if (!m_Engine->GetLayerStack().OnEvent(e))
				{
					VX_DISPATCH_EVENT(e);
				}
				break;
			}
			
			case SDL_EVENT_WINDOW_FOCUS_LOST:
			{
			WindowLostFocusEvent e;
			if (!m_Engine->GetLayerStack().OnEvent(e))
				{
					VX_DISPATCH_EVENT(e);
				}
				break;
			}
			
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			{
				// WindowCloseEvent is already dispatched in the main loop
				break;
			}
			
			// =============================================================================
			// KEYBOARD EVENTS
			// =============================================================================
			case SDL_EVENT_KEY_DOWN:
			{
				KeyCode keyCode = static_cast<KeyCode>(sdlEvent.key.scancode);
				bool isRepeat = sdlEvent.key.repeat != 0;
				{
				KeyPressedEvent e(keyCode, isRepeat);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			case SDL_EVENT_KEY_UP:
			{
				KeyCode keyCode = static_cast<KeyCode>(sdlEvent.key.scancode);
				{
				KeyReleasedEvent e(keyCode);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			case SDL_EVENT_TEXT_INPUT:
			{
				// SDL provides text as UTF-8 string, we'll use the first character
				if (sdlEvent.text.text[0] != '\0')
				{
					uint32_t character = static_cast<uint32_t>(sdlEvent.text.text[0]);
				KeyTypedEvent e(character);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			// =============================================================================
			// MOUSE EVENTS
			// =============================================================================
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			{
				MouseCode button = static_cast<MouseCode>(sdlEvent.button.button - 1); // SDL buttons are 1-based
				{
				MouseButtonPressedEvent e(button);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			case SDL_EVENT_MOUSE_BUTTON_UP:
			{
				MouseCode button = static_cast<MouseCode>(sdlEvent.button.button - 1); // SDL buttons are 1-based
				{
				MouseButtonReleasedEvent e(button);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			case SDL_EVENT_MOUSE_MOTION:
			{
				float x = static_cast<float>(sdlEvent.motion.x);
				float y = static_cast<float>(sdlEvent.motion.y);
				{
				MouseMovedEvent e(x, y);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			case SDL_EVENT_MOUSE_WHEEL:
			{
				float xOffset = sdlEvent.wheel.x;
				float yOffset = sdlEvent.wheel.y;
				{
				MouseScrolledEvent e(xOffset, yOffset);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			// =============================================================================
			// GAMEPAD EVENTS
			// =============================================================================
			case SDL_EVENT_GAMEPAD_ADDED:
			{
				int gamepadId = sdlEvent.gdevice.which;
				// Open the gamepad so that button/axis events are generated by SDL
				if (SDL_Gamepad* pad = SDL_OpenGamepad(gamepadId))
				{
					VX_CORE_INFO("Opened gamepad {} for input events", gamepadId);
				}
				else
				{
					VX_CORE_WARN("Failed to open gamepad {}: {}", gamepadId, SDL_GetError());
				}
				{
				GamepadConnectedEvent e(gamepadId);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			case SDL_EVENT_GAMEPAD_REMOVED:
			{
				int gamepadId = sdlEvent.gdevice.which;
				// Close the gamepad handle if it is open
				if (SDL_Gamepad* pad = SDL_GetGamepadFromID(gamepadId))
				{
					SDL_CloseGamepad(pad);
					VX_CORE_INFO("Closed gamepad {}", gamepadId);
				}
				{
				GamepadDisconnectedEvent e(gamepadId);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			{
				int gamepadId = sdlEvent.gbutton.which;
				int button = sdlEvent.gbutton.button;
				{
				GamepadButtonPressedEvent e(gamepadId, button);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
			{
				int gamepadId = sdlEvent.gbutton.which;
				int button = sdlEvent.gbutton.button;
				{
				GamepadButtonReleasedEvent e(gamepadId, button);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			{
				int gamepadId = sdlEvent.gaxis.which;
				int axis = sdlEvent.gaxis.axis;
				// SDL3 may report axis values as floats [-1,1] or as int16 depending on backend
				float raw = static_cast<float>(sdlEvent.gaxis.value);
				float value = (std::abs(raw) > 1.1f) ? (raw / 32767.0f) : raw; // Robust normalization
				{
				GamepadAxisEvent e(gamepadId, axis, value);
				if (!m_Engine->GetLayerStack().OnEvent(e))
					{
						VX_DISPATCH_EVENT(e);
					}
				}
				break;
			}
			
			// =============================================================================
			// JOYSTICK EVENTS (fallback for controllers not recognized as gamepads)
			// =============================================================================
			case SDL_EVENT_JOYSTICK_ADDED:
			{
				SDL_JoystickID jid = sdlEvent.jdevice.which;
				if (SDL_IsGamepad(jid))
				{
					if (SDL_Gamepad* pad = SDL_OpenGamepad(jid))
					{
						VX_CORE_INFO("Opened gamepad {} (from JOYSTICK_ADDED)", static_cast<int>(jid));
					}
				}
				else if (SDL_Joystick* js = SDL_OpenJoystick(jid))
				{
					VX_CORE_INFO("Opened joystick {} for input events", static_cast<int>(jid));
				}
				{
					GamepadConnectedEvent e(static_cast<int>(jid));
					if (!m_Engine->GetLayerStack().OnEvent(e)) { VX_DISPATCH_EVENT(e); }
				}
				break;
			}
			case SDL_EVENT_JOYSTICK_REMOVED:
			{
				SDL_JoystickID jid = sdlEvent.jdevice.which;
				if (SDL_Gamepad* pad = SDL_GetGamepadFromID(jid))
				{
					SDL_CloseGamepad(pad);
				}
				else if (SDL_Joystick* js = SDL_GetJoystickFromID(jid))
				{
					SDL_CloseJoystick(js);
				}
				{
					GamepadDisconnectedEvent e(static_cast<int>(jid));
					if (!m_Engine->GetLayerStack().OnEvent(e)) { VX_DISPATCH_EVENT(e); }
				}
				break;
			}
			case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
			{
				int jid = sdlEvent.jbutton.which;
				int button = sdlEvent.jbutton.button;
				GamepadButtonPressedEvent e(jid, button);
				if (!m_Engine->GetLayerStack().OnEvent(e)) { VX_DISPATCH_EVENT(e); }
				break;
			}
			case SDL_EVENT_JOYSTICK_BUTTON_UP:
			{
				int jid = sdlEvent.jbutton.which;
				int button = sdlEvent.jbutton.button;
				GamepadButtonReleasedEvent e(jid, button);
				if (!m_Engine->GetLayerStack().OnEvent(e)) { VX_DISPATCH_EVENT(e); }
				break;
			}
			case SDL_EVENT_JOYSTICK_AXIS_MOTION:
			{
				int jid = sdlEvent.jaxis.which;
				int axis = sdlEvent.jaxis.axis;
				float raw = static_cast<float>(sdlEvent.jaxis.value);
				float value = (std::abs(raw) > 1.1f) ? (raw / 32767.0f) : raw;
				GamepadAxisEvent e(jid, axis, value);
				if (!m_Engine->GetLayerStack().OnEvent(e)) { VX_DISPATCH_EVENT(e); }
				break;
			}
			default:
				break;
		}
	}
	#endif // VX_USE_SDL
	
	// ===== Layer API delegates to the engine =====
	LayerStack& Application::GetLayerStack()
	{
		VX_CORE_ASSERT(m_Engine, "Engine must be set before accessing LayerStack");
		return m_Engine->GetLayerStack();
	}
	
	const LayerStack& Application::GetLayerStack() const
	{
		VX_CORE_ASSERT(m_Engine, "Engine must be set before accessing LayerStack");
		return m_Engine->GetLayerStack();
	}
	
	Layer* Application::PushLayer(std::unique_ptr<Layer> layer)
	{
		VX_CORE_ASSERT(m_Engine, "Engine must be set before pushing layers");
		return m_Engine->PushLayer(std::move(layer));
	}
	
	bool Application::PopLayer(Layer* layer)
	{
		VX_CORE_ASSERT(m_Engine, "Engine must be set before popping layers");
		return m_Engine->PopLayer(layer);
	}
	
	bool Application::PopLayer(const std::string& name)
	{
		VX_CORE_ASSERT(m_Engine, "Engine must be set before popping layers");
		return m_Engine->PopLayer(name);
	}
	
	size_t Application::PopLayersByType(int type)
	{
		VX_CORE_ASSERT(m_Engine, "Engine must be set before popping layers");
		return m_Engine->PopLayersByType(static_cast<LayerType>(type));
	}	
}