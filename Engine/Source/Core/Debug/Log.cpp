#include "vxpch.h"
#include "Log.h"
#include <filesystem>
#include <iostream>

namespace Vortex 
{
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;
	LogConfig Log::s_Config;
	bool Log::s_Initialized = false;

	void Log::Init()
	{
		// Use default configuration
		Init(LogConfig{});
	}

	void Log::Init(const LogConfig& config)
	{
		if (s_Initialized)
		{
			std::cerr << "Log system already initialized!\n";
			return;
		}

		s_Config = config;

		try
		{
			// Create log directory if file logging is enabled
			if (s_Config.EnableFileLogging)
			{
				CreateLogDirectory();
			}

			// Setup async logging if enabled
			if (s_Config.EnableAsync)
			{
				SetupAsyncLogging();
			}

			// Set global spdlog pattern
			spdlog::set_pattern("%^[%T] %n: %v%$");

			// Create loggers
			CreateLoggers();

			// Setup flush policy
			SetupFlushPolicy();

			// Mark as initialized so SetLevel will work
			s_Initialized = true;
			
			// Set log levels
			SetLevel(s_Config.LogLevel);
			VX_CORE_INFO("Vortex Engine logging system initialized");
			VX_CORE_INFO("  Async: {}, File Logging: {}, Rotation: {}", 
				s_Config.EnableAsync ? "ON" : "OFF",
				s_Config.EnableFileLogging ? "ON" : "OFF",
				s_Config.EnableRotation ? "ON" : "OFF");
			if (s_Config.EnableFileLogging)
			{
				VX_CORE_INFO("  Log Directory: {}", s_Config.LogDirectory);
				if (s_Config.EnableRotation)
				{
					if (s_Config.DailyRotation)
					{
						VX_CORE_INFO("  Daily rotation at {}:{:02d}", s_Config.RotationHour, s_Config.RotationMinute);
					}
					else
					{
						VX_CORE_INFO("  Size-based rotation: {} MB, keep {} files", 
							s_Config.MaxFileSize / (1024 * 1024), s_Config.MaxFiles);
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Failed to initialize logging system: " << e.what() << std::endl;
			// Fallback to simple console logging
			s_CoreLogger = spdlog::stdout_color_mt("VORTEX");
			s_ClientLogger = spdlog::stdout_color_mt("APP");
			s_Initialized = true;
		}
	}

	void Log::Shutdown()
	{
		if (!s_Initialized) return;

		VX_CORE_INFO("Shutting down logging system...");

		// Flush all pending logs
		Flush();

		// Drop loggers to release resources
		if (s_CoreLogger) s_CoreLogger.reset();
		if (s_ClientLogger) s_ClientLogger.reset();

		// Shutdown spdlog thread pool if async was enabled
		if (s_Config.EnableAsync)
		{
			spdlog::shutdown();
		}

		s_Initialized = false;
	}

	void Log::Flush()
	{
		if (!s_Initialized) return;

		if (s_CoreLogger) s_CoreLogger->flush();
		if (s_ClientLogger) s_ClientLogger->flush();
	}

	void Log::SetLevel(spdlog::level::level_enum level)
	{
		if (!s_Initialized) {
			std::cerr << "[LOG DEBUG] SetLevel called but logger not initialized yet" << std::endl;
			return;
		}

		s_Config.LogLevel = level;
		
		// Set logger levels
		if (s_CoreLogger) {
			s_CoreLogger->set_level(level);
			// Also set all sink levels - this is crucial!
			auto& sinks = s_CoreLogger->sinks();
			for (auto& sink : sinks) {
				sink->set_level(level);
			}
		}
		
		if (s_ClientLogger) {
			s_ClientLogger->set_level(level);
			// Also set all sink levels - this is crucial!
			auto& sinks = s_ClientLogger->sinks();
			for (auto& sink : sinks) {
				sink->set_level(level);
			}
		}
		
		// Set the global spdlog level so all registered loggers get the same threshold
		spdlog::set_level(level);

		// Ensure every registered logger's sinks adopt the same level for consistent filtering
		spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) {
			for (auto& s : l->sinks()) {
				s->set_level(level);
			}
			l->set_level(level);
		});

		VX_CORE_INFO("Log level set to {}", spdlog::level::to_string_view(level));
	}

	void Log::CreateLogDirectory()
	{
		try
		{
			std::filesystem::create_directories(s_Config.LogDirectory);
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			std::cerr << "Failed to create log directory '" << s_Config.LogDirectory 
					  << "': " << e.what() << std::endl;
			throw;
		}
	}

	void Log::SetupAsyncLogging()
	{
		// Initialize spdlog async thread pool
		// Queue size must be a power of 2
		size_t queue_size = s_Config.AsyncQueueSize;
		// Ensure it's a power of 2
		if ((queue_size & (queue_size - 1)) != 0)
		{
			// Round up to next power of 2
			queue_size = 1;
			while (queue_size < s_Config.AsyncQueueSize)
			{
				queue_size <<= 1;
			}
		}

		spdlog::init_thread_pool(queue_size, s_Config.AsyncThreadCount);
	}

	void Log::CreateLoggers()
	{
		std::vector<spdlog::sink_ptr> sinks;

		// Console sink
		if (s_Config.EnableConsole)
		{
			if (s_Config.ConsoleColors)
			{
				auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
				console_sink->set_pattern("%^[%T] %n: %v%$");
				sinks.push_back(console_sink);
			}
			else
			{
				auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
				console_sink->set_pattern("[%T] %n: %v");
				sinks.push_back(console_sink);
			}
		}

		// File sink with rotation
		if (s_Config.EnableFileLogging)
		{
			if (s_Config.DailyRotation)
			{
				// Daily rotation
				std::string filepath = s_Config.LogDirectory + "/vortex.log";
				auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
					filepath, s_Config.RotationHour, s_Config.RotationMinute);
				file_sink->set_pattern("[%T] [%l] %n: %v");
				sinks.push_back(file_sink);
			}
			else if (s_Config.EnableRotation)
			{
				// Size-based rotation
				std::string filepath = s_Config.LogDirectory + "/vortex.log";
				auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
					filepath, s_Config.MaxFileSize, s_Config.MaxFiles);
				file_sink->set_pattern("[%T] [%l] %n: %v");
				sinks.push_back(file_sink);
			}
			else
			{
				// Simple file sink without rotation
				std::string filepath = s_Config.LogDirectory + "/vortex.log";
				auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath);
				file_sink->set_pattern("[%T] [%l] %n: %v");
				sinks.push_back(file_sink);
			}
		}

		// Create loggers
		if (s_Config.EnableAsync)
		{
			// Async loggers
			s_CoreLogger = std::make_shared<spdlog::async_logger>(
				"VORTEX", sinks.begin(), sinks.end(),
				spdlog::thread_pool(), spdlog::async_overflow_policy::block);

			s_ClientLogger = std::make_shared<spdlog::async_logger>(
				"APP", sinks.begin(), sinks.end(),
				spdlog::thread_pool(), spdlog::async_overflow_policy::block);
		}
		else
		{
			// Synchronous loggers
			s_CoreLogger = std::make_shared<spdlog::logger>("VORTEX", sinks.begin(), sinks.end());
			s_ClientLogger = std::make_shared<spdlog::logger>("APP", sinks.begin(), sinks.end());
		}

		// Register loggers with spdlog
		spdlog::register_logger(s_CoreLogger);
		spdlog::register_logger(s_ClientLogger);
		// Route spdlog's default logger through our core logger so direct spdlog calls behave consistently
		spdlog::set_default_logger(s_CoreLogger);
	}

	void Log::SetupFlushPolicy()
	{
		if (s_Config.AutoFlush)
		{
			// Flush after every log (impacts performance)
			if (s_CoreLogger) s_CoreLogger->flush_on(spdlog::level::trace);
			if (s_ClientLogger) s_ClientLogger->flush_on(spdlog::level::trace);
		}
		else
		{
			// Flush only on errors and critical messages
			if (s_CoreLogger) s_CoreLogger->flush_on(spdlog::level::err);
			if (s_ClientLogger) s_ClientLogger->flush_on(spdlog::level::err);
		}

		// Set periodic flush for async logging
		if (s_Config.EnableAsync)
		{
			spdlog::flush_every(s_Config.FlushInterval);
		}
	}
}
