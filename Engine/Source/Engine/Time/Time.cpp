#include "vxpch.h"
#include "Time.h"
#include "Core/Debug/Log.h"
#include "Core/Debug/Assert.h"

namespace Vortex 
{
	// Static member definitions
	std::chrono::high_resolution_clock::time_point Time::s_StartTime;
	std::chrono::high_resolution_clock::time_point Time::s_LastFrameTime;
	std::chrono::high_resolution_clock::time_point Time::s_CurrentFrameTime;

	float Time::s_Time = 0.0f;
	float Time::s_UnscaledTime = 0.0f;
	float Time::s_DeltaTime = 0.0f;
	float Time::s_UnscaledDeltaTime = 0.0f;
	float Time::s_TimeScale = 1.0f;
	float Time::s_MaxDeltaTime = 0.1f;  // 10 FPS minimum
	float Time::s_FixedDeltaTime = 1.0f / 60.0f;  // 60 FPS physics

	uint64_t Time::s_FrameCount = 0;
	float Time::s_FPS = 0.0f;
	std::chrono::high_resolution_clock::time_point Time::s_LastFPSUpdate;
	uint32_t Time::s_FramesSinceLastFPSUpdate = 0;

	bool Time::s_Initialized = false;

	void Time::Initialize()
	{
		VX_CORE_ASSERT(!s_Initialized, "Time system is already initialized");

		// Record the start time
		s_StartTime = std::chrono::high_resolution_clock::now();
		s_LastFrameTime = s_StartTime;
		s_CurrentFrameTime = s_StartTime;
		s_LastFPSUpdate = s_StartTime;

		// Reset all values
		s_Time = 0.0f;
		s_UnscaledTime = 0.0f;
		s_DeltaTime = 0.0f;
		s_UnscaledDeltaTime = 0.0f;
		s_FrameCount = 0;
		s_FPS = 0.0f;
		s_FramesSinceLastFPSUpdate = 0;

		s_Initialized = true;
	}

	void Time::Update()
	{
		VX_CORE_ASSERT(s_Initialized, "Time system must be initialized before updating");

		// Update timing
		s_LastFrameTime = s_CurrentFrameTime;
		s_CurrentFrameTime = std::chrono::high_resolution_clock::now();

		// Calculate unscaled delta time
		s_UnscaledDeltaTime = std::chrono::duration<float>(s_CurrentFrameTime - s_LastFrameTime).count();

		// Clamp delta time to prevent huge jumps (e.g., during debugging pauses)
		s_UnscaledDeltaTime = std::min(s_UnscaledDeltaTime, s_MaxDeltaTime);

		// Apply time scale
		s_DeltaTime = s_UnscaledDeltaTime * s_TimeScale;

		// Update accumulated time
		s_UnscaledTime = std::chrono::duration<float>(s_CurrentFrameTime - s_StartTime).count();
		s_Time += s_DeltaTime;

		// Update frame count
		s_FrameCount++;

		// Update FPS calculation
		UpdateFPS();
	}

	void Time::Shutdown()
	{
		if (!s_Initialized)
		{
			return;
		}

		// Log final timing statistics
		VX_CORE_INFO("Runtime: {:.2f}s | Frames: {} | Avg FPS: {:.1f}", 
			s_UnscaledTime, s_FrameCount, s_FrameCount / s_UnscaledTime);

		s_Initialized = false;
	}

	uint64_t Time::GetHighResolutionTimestamp()
	{
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	}

	double Time::NanosecondsToSeconds(uint64_t nanoseconds)
	{
		return static_cast<double>(nanoseconds) / 1000000000.0;
	}

	void Time::LogTimingInfo()
	{
		VX_CORE_INFO("=== Time System Status ===");
		VX_CORE_INFO("  Initialized: {}", s_Initialized ? "Yes" : "No");
		VX_CORE_INFO("  Time: {:.6f}s", s_Time);
		VX_CORE_INFO("  Unscaled Time: {:.6f}s", s_UnscaledTime);
		VX_CORE_INFO("  Delta Time: {:.6f}s ({:.2f}ms)", s_DeltaTime, s_DeltaTime * 1000.0f);
		VX_CORE_INFO("  Unscaled Delta Time: {:.6f}s ({:.2f}ms)", s_UnscaledDeltaTime, s_UnscaledDeltaTime * 1000.0f);
		VX_CORE_INFO("  Time Scale: {:.3f}", s_TimeScale);
		VX_CORE_INFO("  Max Delta Time: {:.3f}s", s_MaxDeltaTime);
		VX_CORE_INFO("  Fixed Delta Time: {:.6f}s ({:.2f} FPS)", s_FixedDeltaTime, 1.0f / s_FixedDeltaTime);
		VX_CORE_INFO("  Frame Count: {}", s_FrameCount);
		VX_CORE_INFO("  Current FPS: {:.2f}", s_FPS);
		VX_CORE_INFO("  Paused: {}", IsPaused() ? "Yes" : "No");
		VX_CORE_INFO("=========================");
	}

	void Time::UpdateFPS()
	{
		s_FramesSinceLastFPSUpdate++;

		// Update FPS every second
		auto timeSinceLastUpdate = std::chrono::duration<float>(s_CurrentFrameTime - s_LastFPSUpdate).count();
		if (timeSinceLastUpdate >= 1.0f)
		{
			s_FPS = static_cast<float>(s_FramesSinceLastFPSUpdate) / timeSinceLastUpdate;
			s_LastFPSUpdate = s_CurrentFrameTime;
			s_FramesSinceLastFPSUpdate = 0;
		}
	}
}