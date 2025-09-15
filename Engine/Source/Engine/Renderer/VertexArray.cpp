#include "vxpch.h"
#include "VertexArray.h"
#include "RendererAPI.h"
#ifdef VX_OPENGL_SUPPORT
#include "OpenGL/OpenGLVertexArray.h"
#endif

namespace Vortex
{
    std::shared_ptr<VertexArray> VertexArray::Create()
    {
        switch (RendererAPIManager::GetInstance().GetCurrentAPI())
        {
            case GraphicsAPI::OpenGL:
            {
            #ifdef VX_OPENGL_SUPPORT
                return std::make_shared<OpenGLVertexArray>();
            #else
                return nullptr;
            #endif
            }
            default:
                return nullptr;
        }
    }
}


