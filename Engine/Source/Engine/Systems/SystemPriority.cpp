#include "vxpch.h"
#include "SystemPriority.h"

namespace Vortex 
{
	const char* SystemPriorityToString(SystemPriority priority)
	{
		switch (priority)
		{
			case SystemPriority::Critical: return "Critical";
			case SystemPriority::Core: return "Core";
			case SystemPriority::High: return "High";
			case SystemPriority::Normal: return "Normal";
			case SystemPriority::Low: return "Low";
			default: return "Unknown";
		}
	}
}