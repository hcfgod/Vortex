#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <memory>

namespace Vortex 
{
	/**
	 * @brief Configuration for the logging system
	 */
	struct LogConfig
	{
		// General settings
		bool EnableAsync = true;                    // Enable async logging for performance
		size_t AsyncQueueSize = 8192;              // Async queue size (power of 2)
		size_t AsyncThreadCount = 1;               // Number of async logging threads
		spdlog::level::level_enum LogLevel = spdlog::level::info;  // Default log level
		
		// Console logging
		bool EnableConsole = true;                 // Enable console output
		bool ConsoleColors = true;                 // Enable colored console output
		
		// File logging
		bool EnableFileLogging = true;             // Enable file logging
		std::string LogDirectory = "logs";          // Log file directory
		std::string LogFilePattern = "vortex_%Y%m%d_%H%M%S.log";  // Log file name pattern
		
		// Rotation settings
		bool EnableRotation = true;                // Enable log rotation
		size_t MaxFileSize = 10 * 1024 * 1024;    // 10MB max file size
		size_t MaxFiles = 5;                       // Keep last 5 log files
		bool DailyRotation = false;                // Rotate daily instead of by size
		int RotationHour = 0;                      // Hour to rotate daily (0-23)
		int RotationMinute = 0;                    // Minute to rotate daily (0-59)
		
		// Performance settings
		bool AutoFlush = false;                    // Auto flush after each log (impacts performance)
		std::chrono::seconds FlushInterval = std::chrono::seconds(3);  // Flush interval for async
	};

	/**
	 * @brief Advanced logging system with async support and rotation
	 */
	class Log
	{
	public:
		/**
		 * @brief Initialize logging system with default configuration
		 */
		static void Init();
		
		/**
		 * @brief Initialize logging system with custom configuration
		 * @param config Logging configuration
		 */
		static void Init(const LogConfig& config);
		
		/**
		 * @brief Shutdown logging system and flush all pending logs
		 */
		static void Shutdown();
		
		/**
		 * @brief Flush all loggers immediately
		 */
		static void Flush();
		
		/**
		 * @brief Set log level for both loggers
		 * @param level New log level
		 */
		static void SetLevel(spdlog::level::level_enum level);
		
		/**
		 * @brief Get current configuration
		 * @return Current log configuration
		 */
		static const LogConfig& GetConfig() { return s_Config; }
		
		/**
		 * @brief Check if logging system is initialized
		 * @return True if initialized
		 */
		static bool IsInitialized() { return s_Initialized; }

		// Logger accessors - safe versions that check initialization
		static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_Initialized ? s_CoreLogger : nullptr; }
		static std::shared_ptr<spdlog::logger> GetClientLogger() { return s_Initialized ? s_ClientLogger : nullptr; }

	private:
		static void CreateLogDirectory();
		static void SetupAsyncLogging();
		static void CreateLoggers();
		static void SetupFlushPolicy();
		
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
		static LogConfig s_Config;
		static bool s_Initialized;
	};
}

// Core log macros with safe initialization check
#define VX_CORE_TRACE(...)    do { auto logger = ::Vortex::Log::GetCoreLogger(); if (logger) logger->trace(__VA_ARGS__); } while(0)
#define VX_CORE_DEBUG(...)    do { auto logger = ::Vortex::Log::GetCoreLogger(); if (logger) logger->debug(__VA_ARGS__); } while(0)
#define VX_CORE_INFO(...)     do { auto logger = ::Vortex::Log::GetCoreLogger(); if (logger) logger->info(__VA_ARGS__); } while(0)
#define VX_CORE_WARN(...)     do { auto logger = ::Vortex::Log::GetCoreLogger(); if (logger) logger->warn(__VA_ARGS__); } while(0)
#define VX_CORE_ERROR(...)    do { auto logger = ::Vortex::Log::GetCoreLogger(); if (logger) logger->error(__VA_ARGS__); } while(0)
#define VX_CORE_CRITICAL(...) do { auto logger = ::Vortex::Log::GetCoreLogger(); if (logger) logger->critical(__VA_ARGS__); } while(0)

// Client log macros with safe initialization check
#define VX_TRACE(...)         do { auto logger = ::Vortex::Log::GetClientLogger(); if (logger) logger->trace(__VA_ARGS__); } while(0)
#ifndef VX_DEBUG
#define VX_DEBUG(...)         do { auto logger = ::Vortex::Log::GetClientLogger(); if (logger) logger->debug(__VA_ARGS__); } while(0)
#endif
#define VX_INFO(...)          do { auto logger = ::Vortex::Log::GetClientLogger(); if (logger) logger->info(__VA_ARGS__); } while(0)
#define VX_WARN(...)          do { auto logger = ::Vortex::Log::GetClientLogger(); if (logger) logger->warn(__VA_ARGS__); } while(0)
#define VX_ERROR(...)         do { auto logger = ::Vortex::Log::GetClientLogger(); if (logger) logger->error(__VA_ARGS__); } while(0)
#define VX_CRITICAL(...)      do { auto logger = ::Vortex::Log::GetClientLogger(); if (logger) logger->critical(__VA_ARGS__); } while(0)
