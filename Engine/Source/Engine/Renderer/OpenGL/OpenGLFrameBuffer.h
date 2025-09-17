#pragma once

#include "Engine/Renderer/FrameBuffer.h"

namespace Vortex
{
    class OpenGLFrameBuffer : public FrameBuffer
    {
    public:
        explicit OpenGLFrameBuffer(const FrameBufferSpec& spec);
        ~OpenGLFrameBuffer() override;

        void Bind() const override;
        void Unbind() const override;

        uint32_t GetRendererID() const override { return m_FBO; }
        const FrameBufferSpec& GetSpec() const override { return m_Spec; }
        Texture2DRef GetColorAttachment(uint32_t index = 0) const override;

    private:
        void Invalidate();

    private:
        FrameBufferSpec m_Spec{};
        uint32_t m_FBO = 0;
        std::vector<Texture2DRef> m_ColorAttachments;
        uint32_t m_DepthAttachment = 0; // optional renderbuffer for depth
    };
}


