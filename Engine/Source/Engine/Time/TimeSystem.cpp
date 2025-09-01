#include "vxpch.h"
#include "TimeSystem.h"
#include "Core/Debug/Log.h"

namespace Vortex 
{
	TimeSystem::TimeSystem() : EngineSystem("TimeSystem", SystemPriority::Critical)
	{
		
	}

	Result<void> TimeSystem::Initialize()
	{
		Time::Initialize();
		MarkInitialized();
		return Result<void>();
	}

	Result<void> TimeSystem::Update()
	{
		// Update the global Time class
		Time::Update();
		
		return Result<void>();
	}

	Result<void> TimeSystem::Render()
	{
		// Time system doesn't need to render anything
		return Result<void>();
	}

	Result<void> TimeSystem::Shutdown()
	{
		Time::Shutdown();
		MarkShutdown();
		return Result<void>();
	}
}