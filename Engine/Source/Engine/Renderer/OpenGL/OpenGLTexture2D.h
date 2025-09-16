#pragma once

#include "Engine/Renderer/Texture.h"

namespace Vortex
{
    class OpenGLTexture2D : public Texture2D
    {
    public:
        explicit OpenGLTexture2D(const CreateInfo& info);
        ~OpenGLTexture2D() override;

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }
        TextureFormat GetFormat() const override { return m_Format; }
        uint32_t GetRendererID() const override { return m_RendererID; }

        void Bind(uint32_t slot = 0) const override;
        void Unbind(uint32_t slot = 0) const override;

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        TextureFormat m_Format = TextureFormat::RGBA8;
    };
}


