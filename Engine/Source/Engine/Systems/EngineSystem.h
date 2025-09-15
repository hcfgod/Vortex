#pragma once

#include "SystemPriority.h"
#include "Core/Debug/ErrorCodes.h"
#include "Core/Debug/Assert.h"

namespace Vortex 
{
	/// <summary>
	/// Base class for all engine systems providing core functionality and lifecycle management.
	/// All engine subsystems should inherit from this class to ensure proper initialization,
	/// update, and shutdown behavior within the engine architecture.
	/// </summary>
	/// <remarks>
	/// The lifecycle of an engine system follows these stages:
	/// 1. Registration with Engine
	/// 2. Initialization
	/// 3. Regular Updates/Renders
	/// 4. Shutdown
	/// </remarks>
	class EngineSystem
	{
	public:
		EngineSystem(const std::string& name, SystemPriority priority = SystemPriority::Normal);
		virtual ~EngineSystem();

		/// <summary>
		/// Initialize the system. Called once during engine startup.
		/// </summary>
		/// <returns>Result indicating success or failure with error details</returns>
		virtual Result<void> Initialize() = 0;

		/// <summary>
		/// Update the system. Called every frame during the update phase.
		/// </summary>
		/// <param name="deltaTime">Time elapsed since last frame in seconds</param>
		/// <returns>Result indicating success or failure with error details</returns>
		virtual Result<void> Update() { return Result<void>(); }

		/// <summary>
		/// Render the system. Called every frame during the render phase.
		/// </summary>
		/// <returns>Result indicating success or failure with error details</returns>
		virtual Result<void> Render() { return Result<void>(); }

		/// <summary>
		/// Shutdown the system. Called once during engine shutdown.
		/// </summary>
		/// <returns>Result indicating success or failure with error details</returns>
		virtual Result<void> Shutdown() = 0;

		// System information
		const std::string& GetName() const { return m_Name; }
		SystemPriority GetPriority() const { return m_Priority; }
		bool IsInitialized() const { return m_Initialized; }
		bool IsEnabled() const { return m_Enabled; }

		// System control
		void SetEnabled(bool enabled) { m_Enabled = enabled; }

	protected:
		/// <summary>
		/// Mark the system as initialized. Should be called at the end of Initialize() override.
		/// </summary>
		void MarkInitialized() { m_Initialized = true; }

		/// <summary>
		/// Mark the system as shutdown. Should be called at the end of Shutdown() override.
		/// </summary>
		void MarkShutdown() { m_Initialized = false; }

	private:
		std::string m_Name;
		SystemPriority m_Priority;
		bool m_Initialized = false;
		bool m_Enabled = true;
	};

	/// <summary>
	/// Helper macro to define a system's priority.
	/// Place this in the system's .cpp file.
	/// </summary>
	#define VX_DEFINE_SYSTEM_PRIORITY(SystemClass, Priority) \
		template<> \
		SystemPriority GetSystemPriority<SystemClass>() { return Priority; }
}