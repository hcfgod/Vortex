#pragma once

#include <cstdint>
#include <string>

namespace Vortex 
{
	// Engine-wide error codes
	enum class ErrorCode : uint32_t
	{
		Success = 0,

		// General errors (1-99)
		Unknown = 1,
		InvalidParameter = 2,
		NullPointer = 3,
		OutOfMemory = 4,
		NotImplemented = 5,
		InvalidState = 6,
		Timeout = 7,
		AccessDenied = 8,

		// Engine errors (100-199)
		EngineNotInitialized = 100,
		EngineAlreadyInitialized = 101,
		EngineShutdownFailed = 102,
		SystemInitializationFailed = 103,

		// Renderer errors (200-299)
		RendererInitFailed = 200,
		ShaderCompilationFailed = 201,
		TextureLoadFailed = 202,
		BufferCreationFailed = 203,
		RenderTargetCreationFailed = 204,

		// Audio errors (300-399)
		AudioInitFailed = 300,
		AudioDeviceNotFound = 301,
		SoundLoadFailed = 302,
		AudioPlaybackFailed = 303,

		// Input errors (400-499)
		InputInitFailed = 400,
		ControllerNotFound = 401,
		KeyboardNotFound = 402,
		MouseNotFound = 403,

		// File I/O errors (500-599)
		FileNotFound = 500,
		FileAccessDenied = 501,
		FileCorrupted = 502,
		DirectoryNotFound = 503,
		DiskFull = 504,
		ConfigurationError = 505,

		// Network errors (600-699)
		NetworkInitFailed = 600,
		ConnectionFailed = 601,
		HostNotFound = 602,
		NetworkTimeout = 603,

		// Physics errors (700-799)
		PhysicsInitFailed = 700,
		CollisionDetectionFailed = 701,
		PhysicsSimulationFailed = 702,

		// Resource errors (800-899)
		ResourceNotFound = 800,
		ResourceLoadFailed = 801,
		ResourceCorrupted = 802,
		ResourceOutOfMemory = 803
	};

	// Result type for error handling
	template<typename T>
	class Result
	{
	public:
		Result(T&& value) : m_Value(std::forward<T>(value)), m_ErrorCode(ErrorCode::Success) {}
		Result(const T& value) : m_Value(value), m_ErrorCode(ErrorCode::Success) {}
		Result(ErrorCode errorCode) : m_ErrorCode(errorCode) {}
		Result(ErrorCode errorCode, const std::string& message) : m_ErrorCode(errorCode), m_ErrorMessage(message) {}

		bool IsSuccess() const { return m_ErrorCode == ErrorCode::Success; }
		bool IsError() const { return m_ErrorCode != ErrorCode::Success; }

		ErrorCode GetErrorCode() const { return m_ErrorCode; }
		const std::string& GetErrorMessage() const { return m_ErrorMessage; }

		T& GetValue() { return m_Value; }
		const T& GetValue() const { return m_Value; }

		T& operator*() { return m_Value; }
		const T& operator*() const { return m_Value; }

		T* operator->() { return &m_Value; }
		const T* operator->() const { return &m_Value; }

		explicit operator bool() const { return IsSuccess(); }

	private:
		T m_Value{};
		ErrorCode m_ErrorCode;
		std::string m_ErrorMessage;
	};

	// Specialization for void results
	template<>
	class Result<void>
	{
	public:
		Result() : m_ErrorCode(ErrorCode::Success) {}
		Result(ErrorCode errorCode) : m_ErrorCode(errorCode) {}
		Result(ErrorCode errorCode, const std::string& message) : m_ErrorCode(errorCode), m_ErrorMessage(message) {}

		bool IsSuccess() const { return m_ErrorCode == ErrorCode::Success; }
		bool IsError() const { return m_ErrorCode != ErrorCode::Success; }

		ErrorCode GetErrorCode() const { return m_ErrorCode; }
		const std::string& GetErrorMessage() const { return m_ErrorMessage; }

		explicit operator bool() const { return IsSuccess(); }

	private:
		ErrorCode m_ErrorCode;
		std::string m_ErrorMessage;
	};

	// Helper functions
	const char* ErrorCodeToString(ErrorCode code);
	std::string FormatError(ErrorCode code, const std::string& additionalInfo = "");

	// Macros for easy error handling
	#define VX_RETURN_IF_ERROR(result) \
		do { \
			if ((result).IsError()) \
			{ \
				VX_CORE_ERROR("Error: {0} - {1}", ::Vortex::ErrorCodeToString((result).GetErrorCode()), (result).GetErrorMessage()); \
				return (result); \
			} \
		} while(0)

	#define VX_LOG_ERROR(result) \
		do { \
			if ((result).IsError()) \
			{ \
				VX_CORE_ERROR("Error: {0} - {1}", ::Vortex::ErrorCodeToString((result).GetErrorCode()), (result).GetErrorMessage()); \
			} \
		} while(0)

}