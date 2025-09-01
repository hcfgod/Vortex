#pragma once

#include "vxpch.h"
#include "ErrorCodes.h"

namespace Vortex 
{
	// Base engine exception
	class EngineException : public std::exception
	{
	public:
		EngineException(ErrorCode errorCode, const std::string& message = "", const std::string& file = "", int line = 0)
			: m_ErrorCode(errorCode), m_Message(message), m_File(file), m_Line(line)
		{
			m_FormattedMessage = FormatError(errorCode, message);
			if (!file.empty())
			{
				m_FormattedMessage += " at " + file + ":" + std::to_string(line);
			}
		}

		const char* what() const noexcept override { return m_FormattedMessage.c_str(); }
		
		ErrorCode GetErrorCode() const { return m_ErrorCode; }
		const std::string& GetMessage() const { return m_Message; }
		const std::string& GetFile() const { return m_File; }
		int GetLine() const { return m_Line; }

	private:
		ErrorCode m_ErrorCode;
		std::string m_Message;
		std::string m_File;
		int m_Line;
		std::string m_FormattedMessage;
	};

	// Specific exception types
	class RendererException : public EngineException
	{
	public:
		RendererException(ErrorCode errorCode, const std::string& message = "", const std::string& file = "", int line = 0)
			: EngineException(errorCode, message, file, line) {}
	};

	class AudioException : public EngineException
	{
	public:
		AudioException(ErrorCode errorCode, const std::string& message = "", const std::string& file = "", int line = 0)
			: EngineException(errorCode, message, file, line) {}
	};

	class InputException : public EngineException
	{
	public:
		InputException(ErrorCode errorCode, const std::string& message = "", const std::string& file = "", int line = 0)
			: EngineException(errorCode, message, file, line) {}
	};

	class FileException : public EngineException
	{
	public:
		FileException(ErrorCode errorCode, const std::string& message = "", const std::string& file = "", int line = 0)
			: EngineException(errorCode, message, file, line) {}
	};

	class PhysicsException : public EngineException
	{
	public:
		PhysicsException(ErrorCode errorCode, const std::string& message = "", const std::string& file = "", int line = 0)
			: EngineException(errorCode, message, file, line) {}
	};

	class ResourceException : public EngineException
	{
	public:
		ResourceException(ErrorCode errorCode, const std::string& message = "", const std::string& file = "", int line = 0)
			: EngineException(errorCode, message, file, line) {}
	};

}

// Exception throwing macros
#define VX_THROW(exceptionType, errorCode, message) \
	throw exceptionType(errorCode, message, __FILE__, __LINE__)

#define VX_THROW_IF(condition, exceptionType, errorCode, message) \
	do { \
		if (condition) \
		{ \
			VX_THROW(exceptionType, errorCode, message); \
		} \
	} while(0)

// Safe exception handling macros
#define VX_TRY_CATCH_LOG(code) \
	try \
	{ \
		code \
	} \
	catch (const ::Vortex::EngineException& e) \
	{ \
		VX_CORE_ERROR("Engine Exception: {0}", e.what()); \
	} \
	catch (const std::exception& e) \
	{ \
		VX_CORE_ERROR("Standard Exception: {0}", e.what()); \
	} \
	catch (...) \
	{ \
		VX_CORE_ERROR("Unknown Exception caught"); \
	}

#define VX_TRY_CATCH_RETURN(code, returnValue) \
	try \
	{ \
		code \
	} \
	catch (const ::Vortex::EngineException& e) \
	{ \
		VX_CORE_ERROR("Engine Exception: {0}", e.what()); \
		return returnValue; \
	} \
	catch (const std::exception& e) \
	{ \
		VX_CORE_ERROR("Standard Exception: {0}", e.what()); \
		return returnValue; \
	} \
	catch (...) \
	{ \
		VX_CORE_ERROR("Unknown Exception caught"); \
		return returnValue; \
	}