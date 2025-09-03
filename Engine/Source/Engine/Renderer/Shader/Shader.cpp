#include "vxpch.h"
#include "Shader.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/OpenGL/OpenGLShader.h"
#include "Core/Debug/Assert.h"
#include "Core/Debug/Log.h"

namespace Vortex
{
    bool GPUShader::HasStage(ShaderStage stage) const
    {
        uint32_t stageFlag = 1u << static_cast<uint32_t>(stage);
        return (m_StageFlags & stageFlag) != 0;
    }

    void GPUShader::SetMetadata(const std::string& name, 
                            const ShaderReflectionData& reflection,
                            ShaderStageFlags stageFlags)
    {
        m_Name = name;
        m_ReflectionData = reflection;
        m_StageFlags = stageFlags;
    }

    std::unique_ptr<GPUShader> GPUShader::Create(const std::string& name)
    {
        // Get current graphics API and create appropriate shader implementation
        auto currentAPI = GetGraphicsAPI();
        
        switch (currentAPI)
        {
            case GraphicsAPI::OpenGL:
                return std::make_unique<OpenGLShader>(name);
            case GraphicsAPI::Vulkan:
                // TODO: Implement VulkanShader when we add Vulkan support
                VX_CORE_ASSERT(false, "Vulkan shaders not yet implemented!");
                return nullptr;
		    case GraphicsAPI::DirectX11:
			    VX_CORE_ASSERT(false, "DirectX11 shaders not yet implemented!");
			    return nullptr;
            case GraphicsAPI::DirectX12:
                // TODO: Implement D3D12Shader when we add D3D12 support
                VX_CORE_ASSERT(false, "D3D12 shaders not yet implemented!");
                return nullptr;
            default:
                VX_CORE_ASSERT(false, "Unknown graphics API!");
            return nullptr;
        }
    }

} // namespace Vortex