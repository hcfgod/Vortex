#include "vxpch.h"
#include "Texture.h"
#include "Engine/Renderer/GraphicsContext.h"
#include "Engine/Renderer/OpenGL/OpenGLTexture2D.h"
#include <Core/Debug/Assert.h>

namespace Vortex
{
    std::shared_ptr<Texture2D> Texture2D::Create(const TextureCreateInfo& info)
    {
        switch (GetGraphicsAPI())
        {
            case GraphicsAPI::OpenGL:
                return std::make_shared<OpenGLTexture2D>(info);
            default:
                VX_CORE_ASSERT(false, "Texture2D::Create - Unsupported GraphicsAPI");
                return nullptr;
        }
    }
}


