#include "vxpch.h"
#include "OpenGLUniformBuffer.h"
#include "Engine/Renderer/RenderCommandQueue.h"
#include "Engine/Renderer/RenderTypes.h"

namespace Vortex
{
    OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t bindingIndex, const void* initialData)
        : m_BindingIndex(bindingIndex), m_Size(size)
    {
        GetRenderCommandQueue().GenBuffers(1, &m_RendererID, true);
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::UniformBuffer), m_RendererID);
        if (size > 0)
        {
            GetRenderCommandQueue().BufferData(static_cast<uint32_t>(BufferTarget::UniformBuffer), initialData, size,
                static_cast<uint32_t>(BufferUsage::DynamicDraw));
        }
        if (bindingIndex != UINT32_MAX)
        {
            GetRenderCommandQueue().BindBufferBase(BufferTarget::UniformBuffer, bindingIndex, m_RendererID);
        }
    }

    OpenGLUniformBuffer::~OpenGLUniformBuffer()
    {
        if (m_RendererID)
        {
            GetRenderCommandQueue().DeleteBuffers(1, &m_RendererID, true);
            m_RendererID = 0;
        }
    }

    void OpenGLUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
    {
        GetRenderCommandQueue().BindBuffer(static_cast<uint32_t>(BufferTarget::UniformBuffer), m_RendererID);
        if (offset == 0 && size == m_Size)
        {
            GetRenderCommandQueue().BufferData(static_cast<uint32_t>(BufferTarget::UniformBuffer), data, size,
                static_cast<uint32_t>(BufferUsage::DynamicDraw));
        }
        else
        {
            GetRenderCommandQueue().BufferSubData(static_cast<uint32_t>(BufferTarget::UniformBuffer), offset, data, size);
        }
    }

    void OpenGLUniformBuffer::BindBase(uint32_t bindingIndex) const
    {
        GetRenderCommandQueue().BindBufferBase(BufferTarget::UniformBuffer, bindingIndex, m_RendererID);
    }
}


