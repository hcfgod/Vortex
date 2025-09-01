#include "vxpch.h"
#include "ErrorCodes.h"

namespace Vortex 
{
	const char* ErrorCodeToString(ErrorCode code)
	{
		switch (code)
		{
			case ErrorCode::Success: return "Success";
			
			// General errors
			case ErrorCode::Unknown: return "Unknown Error";
			case ErrorCode::InvalidParameter: return "Invalid Parameter";
			case ErrorCode::NullPointer: return "Null Pointer";
			case ErrorCode::OutOfMemory: return "Out of Memory";
			case ErrorCode::NotImplemented: return "Not Implemented";
			case ErrorCode::InvalidState: return "Invalid State";
			case ErrorCode::Timeout: return "Timeout";
			case ErrorCode::AccessDenied: return "Access Denied";

			// Engine errors
			case ErrorCode::EngineNotInitialized: return "Engine Not Initialized";
			case ErrorCode::EngineAlreadyInitialized: return "Engine Already Initialized";
			case ErrorCode::EngineShutdownFailed: return "Engine Shutdown Failed";
			case ErrorCode::SystemInitializationFailed: return "System Initialization Failed";

			// Renderer errors
			case ErrorCode::RendererInitFailed: return "Renderer Initialization Failed";
			case ErrorCode::ShaderCompilationFailed: return "Shader Compilation Failed";
			case ErrorCode::TextureLoadFailed: return "Texture Load Failed";
			case ErrorCode::BufferCreationFailed: return "Buffer Creation Failed";
			case ErrorCode::RenderTargetCreationFailed: return "Render Target Creation Failed";

			// Audio errors
			case ErrorCode::AudioInitFailed: return "Audio Initialization Failed";
			case ErrorCode::AudioDeviceNotFound: return "Audio Device Not Found";
			case ErrorCode::SoundLoadFailed: return "Sound Load Failed";
			case ErrorCode::AudioPlaybackFailed: return "Audio Playback Failed";

			// Input errors
			case ErrorCode::InputInitFailed: return "Input Initialization Failed";
			case ErrorCode::ControllerNotFound: return "Controller Not Found";
			case ErrorCode::KeyboardNotFound: return "Keyboard Not Found";
			case ErrorCode::MouseNotFound: return "Mouse Not Found";

			// File I/O errors
			case ErrorCode::FileNotFound: return "File Not Found";
			case ErrorCode::FileAccessDenied: return "File Access Denied";
			case ErrorCode::FileCorrupted: return "File Corrupted";
			case ErrorCode::DirectoryNotFound: return "Directory Not Found";
			case ErrorCode::DiskFull: return "Disk Full";

			// Network errors
			case ErrorCode::NetworkInitFailed: return "Network Initialization Failed";
			case ErrorCode::ConnectionFailed: return "Connection Failed";
			case ErrorCode::HostNotFound: return "Host Not Found";
			case ErrorCode::NetworkTimeout: return "Network Timeout";

			// Physics errors
			case ErrorCode::PhysicsInitFailed: return "Physics Initialization Failed";
			case ErrorCode::CollisionDetectionFailed: return "Collision Detection Failed";
			case ErrorCode::PhysicsSimulationFailed: return "Physics Simulation Failed";

			// Resource errors
			case ErrorCode::ResourceNotFound: return "Resource Not Found";
			case ErrorCode::ResourceLoadFailed: return "Resource Load Failed";
			case ErrorCode::ResourceCorrupted: return "Resource Corrupted";
			case ErrorCode::ResourceOutOfMemory: return "Resource Out of Memory";

			default: return "Unknown Error Code";
		}
	}

	std::string FormatError(ErrorCode code, const std::string& additionalInfo)
	{
		std::string result = ErrorCodeToString(code);
		if (!additionalInfo.empty())
		{
			result += ": " + additionalInfo;
		}
		return result;
	}

}