#pragma once

// For use by Vortex applications

// Core
#include "Core/Application.h"
#include "Core/EngineConfig.h"
#include "Core/FileSystem.h"
#include "Core/Async/Task.h"
#include "Core/Async/Coroutine.h"
#include "Core/Async/CoroutineScheduler.h"
#include "Engine/Engine.h"
#include "Core/Common/SmartRef.h"
#include "Engine/Systems/SystemAccessors.h"

// Event System
#include "Core/Events/EventSystem.h"
#include "Core/Events/Event.h"
#include "Core/Events/ApplicationEvent.h"
#include "Core/Events/WindowEvent.h"

// Debug and Error Handling
#include "Core/Debug/Log.h"
#include "Core/Debug/Assert.h"
#include "Core/Debug/ErrorCodes.h"
#include "Core/Debug/Exception.h"
#include "Core/Debug/CrashHandler.h"

// Time System
#include "Engine/Time/Time.h"
#include "Engine/Time/TimeSystem.h"

// Input
#include "Engine/Input/KeyCodes.h"
#include "Engine/Input/MouseCodes.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/InputActions.h"
#include "Engine/Input/InputSystem.h"

// Layer System
#include "Core/Layer/Layer.h"
#include "Core/Layer/LayerStack.h"

// Renderer
#include "Engine/Renderer/GraphicsContext.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Engine/Renderer/RenderCommandQueue.h"
#include "Engine/Systems/RenderSystem.h"
#include "Engine/Renderer/Shader/Shader.h"
#include "Engine/Renderer/Shader/ShaderCompiler.h"
#include "Engine/Renderer/Shader/ShaderManager.h"
#include "Engine/Renderer/Shader/ShaderReflection.h"
#include "Engine/Renderer/RenderTypes.h"
#include "Engine/Renderer/VertexArray.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/UniformBuffer.h"
#include "Engine/Renderer/FrameBuffer.h"

// Camera
#include "Engine/Camera/Camera.h"

// Assets
#include "Engine/Assets/Asset.h"
#include "Engine/Assets/AssetHandle.h"
#include "Engine/Assets/ShaderAsset.h"
#include "Engine/Assets/TextureAsset.h"
#include "Engine/Assets/AssetSystem.h"

// ---Entry Point--------------------- 
// Only include EntryPoint when VX_IMPLEMENT_MAIN is defined
// This prevents multiple main() function definitions
#ifdef VX_IMPLEMENT_MAIN
#include "Core/EntryPoint.h"
#endif
// -----------------------------------