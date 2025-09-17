#include "vxpch.h"
#include "FrameBuffer.h"
#include "Engine/Renderer/RendererAPI.h"
#include "Engine/Renderer/OpenGL/OpenGLFrameBuffer.h"

namespace Vortex
{
    std::shared_ptr<FrameBuffer> FrameBuffer::Create(const FrameBufferSpec& spec)
    {
        switch (RendererAPIManager::GetInstance().GetCurrentAPI())
        {
            case GraphicsAPI::OpenGL:
                return std::make_shared<OpenGLFrameBuffer>(spec);
            default:
                return nullptr;
        }
    }
}


