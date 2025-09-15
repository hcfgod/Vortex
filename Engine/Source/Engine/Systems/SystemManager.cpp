#include "vxpch.h"
#include "SystemManager.h"
#include "Core/Debug/Log.h"

namespace Vortex 
{
	SystemManager::SystemManager()
	{
		// SystemManager created silently
	}

	SystemManager::~SystemManager()
	{
		if (m_Initialized)
		{
			VX_CORE_WARN("SystemManager destructor called while systems are still initialized. Forcing shutdown...");
			ShutdownAllSystems();
		}
		// SystemManager destroyed silently
	}

	Result<void> SystemManager::InitializeAllSystems()
	{
		VX_CORE_ASSERT(!m_Initialized, "Systems are already initialized");

		if (m_Systems.empty())
		{
			VX_CORE_INFO("No engine systems to initialize");
			m_Initialized = true;
			return Result<void>();
		}

		VX_CORE_INFO("Initializing {0} engine systems...", m_Systems.size());

		for (size_t i = 0; i < m_Systems.size(); ++i)
		{
			auto& system = m_Systems[i];
			
			// Only log individual systems if there are more than 3, or on failure
			if (m_Systems.size() > 3)
			{
				VX_CORE_INFO("  [{0}/{1}] {2}...", i + 1, m_Systems.size(), system->GetName());
			}

			auto result = system->Initialize();
			if (!result.IsSuccess())
			{
				VX_CORE_ERROR("Failed to initialize system '{0}': {1}", 
					system->GetName(), 
					ErrorCodeToString(result.GetErrorCode()));

				// Shutdown any systems that were successfully initialized
				for (size_t j = 0; j < i; ++j)
				{
					m_Systems[j]->Shutdown();
				}

				return Result<void>(result.GetErrorCode(), result.GetErrorMessage());
			}
		}

		m_Initialized = true;
		VX_CORE_INFO("Engine systems initialized successfully {}", m_Systems.size());
		
		return Result<void>();
	}  

	Result<void> SystemManager::UpdateAllSystems()
	{
		VX_CORE_ASSERT(m_Initialized, "Systems must be initialized before updating");

		for (auto& system : m_Systems)
		{
			auto result = system->Update();
			if (!result.IsSuccess())
			{
				VX_CORE_ERROR("System '{0}' update failed: {1}", 
					system->GetName(), 
				ErrorCodeToString(result.GetErrorCode()));
				return result;
			}
		}

		return Result<void>();
	}

	Result<void> SystemManager::RenderAllSystems()
	{
		VX_CORE_ASSERT(m_Initialized, "Systems must be initialized before rendering");

		for (auto& system : m_Systems)
		{
			auto result = system->Render();
			if (!result.IsSuccess())
			{
				VX_CORE_ERROR("System '{0}' render failed: {1}", 
					system->GetName(), 
				ErrorCodeToString(result.GetErrorCode()));
				return result;
			}
		}

		return Result<void>();
	}

	Result<void> SystemManager::ShutdownAllSystems()
	{
		if (!m_Initialized)
		{
			VX_CORE_WARN("Attempted to shutdown systems that are not initialized");
			return Result<void>();
		}

		if (m_Systems.empty())
		{
			m_Initialized = false;
			return Result<void>();
		}

		VX_CORE_INFO("Shutting down {0} engine systems...", m_Systems.size());

		ErrorCode lastError = ErrorCode::Success;
		bool hasErrors = false;

		// Shutdown in reverse order (highest priority last)
		for (auto it = m_Systems.rbegin(); it != m_Systems.rend(); ++it)
		{
			auto& system = *it;

			auto result = system->Shutdown();
			if (!result.IsSuccess())
			{
				VX_CORE_ERROR("Failed to shutdown system '{0}': {1}", 
					system->GetName(), 
					ErrorCodeToString(result.GetErrorCode()));
				lastError = result.GetErrorCode();
				hasErrors = true;
			}
		}

		m_Initialized = false;

		if (hasErrors)
		{
			VX_CORE_ERROR("System shutdown completed with errors. Last error: {0}", 
				ErrorCodeToString(lastError));
			return Result<void>(lastError);
		}

		VX_CORE_INFO("Engine systems shutdown successfully");
		return Result<void>();
	}

	void SystemManager::LogRegisteredSystems() const
	{
		VX_CORE_INFO("=== Registered Engine Systems ({0} total) ===", m_Systems.size());
		
		if (m_Systems.empty())
		{
			VX_CORE_INFO("No systems registered");
			return;
		}

		for (size_t i = 0; i < m_Systems.size(); ++i)
		{
			const auto& system = m_Systems[i];
			VX_CORE_INFO("[{0}] {1} - Priority: {2} ({3}), Initialized: {4}", 
				i + 1,
				system->GetName(),
				SystemPriorityToString(system->GetPriority()),
				static_cast<uint32_t>(system->GetPriority()),
				system->IsInitialized() ? "Yes" : "No");
		}
		
		VX_CORE_INFO("=== End System List ===");
	}
}