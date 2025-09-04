#include "vxpch.h"
#include "Assert.h"

#ifdef VX_PLATFORM_WINDOWS
	#include <DbgHelp.h>
	#include <TlHelp32.h>
	#pragma comment(lib, "dbghelp.lib")
#endif

namespace Vortex 
{
	namespace Debug 
	{
		void CaptureStackTrace(std::vector<std::string>& stackTrace, int maxFrames)
		{
			stackTrace.clear();

#ifdef VX_PLATFORM_WINDOWS
			// Initialize symbol handler
			HANDLE process = GetCurrentProcess();
			// Load line info and demangled names where possible
			SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
			BOOL symInitialized = SymInitialize(process, NULL, TRUE);

			// Capture the stack (cap to our local array size)
			void* stack[64];
			int framesToCapture = (maxFrames > 0 && maxFrames <= 64) ? maxFrames : 64;
			USHORT numberOfFrames = CaptureStackBackTrace(0, (ULONG)framesToCapture, stack, NULL);

			// Get symbol information (optional)
			SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
			if (symbol)
			{
				symbol->MaxNameLen = 255;
				symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			}

			for (int i = 0; i < numberOfFrames; i++)
			{
				DWORD64 address = (DWORD64)(stack[i]);
				
				std::string frameInfo = "0x" + std::to_string(address);
				
				// Resolve symbol name if available
				if (symbol && symInitialized && SymFromAddr(process, address, 0, symbol))
				{
					frameInfo += " " + std::string(symbol->Name);
					
					// Try to get line information
					IMAGEHLP_LINE64 line{};
					line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
					DWORD displacement = 0;
					
					if (SymGetLineFromAddr64(process, address, &displacement, &line))
					{
						frameInfo += " at " + std::string(line.FileName) + ":" + std::to_string(line.LineNumber);
					}
				}
				
				stackTrace.push_back(frameInfo);
			}

			if (symbol) free(symbol);
			if (symInitialized) SymCleanup(process);
#else
			stackTrace.push_back("Stack trace not implemented for this platform");
#endif
		}

		void PrintStackTrace(const std::vector<std::string>& stackTrace)
		{
			VX_CORE_ERROR("=== STACK TRACE ===");
			for (size_t i = 0; i < stackTrace.size(); ++i)
			{
				VX_CORE_ERROR("  [{0}] {1}", i, stackTrace[i]);
			}
			VX_CORE_ERROR("=== END STACK TRACE ===");
		}

		bool AssertionHandler(const char* expression, const char* message, const char* file, int line)
		{
			VX_CORE_CRITICAL("=== ASSERTION FAILED ===");
			VX_CORE_CRITICAL("Expression: {0}", expression);
			VX_CORE_CRITICAL("Message: {0}", message ? message : "No message provided");
			VX_CORE_CRITICAL("File: {0}", file);
			VX_CORE_CRITICAL("Line: {0}", line);

			// Capture and print stack trace
			std::vector<std::string> stackTrace;
			CaptureStackTrace(stackTrace);
			PrintStackTrace(stackTrace);

			VX_CORE_CRITICAL("=== END ASSERTION ===");

			// In debug builds, break into debugger
			#ifdef VX_DEBUG
				return true; // Break into debugger
			#else
				return false; // Don't break in release
			#endif
		}

	}
}