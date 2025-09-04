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

		// Logger accessors
		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

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

// Core log macros
#define VX_CORE_TRACE(...)    ::Vortex::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VX_CORE_DEBUG(...)    ::Vortex::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define VX_CORE_INFO(...)     ::Vortex::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VX_CORE_WARN(...)     ::Vortex::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VX_CORE_ERROR(...)    ::Vortex::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VX_CORE_CRITICAL(...) ::Vortex::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define VX_TRACE(...)         ::Vortex::Log::GetClientLogger()->trace(__VA_ARGS__)
#ifndef VX_DEBUG
#define VX_DEBUG(...)         ::Vortex::Log::GetClientLogger()->debug(__VA_ARGS__)
#endif
#define VX_INFO(...)          ::Vortex::Log::GetClientLogger()->info(__VA_ARGS__)
#define VX_WARN(...)          ::Vortex::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VX_ERROR(...)         ::Vortex::Log::GetClientLogger()->error(__VA_ARGS__)
#define VX_CRITICAL(...)      ::Vortex::Log::GetClientLogger()->critical(__VA_ARGS__)
