#include "vxpch.h"
#include "CrashHandler.h"
#include "Assert.h"
#include "Log.h"

#ifdef VX_PLATFORM_WINDOWS
#include <signal.h>
#include <crtdbg.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/stat.h>
#include <signal.h>
#endif

namespace Vortex
{
	namespace Debug
	{
		bool CrashHandler::s_Initialized = false;
		std::string CrashHandler::s_CrashLogDirectory = "Logs/Crashes/";
		thread_local std::vector<std::string> CrashContext::s_ContextStack;

		void CrashHandler::Initialize()
		{
			if (s_Initialized)
				return;

			VX_CORE_INFO("Initializing Crash Handler...");

			// Create crash log directory
#ifdef VX_PLATFORM_WINDOWS
			CreateDirectoryA(s_CrashLogDirectory.c_str(), NULL);
#else
			mkdir(s_CrashLogDirectory.c_str(), 0755);
#endif

			SetupSignalHandlers();

#ifdef VX_PLATFORM_WINDOWS
			// Set unhandled exception filter
			SetUnhandledExceptionFilter(UnhandledExceptionFilter);

			// Set invalid parameter handler
			_set_invalid_parameter_handler(InvalidParameterHandler);

			// Set pure call handler
			_set_purecall_handler(PurecallHandler);

			// Disable Windows error reporting dialog
			SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif

			s_Initialized = true;
			VX_CORE_INFO("Crash Handler initialized successfully");
		}

		void CrashHandler::Shutdown()
		{
			if (!s_Initialized)
				return;

			VX_CORE_INFO("Shutting down Crash Handler...");
			s_Initialized = false;
		}

		void CrashHandler::GenerateCrashReport(const std::string& reason)
		{
			VX_CORE_CRITICAL("=== CRASH REPORT ===");
			VX_CORE_CRITICAL("Reason: {0}", reason);
			VX_CORE_CRITICAL("Time: {0}", "TODO: Add timestamp");

			// Write system information
			WriteSystemInfo();
			WriteModuleInfo();

			// Write context stack
			const auto& contextStack = CrashContext::GetContextStack();
			if (!contextStack.empty())
			{
				VX_CORE_CRITICAL("=== CRASH CONTEXT ===");
				for (const auto& context : contextStack)
				{
					VX_CORE_CRITICAL("  {0}", context);
				}
			}

			// Generate minidump
			WriteMiniDump();

			VX_CORE_CRITICAL("=== END CRASH REPORT ===");
		}

		void CrashHandler::WriteMiniDump(const std::string& filename)
		{
#ifdef VX_PLATFORM_WINDOWS
			std::string dumpFilename = filename;
			if (dumpFilename.empty())
			{
				// Generate filename with timestamp
				dumpFilename = s_CrashLogDirectory + "crash_dump.dmp";
			}

			HANDLE hFile = CreateFileA(dumpFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
				exceptionInfo.ThreadId = GetCurrentThreadId();
				exceptionInfo.ExceptionPointers = NULL;
				exceptionInfo.ClientPointers = FALSE;

				MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
					MiniDumpWithIndirectlyReferencedMemory |
					MiniDumpWithDataSegs |
					MiniDumpWithThreadInfo |
					MiniDumpWithModuleHeaders
					);

				if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpType,
					NULL, NULL, NULL))
				{
					VX_CORE_INFO("Minidump written to: {0}", dumpFilename);
				}
				else
				{
					VX_CORE_ERROR("Failed to write minidump");
				}

				CloseHandle(hFile);
			}
#endif
		}

		void CrashHandler::SetupSignalHandlers()
		{
#ifdef VX_PLATFORM_WINDOWS
			signal(SIGABRT, SignalHandler);
			signal(SIGFPE, SignalHandler);
			signal(SIGILL, SignalHandler);
			signal(SIGINT, SignalHandler);
			signal(SIGSEGV, SignalHandler);
			signal(SIGTERM, SignalHandler);
#endif
		}

#ifdef VX_PLATFORM_WINDOWS
		LONG WINAPI CrashHandler::UnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
		{
			VX_CORE_CRITICAL("Unhandled exception caught!");
			GenerateCrashReport("Unhandled Exception");
			return EXCEPTION_EXECUTE_HANDLER;
		}

		void CrashHandler::InvalidParameterHandler(const wchar_t* expression, const wchar_t* function,
			const wchar_t* file, unsigned int line, uintptr_t reserved)
		{
			VX_CORE_CRITICAL("Invalid parameter detected!");
			GenerateCrashReport("Invalid Parameter");
		}

		void CrashHandler::PurecallHandler()
		{
			VX_CORE_CRITICAL("Pure virtual function call detected!");
			GenerateCrashReport("Pure Virtual Function Call");
		}

		void CrashHandler::SignalHandler(int signal)
		{
			std::string signalName = "Unknown Signal";
			switch (signal)
			{
			case SIGABRT: signalName = "SIGABRT"; break;
			case SIGFPE: signalName = "SIGFPE"; break;
			case SIGILL: signalName = "SIGILL"; break;
			case SIGINT: signalName = "SIGINT"; break;
			case SIGSEGV: signalName = "SIGSEGV"; break;
			case SIGTERM: signalName = "SIGTERM"; break;
			}

			VX_CORE_CRITICAL("Signal caught: {0}", signalName);
			GenerateCrashReport("Signal: " + signalName);
		}
#endif

		void CrashHandler::WriteSystemInfo()
		{
			VX_CORE_INFO("=== SYSTEM INFORMATION ===");
			// TODO: Implement system info gathering
			VX_CORE_INFO("OS: Windows"); // Placeholder
			VX_CORE_INFO("=== END SYSTEM INFORMATION ===");
		}

		void CrashHandler::WriteModuleInfo()
		{
			VX_CORE_INFO("=== MODULE INFORMATION ===");
			// TODO: Implement module info gathering
			VX_CORE_INFO("=== END MODULE INFORMATION ===");
		}

		// CrashContext implementation
		CrashContext::CrashContext(const std::string& context) : m_Context(context)
		{
			s_ContextStack.push_back(context);
		}

		CrashContext::~CrashContext()
		{
			if (!s_ContextStack.empty())
			{
				s_ContextStack.pop_back();
			}
		}

	}
}