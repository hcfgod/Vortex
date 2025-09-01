#pragma once

#include "vxpch.h"
#include <chrono>

namespace Vortex 
{
	/// <summary>
	/// Unity-style Time class providing global access to timing information.
	/// This is a static class that can be accessed from anywhere in the engine.
	/// </summary>
	class Time
	{
	public:
		// No construction - static class only
		Time() = delete;
		~Time() = delete;

		/// <summary>
		/// Initialize the time system. Should be called once at engine startup.
		/// </summary>
		static void Initialize();

		/// <summary>
		/// Update the time system. Should be called once per frame.
		/// </summary>
		static void Update();

		/// <summary>
		/// Shutdown the time system.
		/// </summary>
		static void Shutdown();

		// ===== UNITY-STYLE TIME PROPERTIES =====

		/// <summary>
		/// The time in seconds since the engine started (read-only).
		/// </summary>
		static float GetTime() { return s_Time; }

		/// <summary>
		/// The time in seconds it took to complete the last frame (read-only).
		/// </summary>
		static float GetDeltaTime() { return s_DeltaTime; }

		/// <summary>
		/// The time in seconds since the engine started, but using unscaled time (read-only).
		/// This is not affected by Time.timeScale.
		/// </summary>
		static float GetUnscaledTime() { return s_UnscaledTime; }

		/// <summary>
		/// The unscaled time in seconds it took to complete the last frame (read-only).
		/// This is not affected by Time.timeScale.
		/// </summary>
		static float GetUnscaledDeltaTime() { return s_UnscaledDeltaTime; }

		/// <summary>
		/// The time scale at which time is passing. This can be used for slow motion effects.
		/// Default is 1.0. Setting to 0 pauses time, 0.5 is half speed, 2.0 is double speed.
		/// </summary>
		static float GetTimeScale() { return s_TimeScale; }
		static void SetTimeScale(float scale) { s_TimeScale = std::max(0.0f, scale); }

		/// <summary>
		/// The total number of frames that have passed (read-only).
		/// </summary>
		static uint64_t GetFrameCount() { return s_FrameCount; }

		/// <summary>
		/// The current frames per second (averaged over recent frames).
		/// </summary>
		static float GetFPS() { return s_FPS; }

		/// <summary>
		/// The maximum delta time allowed. Prevents huge time jumps.
		/// Default is 0.1 seconds (10 FPS minimum).
		/// </summary>
		static float GetMaxDeltaTime() { return s_MaxDeltaTime; }
		static void SetMaxDeltaTime(float maxDelta) { s_MaxDeltaTime = std::max(0.0f, maxDelta); }

		/// <summary>
		/// Fixed timestep for physics updates. Default is 1/60 seconds.
		/// </summary>
		static float GetFixedDeltaTime() { return s_FixedDeltaTime; }
		static void SetFixedDeltaTime(float fixedDelta) { s_FixedDeltaTime = std::max(0.001f, fixedDelta); }

		/// <summary>
		/// Returns true if the game is paused (timeScale = 0).
		/// </summary>
		static bool IsPaused() { return s_TimeScale == 0.0f; }

		/// <summary>
		/// Pause the game (sets timeScale to 0).
		/// </summary>
		static void Pause() { s_TimeScale = 0.0f; }

		/// <summary>
		/// Resume the game (sets timeScale to 1).
		/// </summary>
		static void Resume() { s_TimeScale = 1.0f; }

		// ===== DEBUGGING AND PROFILING =====

		/// <summary>
		/// Get the current high-resolution timestamp in nanoseconds.
		/// Useful for profiling and precise timing measurements.
		/// </summary>
		static uint64_t GetHighResolutionTimestamp();

		/// <summary>
		/// Convert nanoseconds to seconds.
		/// </summary>
		static double NanosecondsToSeconds(uint64_t nanoseconds);

		/// <summary>
		/// Get detailed timing information for debugging.
		/// </summary>
		static void LogTimingInfo();

	private:
		// Core timing
		static std::chrono::high_resolution_clock::time_point s_StartTime;
		static std::chrono::high_resolution_clock::time_point s_LastFrameTime;
		static std::chrono::high_resolution_clock::time_point s_CurrentFrameTime;

		// Time values
		static float s_Time;                // Scaled time since start
		static float s_UnscaledTime;        // Unscaled time since start
		static float s_DeltaTime;           // Scaled delta time
		static float s_UnscaledDeltaTime;   // Unscaled delta time
		static float s_TimeScale;           // Time scale multiplier
		static float s_MaxDeltaTime;        // Maximum allowed delta time
		static float s_FixedDeltaTime;      // Fixed timestep for physics

		// Frame tracking
		static uint64_t s_FrameCount;       // Total frames rendered
		static float s_FPS;                 // Current FPS
		static std::chrono::high_resolution_clock::time_point s_LastFPSUpdate;
		static uint32_t s_FramesSinceLastFPSUpdate;

		// State
		static bool s_Initialized;

		// Internal helpers
		static void UpdateFPS();
	};
}