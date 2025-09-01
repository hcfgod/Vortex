#include "vxpch.h"
#include "EngineSystem.h"
#include "Core/Debug/Log.h"

namespace Vortex 
{
	EngineSystem::EngineSystem(const std::string& name, SystemPriority priority)
		: m_Name(name), m_Priority(priority)
	{
		// System created silently
	}

	EngineSystem::~EngineSystem()
	{
		// System destroyed silently
	}
}