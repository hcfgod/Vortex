#include "vxpch.h"
#include "OpenGLBuffer.h"
#include <glad/gl.h>
#include "Engine/Renderer/RenderCommandQueue.h"
#include "Engine/Renderer/RenderTypes.h"

namespace Vortex
{
    // Helper ---------------------------------------------------------------
    static GLenum UsageToGL(BufferUsage usage)
    {
        switch (usage)
        {
        case BufferUsage::StaticDraw:  return GL_STATIC_DRAW;
        case BufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
        case BufferUsage::StreamDraw:  return GL_STREAM_DRAW;
        default:                   return GL_STATIC_DRAW;
        }
    }

    // ------------------------- OpenGLVertexBuffer -------------------------
    OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size, const void* data, BufferUsage usage)
    {
        GetRenderCommandQueue().GenBuffers(1, &m_RendererID, true);
        // Upload data via render command queue (bind to ARRAY_BUFFER)
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::ArrayBuffer), m_RendererID);
        if (data && size > 0)
        {
            GetRenderCommandQueue().BufferData(static_cast<uint32_t>(BufferTarget::ArrayBuffer), data, size,
                UsageToGL(usage));
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


