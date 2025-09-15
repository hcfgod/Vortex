#pragma once

#include "Engine/Systems/EngineSystem.h"
#include "Time.h"

namespace Vortex 
{
	/// <summary>
	/// System wrapper for the Time management functionality.
	/// This manages the global Time class as part of the engine's system architecture.
	/// </summary>
	class TimeSystem : public EngineSystem
	{
	public:
		TimeSystem();
		virtual ~TimeSystem() = default;

		// EngineSystem interface
		virtual Result<void> Initialize() override;
		virtual Result<void> Update() override;
		virtual Result<void> Render() override;
		virtual Result<void> Shutdown() override;

		// Access to the Time class (for convenience, though Time can be accessed directly)
		static float GetTime() { return Time::GetTime(); }
		static float GetDeltaTime() { return Time::GetDeltaTime(); }
		static float GetUnscaledTime() { return Time::GetUnscaledTime(); }
		static float GetUnscaledDeltaTime() { return Time::GetUnscaledDeltaTime(); }
		static uint64_t GetFrameCount() { return Time::GetFrameCount(); }
		static float GetFPS() { return Time::GetFPS(); }
	};
}