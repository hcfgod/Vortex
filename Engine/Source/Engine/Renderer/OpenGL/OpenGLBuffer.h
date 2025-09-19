#pragma once

#include "Engine/Renderer/Buffer.h"
#include <glad/gl.h>

namespace Vortex
{
    class OpenGLVertexBuffer : public VertexBuffer
    {
    public:
        OpenGLVertexBuffer(uint32_t size, const void* data, BufferUsage usage);
        ~OpenGLVertexBuffer() override;

        void Bind() const override;
        void Unbind() const override;
        void SetData(const void* data, uint32_t size) override;

        void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
        const BufferLayout& GetLayout() const override { return m_Layout; }

        uint32_t GetRendererID() const override { return m_RendererID; }

    private:
        uint32_t m_RendererID = 0;
        BufferLayout m_Layout;
    };

    class OpenGLIndexBuffer : public IndexBuffer
    {
    public:
        OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
        ~OpenGLIndexBuffer() override;

        void Bind() const override;
        void Unbind() const override;

        uint32_t GetCount() const override { return m_Count; }
        uint32_t GetRendererID() const override { return m_RendererID; }

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_Count = 0;
    };
}


