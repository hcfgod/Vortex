#pragma once

#include "EngineSystem.h"
#include "SystemPriority.h"
#include "Core/Debug/ErrorCodes.h"
#include "Core/Debug/Assert.h"

namespace Vortex 
{
	/// <summary>
	/// Manages the lifecycle and execution of all engine systems.
	/// Handles registration, initialization, updates, and shutdown in the correct order.
	/// </summary>
	class SystemManager
	{
	public:
		SystemManager();
		~SystemManager();

		/// <summary>
		/// Register a system with the manager. Systems are automatically sorted by priority.
		/// </summary>
		/// <typeparam name="T">System type that inherits from EngineSystem</typeparam>
		/// <typeparam name="...Args">Constructor arguments for the system</typeparam>
		/// <returns>Pointer to the created system or nullptr on failure</returns>
		template<typename T, typename... Args>
		T* RegisterSystem(Args&&... args)
		{
			VX_CORE_ASSERT(!m_Initialized, "Cannot register systems after initialization");
			
			auto system = std::make_unique<T>(std::forward<Args>(args)...);
			T* systemPtr = system.get();
			
			// Insert system in priority order
			SystemPriority newSystemPriority = system->GetPriority();
			auto insertPos = std::lower_bound(m_Systems.begin(), m_Systems.end(), newSystemPriority,
				[](const std::unique_ptr<EngineSystem>& a, SystemPriority priority)
				{
					return static_cast<uint32_t>(a->GetPriority()) < static_cast<uint32_t>(priority);
				});
			
			m_Systems.insert(insertPos, std::move(system));
			
			return systemPtr;
		}

		/// <summary>
		/// Get a system by type. Returns nullptr if not found.
		/// </summary>
		template<typename T>
		T* GetSystem() const
		{
			for (const auto& system : m_Systems)
			{
				T* castedSystem = dynamic_cast<T*>(system.get());
				if (castedSystem)
					return castedSystem;
			}
			return nullptr;
		}

		/// <summary>
		/// Initialize all registered systems in priority order.
		/// </summary>
		Result<void> InitializeAllSystems();

		/// <summary>
		/// Update all systems in priority order.
		/// </summary>
		Result<void> UpdateAllSystems();

		/// <summary>
		/// Render all systems in priority order.
		/// </summary>
		Result<void> RenderAllSystems();

		/// <summary>
		/// Shutdown all systems in reverse priority order.
		/// </summary>
		Result<void> ShutdownAllSystems();

		// System information
		size_t GetSystemCount() const { return m_Systems.size(); }
		bool IsInitialized() const { return m_Initialized; }

		/// <summary>
		/// Get list of all registered systems for debugging.
		/// </summary>
		void LogRegisteredSystems() const;

	private:
		std::vector<std::unique_ptr<EngineSystem>> m_Systems;
		bool m_Initialized = false;
	};
}