#include "vxpch.h"
#include "OpenGLFrameBuffer.h"
#include "Engine/Renderer/RenderCommandQueue.h"

namespace Vortex
{
    OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferSpec& spec)
        : m_Spec(spec)
    {
        Invalidate();
    }

    OpenGLFrameBuffer::~OpenGLFrameBuffer()
    {
        if (m_FBO)
        {
            GetRenderCommandQueue().DeleteFramebuffers(1, &m_FBO, true);
            m_FBO = 0;
        }
    }

    void OpenGLFrameBuffer::Invalidate()
    {
        if (m_FBO)
        {
            GetRenderCommandQueue().DeleteFramebuffers(1, &m_FBO, true);
            m_FBO = 0;
        }

        // Create FBO
        GetRenderCommandQueue().GenFramebuffers(1, &m_FBO, true);
        GetRenderCommandQueue().BindFramebuffer(FramebufferTarget::Framebuffer, m_FBO, false);

        // Create color textures
        m_ColorAttachments.clear();
        m_ColorAttachments.resize(m_Spec.ColorAttachmentCount);
        for (uint32_t i = 0; i < m_Spec.ColorAttachmentCount; ++i)
        {
            Texture2D::CreateInfo info{};
            info.Width = m_Spec.Width;
            info.Height = m_Spec.Height;
            info.Format = TextureFormat::RGBA8;
            info.GenerateMips = false;
            auto tex = Texture2D::Create(info);
            m_ColorAttachments[i] = tex;
            GetRenderCommandQueue().FramebufferTexture2D(FramebufferTarget::Framebuffer,
                static_cast<FramebufferAttachment>(static_cast<uint32_t>(FramebufferAttachment::Color0) + i),
                TextureTarget::Texture2D, tex->GetRendererID(), 0, false);
        }

        // Ensure draw buffers are set for the FBO (at least COLOR_ATTACHMENT0)
        if (m_Spec.ColorAttachmentCount > 0)
        {
            std::vector<uint32_t> attachments(m_Spec.ColorAttachmentCount);
            for (uint32_t i = 0; i < m_Spec.ColorAttachmentCount; ++i)
                attachments[i] = i;
            GetRenderCommandQueue().SetDrawBuffers(static_cast<uint32_t>(attachments.size()), attachments.data(), false);
        }

        // Depth attachment (optional) - create a renderbuffer for depth
        if (m_Spec.HasDepth)
        {
            // Create and attach depth renderbuffer
            uint32_t rbo = 0;
            // Use GL calls through queue not currently exposed; for now, skip to avoid inconsistent state
            // FBO will still be complete without depth for basic color rendering
        }

        uint32_t status = 0;
        GetRenderCommandQueue().CheckFramebufferStatus(FramebufferTarget::Framebuffer, &status, true);

        // Unbind to default
        GetRenderCommandQueue().BindFramebuffer(FramebufferTarget::Framebuffer, 0, false);
    }

    void OpenGLFrameBuffer::Bind() const
    {
        GetRenderCommandQueue().BindFramebuffer(FramebufferTarget::Framebuffer, m_FBO);
    }

    void OpenGLFrameBuffer::Unbind() const
    {
        GetRenderCommandQueue().BindFramebuffer(FramebufferTarget::Framebuffer, 0);
    }

    Texture2DRef OpenGLFrameBuffer::GetColorAttachment(uint32_t index) const
    {
        if (index >= m_ColorAttachments.size()) return nullptr;
        return m_ColorAttachments[index];
    }
}


