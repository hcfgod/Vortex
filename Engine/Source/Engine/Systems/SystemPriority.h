#pragma once

#include <cstdint>

namespace Vortex 
{
	/// <summary>
	/// Represents the possible priorities of an engine system.
	/// Systems are initialized in priority order (Core first, Low last)
	/// and shutdown in reverse priority order (Low first, Core last).
	/// </summary>
	enum class SystemPriority : uint32_t
	{
		/// <summary>Core engine systems like crash handling, logging, and memory management</summary>
		Critical = 0,
		
		/// <summary>Core engine systems like rendering and asset management</summary>
		Core = 1,
		
		/// <summary>High-level systems that depend on core systems</summary>
		High = 2,
		
		/// <summary>Standard game systems</summary>
		Normal = 3,
		
		/// <summary>Optional or utility systems</summary>
		Low = 4
	};

	/// <summary>
	/// Helper function to convert SystemPriority to string for logging
	/// </summary>
	const char* SystemPriorityToString(SystemPriority priority);

	/// <summary>
	/// Macro to register a system with a specific priority.
	/// Usage: VX_SYSTEM_PRIORITY(MySystem, SystemPriority::Core)
	/// </summary>
	#define VX_SYSTEM_PRIORITY(systemClass, priority) \
		namespace { \
			static const SystemPriority systemClass##_Priority = priority; \
		} \
		template<> \
		SystemPriority GetSystemPriority<systemClass>() { return systemClass##_Priority; }

	/// <summary>
	/// Template function to get the priority of a system type.
	/// Can be specialized for custom systems or use the macro above.
	/// </summary>
	template<typename T>
	SystemPriority GetSystemPriority() { return SystemPriority::Normal; }
}