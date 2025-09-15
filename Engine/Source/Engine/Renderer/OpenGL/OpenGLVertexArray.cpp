#include "vxpch.h"
#include "OpenGLVertexArray.h"
#include "OpenGLBuffer.h"
#include <glad/gl.h>
#include "Core/Debug/Assert.h"
#include "Engine/Renderer/RenderCommandQueue.h"
#include "Engine/Renderer/RenderTypes.h"

namespace Vortex
{
    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float:
            case ShaderDataType::Vec2:
            case ShaderDataType::Vec3:
            case ShaderDataType::Vec4:
            case ShaderDataType::Mat2:
            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:    return GL_FLOAT;
            case ShaderDataType::Int:
            case ShaderDataType::IVec2:
            case ShaderDataType::IVec3:
            case ShaderDataType::IVec4:   return GL_INT;
            case ShaderDataType::Bool:    return GL_BOOL;
            default:                      return GL_FLOAT;
        }
    }

    OpenGLVertexArray::OpenGLVertexArray()
    {
		GetRenderCommandQueue().GenVertexArrays(1, &m_RendererID, false);
    }

    OpenGLVertexArray::~OpenGLVertexArray()
    {
        if (m_RendererID)
        {
            GetRenderCommandQueue().DeleteVertexArrays(1, &m_RendererID, false);
            m_RendererID = 0;
        }
    }

    void OpenGLVertexArray::Bind() const
    {
        GetRenderCommandQueue().BindVertexArray(m_RendererID);
    }

    void OpenGLVertexArray::Unbind() const
    {
        GetRenderCommandQueue().BindVertexArray(0);
    }

    void OpenGLVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
    {
        VX_CORE_ASSERT(vertexBuffer && !vertexBuffer->GetLayout().Empty(), "VertexBuffer has no layout!");

        GetRenderCommandQueue().BindVertexArray(m_RendererID);
        vertexBuffer->Bind();

        const auto& layout = vertexBuffer->GetLayout();
        for (const auto& element : layout)
        {
            GLenum glBaseType = ShaderDataTypeToOpenGLBaseType(element.Type);

            if (element.Type == ShaderDataType::Mat2 || element.Type == ShaderDataType::Mat3 || element.Type == ShaderDataType::Mat4)
            {
                uint8_t count = element.GetComponentCount();
                for (uint8_t i = 0; i < count; i++)
                {
                    GetRenderCommandQueue().EnableVertexAttribArray(m_VertexAttribIndex, true);
                    GetRenderCommandQueue().VertexAttribPointer(m_VertexAttribIndex,
                        (element.Type == ShaderDataType::Mat2 ? 2 : (element.Type == ShaderDataType::Mat3 ? 3 : 4)),
                        glBaseType,
                        element.Normalized,
                        layout.GetStride(),
                        (element.Offset + sizeof(float) * (element.Type == ShaderDataType::Mat2 ? 2 : (element.Type == ShaderDataType::Mat3 ? 3 : 4)) * i));
                    m_VertexAttribIndex++;
                }
            }
            else
            {
                GetRenderCommandQueue().EnableVertexAttribArray(m_VertexAttribIndex, true);
                GetRenderCommandQueue().VertexAttribPointer(m_VertexAttribIndex,
                    element.GetComponentCount(),
                    glBaseType,
                    element.Normalized,
                    layout.GetStride(),
                    element.Offset);
                m_VertexAttribIndex++;
            }
        }

        m_VertexBuffers.push_back(vertexBuffer);
    }

    void OpenGLVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
    {
        GetRenderCommandQueue().BindVertexArray(m_RendererID);
        indexBuffer->Bind();
        m_IndexBuffer = indexBuffer;
    }
}


