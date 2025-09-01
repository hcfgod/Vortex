#pragma once

#include "vxpch.h"
#include "Core/Debug/ErrorCodes.h"
#include "Systems/SystemManager.h"

namespace Vortex 
{
	class Engine
	{
	public:
		Engine();
		~Engine();

		Result<void> Initialize();
		Result<void> Shutdown();
		Result<void> Update();
		Result<void> Render();
		
		bool IsInitialized() const { return m_Initialized; }
		bool IsRunning() const { return m_Running; }
		void Stop() { m_Running = false; }

		/// <summary>
		/// Get the system manager for registering and accessing systems
		/// </summary>
		SystemManager& GetSystemManager() { return m_SystemManager; }
		const SystemManager& GetSystemManager() const { return m_SystemManager; }

	private:
		bool m_Initialized = false;
		bool m_Running = false;
		
		SystemManager m_SystemManager;
		
		/// <summary>
		/// Register all core engine systems
		/// </summary>
		Result<void> RegisterCoreSystems();
	};
}