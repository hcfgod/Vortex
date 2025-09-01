#pragma once

#include "vxpch.h"
#include "Log.h"

namespace Vortex
{
	namespace Debug
	{

		// Stack trace capture
		void CaptureStackTrace(std::vector<std::string>& stackTrace, int maxFrames = 64);
		void PrintStackTrace(const std::vector<std::string>& stackTrace);

		// Assertion handler
		bool AssertionHandler(const char* expression, const char* message, const char* file, int line);
	}
}

// Assertion levels
#ifdef VX_DEBUG
#define VX_ENABLE_ASSERTS
#endif

#ifdef VX_ENABLE_ASSERTS
#define VX_ASSERT(x, ...) \
		{ \
			if (!(x)) \
			{ \
				VX_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); \
				if (::Vortex::Debug::AssertionHandler(#x, __VA_ARGS__, __FILE__, __LINE__)) \
					VX_DEBUGBREAK(); \
			} \
		}

#define VX_CORE_ASSERT(x, ...) \
		{ \
			if (!(x)) \
			{ \
				VX_CORE_CRITICAL("Core Assertion Failed: {0}", __VA_ARGS__); \
				if (::Vortex::Debug::AssertionHandler(#x, __VA_ARGS__, __FILE__, __LINE__)) \
					VX_DEBUGBREAK(); \
			} \
		}

	// Verify macros (always enabled, even in release)
#define VX_VERIFY(x, ...) \
		{ \
			if (!(x)) \
			{ \
				VX_CORE_ERROR("Verification Failed: {0}", __VA_ARGS__); \
				::Vortex::Debug::AssertionHandler(#x, __VA_ARGS__, __FILE__, __LINE__); \
			} \
		}

	// Fatal assertion (always crashes)
#define VX_FATAL_ASSERT(x, ...) \
		{ \
			if (!(x)) \
			{ \
				VX_CORE_CRITICAL("Fatal Assertion Failed: {0}", __VA_ARGS__); \
				::Vortex::Debug::AssertionHandler(#x, __VA_ARGS__, __FILE__, __LINE__); \
				std::abort(); \
			} \
		}
#else
#define VX_ASSERT(x, ...)
#define VX_CORE_ASSERT(x, ...)
#define VX_VERIFY(x, ...) { if (!(x)) { VX_CORE_ERROR("Verification Failed: {0}", __VA_ARGS__); } }
#define VX_FATAL_ASSERT(x, ...) { if (!(x)) { VX_CORE_CRITICAL("Fatal Error: {0}", __VA_ARGS__); std::abort(); } }
#endif

// Static assertions
#define VX_STATIC_ASSERT(x, msg) static_assert(x, msg)

// Null pointer checks
#define VX_CHECK_NULL(ptr, ...) VX_ASSERT(ptr != nullptr, __VA_ARGS__)
#define VX_CORE_CHECK_NULL(ptr, ...) VX_CORE_ASSERT(ptr != nullptr, __VA_ARGS__)

// Bounds checking
#define VX_CHECK_BOUNDS(index, size, ...) VX_ASSERT(index >= 0 && index < size, __VA_ARGS__)

// Hardware breakpoint macro
#ifdef VX_PLATFORM_WINDOWS
#define VX_DEBUGBREAK() __debugbreak()
#else
#define VX_DEBUGBREAK() __builtin_trap()
#endif