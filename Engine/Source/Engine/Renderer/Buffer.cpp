#include "vxpch.h"
#include "Buffer.h"
#include "RendererAPI.h"
#ifdef VX_OPENGL_SUPPORT
#include "OpenGL/OpenGLBuffer.h"
#endif

namespace Vortex
{
    std::shared_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size, const void* data, BufferUsage usage)
    {
        switch (RendererAPIManager::GetInstance().GetCurrentAPI())
        {
            case GraphicsAPI::OpenGL:
            {
            #ifdef VX_OPENGL_SUPPORT
                return std::make_shared<OpenGLVertexBuffer>(size, data, usage);
            #else
                return nullptr;
            #endif
            }
            default:
                return nullptr;
        }
    }

    std::shared_ptr<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
    {
        switch (RendererAPIManager::GetInstance().GetCurrentAPI())
        {
            case GraphicsAPI::OpenGL:
            {
            #ifdef VX_OPENGL_SUPPORT
                return std::make_shared<OpenGLIndexBuffer>(indices, count);
            #else
                return nullptr;
            #endif
            }
            default:
                return nullptr;
        }
    }
}
