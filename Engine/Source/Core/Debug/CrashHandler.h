#pragma once

#ifdef VX_PLATFORM_WINDOWS
	#include <Windows.h>
	#include <DbgHelp.h>
	#include <psapi.h>
#endif

namespace Vortex 
{
	namespace Debug 
	{
		class CrashHandler
		{
		public:
			static void Initialize();
			static void Shutdown();
			
			static void GenerateCrashReport(const std::string& reason = "Unknown");
			static void WriteMiniDump(const std::string& filename = "");
			
			// Signal handlers
			static void SetupSignalHandlers();

		private:
			static bool s_Initialized;
			static std::string s_CrashLogDirectory;

#ifdef VX_PLATFORM_WINDOWS
			static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
			static void InvalidParameterHandler(const wchar_t* expression, const wchar_t* function, 
				const wchar_t* file, unsigned int line, uintptr_t reserved);
			static void PurecallHandler();
			static void SignalHandler(int signal);
#endif

			static void WriteSystemInfo();
			static void WriteModuleInfo();
		};

		// RAII crash context for scoped crash reporting
		class CrashContext
		{
		public:
			CrashContext(const std::string& context);
			~CrashContext();

			static const std::vector<std::string>& GetContextStack() { return s_ContextStack; }

		private:
			std::string m_Context;
			static thread_local std::vector<std::string> s_ContextStack;
			
			friend class CrashHandler;
		};

	}

}

// Macros for crash handling
#define VX_CRASH_CONTEXT(context) ::Vortex::Debug::CrashContext _crashContext(context)

#ifdef VX_DEBUG
	#define VX_CRASH_IF(condition, message) \
		do { \
			if (condition) \
			{ \
				VX_CORE_CRITICAL("Crash condition met: {0}", message); \
				::Vortex::Debug::CrashHandler::GenerateCrashReport(message); \
				VX_DEBUGBREAK(); \
			} \
		} while(0)
#else
	#define VX_CRASH_IF(condition, message) \
		do { \
			if (condition) \
			{ \
				VX_CORE_CRITICAL("Crash condition met: {0}", message); \
				::Vortex::Debug::CrashHandler::GenerateCrashReport(message); \
			} \
		} while(0)
#endif

#define VX_FORCE_CRASH(message) \
	do { \
		VX_CORE_CRITICAL("Forced crash: {0}", message); \
		::Vortex::Debug::CrashHandler::GenerateCrashReport(message); \
		std::abort(); \
	} while(0)