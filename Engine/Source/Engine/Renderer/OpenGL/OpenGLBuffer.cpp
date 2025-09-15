#include "vxpch.h"
#include "OpenGLBuffer.h"
#include <glad/gl.h>
#include "Engine/Renderer/RenderCommandQueue.h"
#include "Engine/Renderer/RenderTypes.h"

namespace Vortex
{
    // ------------------------- OpenGLVertexBuffer -------------------------
    OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size, const void* data)
    {
        GetRenderCommandQueue().GenBuffers(1, &m_RendererID, true);
        // Upload data via render command queue (bind to ARRAY_BUFFER)
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer), m_RendererID);
        if (data && size > 0)
        {
            GetRenderCommandQueue().BufferData(static_cast<uint32_t>(BufferTarget::ArrayBuffer), data, size,
                static_cast<uint32_t>(BufferUsage::StaticDraw));
        }
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        if (m_RendererID)
        {
            GetRenderCommandQueue().DeleteBuffers(1, &m_RendererID, true);
            m_RendererID = 0;
        }
    }

    void OpenGLVertexBuffer::Bind() const
    {
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer), m_RendererID);
    }

    void OpenGLVertexBuffer::Unbind() const
    {
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer), 0);
    }

    void OpenGLVertexBuffer::SetData(const void* data, uint32_t size)
    {
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer), m_RendererID);
        GetRenderCommandQueue().BufferData(static_cast<uint32_t>(BufferTarget::ArrayBuffer), data, size,
            static_cast<uint32_t>(BufferUsage::DynamicDraw));
    }

    // ------------------------- OpenGLIndexBuffer -------------------------
    OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count)
        : m_Count(count)
    {
        GetRenderCommandQueue().GenBuffers(1, &m_RendererID, true);
        // Upload data via ARRAY_BUFFER to avoid VAO attachment here
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer), m_RendererID);
        GetRenderCommandQueue().BufferData(static_cast<uint32_t>(BufferTarget::ArrayBuffer), indices,
            count * sizeof(uint32_t), static_cast<uint32_t>(BufferUsage::StaticDraw));
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
        if (m_RendererID)
        {
            GetRenderCommandQueue().DeleteBuffers(1, &m_RendererID, true);
            m_RendererID = 0;
        }
    }

    void OpenGLIndexBuffer::Bind() const
    {
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ElementArrayBuffer), m_RendererID);
    }

    void OpenGLIndexBuffer::Unbind() const
    {
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ElementArrayBuffer), 0);
    }
}


