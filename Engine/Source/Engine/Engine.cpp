#include "vxpch.h"
#include "Engine.h"
#include "Core/Debug/Log.h"
#include "Core/Debug/Assert.h"
#include "Core/Debug/CrashHandler.h"
#include "Core/Debug/Exception.h"
#include "Core/EngineConfig.h"
#include "Time/TimeSystem.h"
#include "Core/Events/EventSystem.h"
#include "Systems/RenderSystem.h"
#include "Engine/Input/InputSystem.h"
#include "Core/Events/EngineEvent.h"
#include "Engine/Time/Time.h"

namespace Vortex 
{
	Engine::Engine()
	{
		VX_CORE_INFO("Engine Created");
	}

	Engine::~Engine()
	{
		VX_TRY_CATCH_LOG(
			if (m_Initialized)
			{
				auto result = Shutdown();
				VX_LOG_ERROR(result);
			}
		);

		VX_CORE_INFO("Engine Destroyed");
	}

	Result<void> Engine::Initialize()
	{
		VX_CRASH_CONTEXT("Engine::Initialize");
		
		if (m_Initialized)
		{
			VX_CORE_WARN("Engine already initialized!");
			return Result<void>(ErrorCode::EngineAlreadyInitialized, "Engine is already initialized");
		}

		VX_CORE_INFO("Vortex Engine Initializing...");
		
		// Initialize crash handler first
		Debug::CrashHandler::Initialize();
		
		// Register core engine systems
		auto registerCoreSystemsResult = RegisterCoreSystems();
		VX_RETURN_IF_ERROR(registerCoreSystemsResult);
		
		// Initialize all registered systems
		auto systemManagerInitAllResult = m_SystemManager.InitializeAllSystems();
		VX_RETURN_IF_ERROR(systemManagerInitAllResult);
	
		// Cache frequently used systems
		m_RenderSystem = m_SystemManager.GetSystem<RenderSystem>();

		m_Initialized = true;
		m_Running = true;
		VX_CORE_INFO("Vortex Engine Initialized Successfully!");
		
		return Result<void>();
	}

	Result<void> Engine::Shutdown()
	{
		VX_CRASH_CONTEXT("Engine::Shutdown");
		
		if (!m_Initialized)
		{
			VX_CORE_WARN("Engine not initialized, cannot shutdown!");
			return Result<void>(ErrorCode::EngineNotInitialized, "Engine is not initialized");
		}

		VX_CORE_INFO("Shutting down Vortex Engine...");
		
		// Clear layers first before system shutdown
		m_LayerStack.Clear();
	
		// Clear cached system pointers
		m_RenderSystem = nullptr;
		
		// Shutdown all engine systems in reverse order
		auto shutdownResult = m_SystemManager.ShutdownAllSystems();
		VX_LOG_ERROR(shutdownResult);
		
		// Shutdown configuration system
		EngineConfig::Get().Shutdown();
		
		// Shutdown crash handler last
		Debug::CrashHandler::Shutdown();
		
		m_Initialized = false;
		m_Running = false;
		
		VX_CORE_INFO("Vortex Engine Shutdown Complete");
		return Result<void>();
	}

	Result<void> Engine::Update()
	{
		VX_CORE_ASSERT(m_Initialized, "Engine must be initialized before updating");
		
		if (!m_Initialized)
			return Result<void>(ErrorCode::EngineNotInitialized, "Engine not initialized");

		// Update engine systems first
		auto systemResult = m_SystemManager.UpdateAllSystems();
		if (systemResult.IsError())
			return systemResult;
		
		// Update layers (Game -> UI -> Debug -> Overlay)
		m_LayerStack.OnUpdate();
		
		// Dispatch engine update event
		VX_DISPATCH_EVENT(EngineUpdateEvent(Time::GetDeltaTime()));
		
		return Result<void>();
	}

	Result<void> Engine::Render()
	{
		VX_CORE_ASSERT(m_Initialized, "Engine must be initialized before rendering");
		
		if (!m_Initialized)
			return Result<void>(ErrorCode::EngineNotInitialized, "Engine not initialized");

		// Pre-render setup (clear buffers, prepare frame)
		if (m_RenderSystem)
		{
			m_RenderSystem->PreRender();
		}
		
		// Render layers first (Game -> UI -> Debug -> Overlay) so they can queue commands
		m_LayerStack.OnRender();
		
		// Render engine systems (processes queued commands)
		auto systemsResult = m_SystemManager.RenderAllSystems();
		if (systemsResult.IsError())
			return systemsResult;
		
		// Post-render cleanup (present frame)
		if (m_RenderSystem)
		{
			m_RenderSystem->PostRender();
		}
		
		// Dispatch engine render event
		VX_DISPATCH_EVENT(EngineRenderEvent(Time::GetDeltaTime()));
		
		return Result<void>();
	}

	Result<void> Engine::RegisterCoreSystems()
	{
		// Register TimeSystem first (Critical priority - needs to run before everything else)
		auto* timeSystem = m_SystemManager.RegisterSystem<TimeSystem>();
		if (!timeSystem)
		{
			VX_CORE_ERROR("Failed to register TimeSystem");
			return Result<void>(ErrorCode::SystemInitializationFailed, "Failed to register TimeSystem");
		}
		
		// Register EventSystem (Critical priority - needed for communication between systems)
		auto* eventSystem = m_SystemManager.RegisterSystem<EventSystem>();
		if (!eventSystem)
		{
			VX_CORE_ERROR("Failed to register EventSystem");
			return Result<void>(ErrorCode::SystemInitializationFailed, "Failed to register EventSystem");
		}

		// Register RenderSystem (Core priority)
		{
			auto* renderSystem = m_SystemManager.RegisterSystem<RenderSystem>();
			if (!renderSystem)
			{
				VX_CORE_ERROR("Failed to register RenderSystem");
				return Result<void>(ErrorCode::SystemInitializationFailed, "Failed to register RenderSystem");
			}
		}

		// Register InputSystem (High priority)
		{
			auto* inputSystem = m_SystemManager.RegisterSystem<class InputSystem>();
			if (!inputSystem)
			{
				VX_CORE_ERROR("Failed to register InputSystem");
				return Result<void>(ErrorCode::SystemInitializationFailed, "Failed to register InputSystem");
			}
		}

		// TODO: Register other core systems here as they are implemented:
		// - Audio System (High priority)
		// - Physics System (Normal priority)
		// etc.
		
		return Result<void>();
	}
}