#include "vxpch.h"
#include "UniformBuffer.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/OpenGL/OpenGLUniformBuffer.h"

namespace Vortex
{
    std::shared_ptr<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t bindingIndex, const void* initialData)
    {
        switch (RendererAPIManager::GetInstance().GetCurrentAPI())
        {
            case GraphicsAPI::OpenGL:
            {
                return std::make_shared<OpenGLUniformBuffer>(size, bindingIndex, initialData);
            }
            default:
                return nullptr;
        }
    }
}


