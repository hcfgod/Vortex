#include "vxpch.h"
#include "OpenGLTexture2D.h"
#include "Engine/Renderer/RenderCommandQueue.h"

namespace Vortex
{
    static uint32_t ToGLInternalFormat(TextureFormat fmt)
    {
        switch (fmt)
        {
            case TextureFormat::RGB8:  return 0x8051; // GL_RGB8
            case TextureFormat::RGBA8: return 0x8058; // GL_RGBA8
            default: return 0x8058;
        }
    }

    static uint32_t ToGLFormat(TextureFormat fmt)
    {
        switch (fmt)
        {
            case TextureFormat::RGB8:  return 0x1907; // GL_RGB
            case TextureFormat::RGBA8: return 0x1908; // GL_RGBA
            default: return 0x1908;
        }
    }

    static uint32_t ToGLFilter(TextureFilter f)
    {
        switch (f)
        {
            case TextureFilter::Nearest: return 0x2600; // GL_NEAREST
            case TextureFilter::Linear:  return 0x2601; // GL_LINEAR
            default: return 0x2601;
        }
    }

    static uint32_t ToGLWrap(TextureWrap w)
    {
        switch (w)
        {
            case TextureWrap::ClampToEdge:   return 0x812F; // GL_CLAMP_TO_EDGE
            case TextureWrap::Repeat:        return 0x2901; // GL_REPEAT
            case TextureWrap::MirroredRepeat:return 0x8370; // GL_MIRRORED_REPEAT
            default: return 0x2901;
        }
    }

    static uint32_t ToGLTarget(TextureTarget target)
    {
        switch (target)
        {
            case TextureTarget::Texture2D: return 0x0DE1; // GL_TEXTURE_2D
            default: return 0x0DE1;
        }
    }

    OpenGLTexture2D::OpenGLTexture2D(const CreateInfo& info)
    {
        m_Width = info.Width;
        m_Height = info.Height;
        m_Format = info.Format;

        // Create texture object
        GetRenderCommandQueue().GenTextures(1, &m_RendererID, true);
        const uint32_t glTarget = static_cast<uint32_t>(TextureTarget::Texture2D);
        GetRenderCommandQueue().BindTextureTarget(glTarget, m_RendererID, true); 

        // Set parameters
        GetRenderCommandQueue().TexParameteri(glTarget, 0x2801 /*GL_TEXTURE_MIN_FILTER*/, ToGLFilter(info.MinFilter), true);
        GetRenderCommandQueue().TexParameteri(glTarget, 0x2800 /*GL_TEXTURE_MAG_FILTER*/, ToGLFilter(info.MagFilter), true);
        GetRenderCommandQueue().TexParameteri(glTarget, 0x2802 /*GL_TEXTURE_WRAP_S*/, ToGLWrap(info.WrapS), true);
        GetRenderCommandQueue().TexParameteri(glTarget, 0x2803 /*GL_TEXTURE_WRAP_T*/, ToGLWrap(info.WrapT), true);

        // Allocate / upload data
        const uint32_t glInternal = ToGLInternalFormat(info.Format);
        const uint32_t glFormat = ToGLFormat(info.Format);
        const uint32_t glType = 0x1401; // GL_UNSIGNED_BYTE
        GetRenderCommandQueue().TexImage2D(glTarget, 0, glInternal, m_Width, m_Height, glFormat, glType,
                                           info.InitialData, info.InitialDataSize, true);

        if (info.GenerateMips)
        {
            GetRenderCommandQueue().GenerateMipmap(glTarget, true);
        }
    }

    OpenGLTexture2D::~OpenGLTexture2D()
    {
        if (m_RendererID)
        {
            GetRenderCommandQueue().DeleteTextures(1, &m_RendererID, true);
            m_RendererID = 0;
        }
    }

    void OpenGLTexture2D::Bind(uint32_t slot) const
    {
        GetRenderCommandQueue().BindTexture(slot, m_RendererID);
    }

    void OpenGLTexture2D::Unbind(uint32_t slot) const
    {
        GetRenderCommandQueue().BindTexture(slot, 0);
    }
}


