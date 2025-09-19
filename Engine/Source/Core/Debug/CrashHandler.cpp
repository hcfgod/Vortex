#include "vxpch.h"
#include "CrashHandler.h"
#include "Assert.h"
#include "Log.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <filesystem>

#ifdef VX_PLATFORM_WINDOWS
#include <signal.h>
#include <crtdbg.h>
#include <psapi.h>
#include <sysinfoapi.h>
#include <intrin.h>
#pragma comment(lib, "psapi.lib")
#elif defined(VX_PLATFORM_LINUX)
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <fstream>
#elif defined(VX_PLATFORM_MACOS)
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#endif

namespace Vortex
{
	namespace Debug
	{
		bool CrashHandler::s_Initialized = false;
		std::string CrashHandler::s_CrashLogDirectory = "Logs/Crashes/";
		thread_local std::vector<std::string> CrashContext::s_ContextStack;

		// Helper function to get current timestamp as string
		std::string GetCurrentTimestamp()
		{
			auto now = std::chrono::system_clock::now();
			auto time_t = std::chrono::system_clock::to_time_t(now);
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
				now.time_since_epoch()) % 1000;

			std::stringstream ss;
			ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
			ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
			return ss.str();
		}

		// Platform-agnostic helper functions for system information
		std::string GetOperatingSystemName()
		{
#ifdef VX_PLATFORM_WINDOWS
			return "Windows";
#elif defined(VX_PLATFORM_LINUX)
			return "Linux";
#elif defined(VX_PLATFORM_MACOS)
			return "macOS";
#else
			return "Unknown";
#endif
		}

		std::string GetOperatingSystemVersion()
		{
#ifdef VX_PLATFORM_WINDOWS
			OSVERSIONINFOEX osvi;
			ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
			
			if (GetVersionEx((OSVERSIONINFO*)&osvi))
			{
				std::stringstream ss;
				ss << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << "." << osvi.dwBuildNumber;
				return ss.str();
			}
			return "Unknown";
#elif defined(VX_PLATFORM_LINUX) || defined(VX_PLATFORM_MACOS)
			struct utsname uts;
			if (uname(&uts) == 0)
			{
				return std::string(uts.release) + " (" + uts.version + ")";
			}
			return "Unknown";
#else
			return "Unknown";
#endif
		}

		std::string GetArchitecture()
		{
#ifdef VX_PLATFORM_WINDOWS
#ifdef _WIN64
			return "x64";
#else
			return "x86";
#endif
#elif defined(VX_PLATFORM_LINUX) || defined(VX_PLATFORM_MACOS)
			struct utsname uts;
			if (uname(&uts) == 0)
			{
				return uts.machine;
			}
			return "Unknown";
#else
			return "Unknown";
#endif
		}

		std::string GetProcessorInfo()
		{
#ifdef VX_PLATFORM_WINDOWS
			int cpuInfo[4] = {-1};
			char cpuBrandString[0x40] = {0};
			
			__cpuid(cpuInfo, 0x80000000);
			unsigned int nExIds = cpuInfo[0];
			
			memset(cpuBrandString, 0, sizeof(cpuBrandString));
			
			for (unsigned int i = 0x80000000; i <= nExIds; ++i)
			{
				__cpuid(cpuInfo, i);
				if (i == 0x80000002)
					memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
				else if (i == 0x80000003)
					memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
				else if (i == 0x80000004)
					memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
			}
			
			return std::string(cpuBrandString);
#elif defined(VX_PLATFORM_LINUX)
			std::ifstream cpuinfo("/proc/cpuinfo");
			std::string line;
			while (std::getline(cpuinfo, line))
			{
				if (line.find("model name") != std::string::npos)
				{
					size_t pos = line.find(":");
					if (pos != std::string::npos)
					{
						return line.substr(pos + 2);
					}
				}
			}
			return "Unknown";
#elif defined(VX_PLATFORM_MACOS)
			// For macOS, we can use sysctl to get CPU info
			size_t size = 0;
			sysctlbyname("machdep.cpu.brand_string", nullptr, &size, nullptr, 0);
			if (size > 0)
			{
				std::string brand(size, '\0');
				sysctlbyname("machdep.cpu.brand_string", &brand[0], &size, nullptr, 0);
				return brand;
			}
			return "Unknown";
#else
			return "Unknown";
#endif
		}

		std::string GetMemoryInfo()
		{
#ifdef VX_PLATFORM_WINDOWS
			MEMORYSTATUSEX memInfo;
			memInfo.dwLength = sizeof(MEMORYSTATUSEX);
			if (GlobalMemoryStatusEx(&memInfo))
			{
				std::stringstream ss;
				ss << "Total: " << (memInfo.ullTotalPhys / (1024 * 1024)) << " MB, ";
				ss << "Available: " << (memInfo.ullAvailPhys / (1024 * 1024)) << " MB";
				return ss.str();
			}
			return "Unknown";
#elif defined(VX_PLATFORM_LINUX)
			std::ifstream meminfo("/proc/meminfo");
			std::string line;
			long totalKB = 0, availableKB = 0;
			
			while (std::getline(meminfo, line))
			{
				if (line.find("MemTotal:") == 0)
				{
					sscanf(line.c_str(), "MemTotal: %ld kB", &totalKB);
				}
				else if (line.find("MemAvailable:") == 0)
				{
					sscanf(line.c_str(), "MemAvailable: %ld kB", &availableKB);
				}
			}
			
			if (totalKB > 0)
			{
				std::stringstream ss;
				ss << "Total: " << (totalKB / 1024) << " MB, ";
				ss << "Available: " << (availableKB / 1024) << " MB";
				return ss.str();
			}
			return "Unknown";
#elif defined(VX_PLATFORM_MACOS)
			vm_statistics64_data_t vmStats;
			mach_msg_type_number_t infoCount = HOST_VM_INFO64_COUNT;
			if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vmStats, &infoCount) == KERN_SUCCESS)
			{
				uint64_t totalMem = 0;
				size_t size = sizeof(totalMem);
				sysctlbyname("hw.memsize", &totalMem, &size, nullptr, 0);
				
				std::stringstream ss;
				ss << "Total: " << (totalMem / (1024 * 1024)) << " MB, ";
				ss << "Available: " << ((vmStats.free_count + vmStats.inactive_count) * vm_page_size / (1024 * 1024)) << " MB";
				return ss.str();
			}
			return "Unknown";
#else
			return "Unknown";
#endif
		}

		// Helper functions for module information
		std::string GetExecutablePath()
		{
#ifdef VX_PLATFORM_WINDOWS
			char path[MAX_PATH];
			if (GetModuleFileNameA(nullptr, path, MAX_PATH) != 0)
			{
				return std::filesystem::path(path).string();
			}
			return "Unknown";
#elif defined(VX_PLATFORM_LINUX) || defined(VX_PLATFORM_MACOS)
			char path[1024];
			ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
			if (len != -1)
			{
				path[len] = '\0';
				return std::string(path);
			}
			return "Unknown";
#else
			return "Unknown";
#endif
		}

		std::string GetExecutableSize()
		{
			try
			{
				std::string exePath = GetExecutablePath();
				if (exePath != "Unknown")
				{
					auto fileSize = std::filesystem::file_size(exePath);
					std::stringstream ss;
					ss << (fileSize / (1024 * 1024)) << " MB";
					return ss.str();
				}
			}
			catch (...)
			{
				// Ignore filesystem errors during crash reporting
			}
			return "Unknown";
		}

		std::string GetWorkingDirectory()
		{
			try
			{
				return std::filesystem::current_path().string();
			}
			catch (...)
			{
				return "Unknown";
			}
		}

		int GetProcessId()
		{
#ifdef VX_PLATFORM_WINDOWS
			return GetCurrentProcessId();
#elif defined(VX_PLATFORM_LINUX) || defined(VX_PLATFORM_MACOS)
			return getpid();
#else
			return -1;
#endif
		}

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
			VX_CORE_CRITICAL("Time: {0}", GetCurrentTimestamp());

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
			VX_CORE_INFO("Operating System: {0} {1}", GetOperatingSystemName(), GetOperatingSystemVersion());
			VX_CORE_INFO("Architecture: {0}", GetArchitecture());
			VX_CORE_INFO("Processor: {0}", GetProcessorInfo());
			VX_CORE_INFO("Memory: {0}", GetMemoryInfo());
			VX_CORE_INFO("Thread Count: {0}", std::thread::hardware_concurrency());
			VX_CORE_INFO("=== END SYSTEM INFORMATION ===");
		}

		void CrashHandler::WriteModuleInfo()
		{
			VX_CORE_INFO("=== MODULE INFORMATION ===");
			VX_CORE_INFO("Executable Path: {0}", GetExecutablePath());
			VX_CORE_INFO("Executable Size: {0}", GetExecutableSize());
			VX_CORE_INFO("Working Directory: {0}", GetWorkingDirectory());
			VX_CORE_INFO("Process ID: {0}", GetProcessId());
			
			#ifdef VX_PLATFORM_WINDOWS
			// Additional Windows-specific module info
			HMODULE hMods[1024];
			DWORD cbNeeded;
			if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded))
			{
				VX_CORE_INFO("Loaded Modules: {0}", cbNeeded / sizeof(HMODULE));
				for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)) && i < 5; i++) // Show first 5 modules
				{
					char szModName[MAX_PATH];
					if (GetModuleFileNameExA(GetCurrentProcess(), hMods[i], szModName, sizeof(szModName)))
					{
						std::string moduleName = std::filesystem::path(szModName).filename().string();
						VX_CORE_INFO("  - {0}", moduleName);
					}
				}
				if ((cbNeeded / sizeof(HMODULE)) > 5)
				{
					VX_CORE_INFO("  ... and {0} more modules", (cbNeeded / sizeof(HMODULE)) - 5);
				}
			}
			#endif

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