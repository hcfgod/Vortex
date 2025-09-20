#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "Engine/Renderer/Shader/ShaderTypes.h"
#include "RenderTypes.h"

namespace Vortex
{
    // =====================================================================================
    // Buffer Layout (Cherno-style) using engine ShaderDataType
    // =====================================================================================

    struct BufferElement
    {
        std::string Name;
        ShaderDataType Type = ShaderDataType::Unknown;
        uint32_t Size = 0;
        size_t Offset = 0;
        bool Normalized = false;

        BufferElement() = default;

        BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
            : Name(name), Type(type), Size(GetShaderDataTypeSize(type)), Offset(0), Normalized(normalized) {}

        uint32_t GetComponentCount() const
        {
            switch (Type)
            {
                case ShaderDataType::Float:
                case ShaderDataType::Int:
                case ShaderDataType::UInt:
                case ShaderDataType::Double:
                case ShaderDataType::Bool:   return 1;
                case ShaderDataType::Vec2:
                case ShaderDataType::IVec2:
                case ShaderDataType::UVec2:  return 2;
                case ShaderDataType::Vec3:
                case ShaderDataType::IVec3:
                case ShaderDataType::UVec3:  return 3;
                case ShaderDataType::Vec4:
                case ShaderDataType::IVec4:
                case ShaderDataType::UVec4:  return 4;
                case ShaderDataType::Mat2:   return 2; // 2 columns
                case ShaderDataType::Mat3:   return 3; // 3 columns
                case ShaderDataType::Mat4:   return 4; // 4 columns
                default: return 0;
            }
        }
    };

class BufferLayout
    {
    public:
        BufferLayout() = default;

        BufferLayout(std::initializer_list<BufferElement> elements)
            : m_Elements(elements)
        {
            CalculateOffsetsAndStride();
        }

        inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }
        inline uint32_t GetStride() const { return m_Stride; }
        inline bool Empty() const { return m_Elements.empty(); }

        // Instance divisor control (0 = per-vertex, 1 = per-instance)
        inline void SetDivisor(uint32_t divisor) { m_Divisor = divisor; }
        inline uint32_t GetDivisor() const { return m_Divisor; }

        std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
        std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
        std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
        std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }

    private:
        void CalculateOffsetsAndStride()
        {
            size_t offset = 0;
            m_Stride = 0;
            for (auto& element : m_Elements)
            {
                element.Offset = offset;
                offset += element.Size;
                m_Stride += element.Size;
            }
        }

    private:
        std::vector<BufferElement> m_Elements;
        uint32_t m_Stride = 0;
        uint32_t m_Divisor = 0; // 0 = per-vertex, >0 = per-instance
    };

    // =====================================================================================
    // VertexBuffer / IndexBuffer Interfaces
    // =====================================================================================

    class VertexBuffer
    {
    public:
        virtual ~VertexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual void SetData(const void* data, uint32_t size) = 0;

        virtual void SetLayout(const BufferLayout& layout) = 0;
        virtual const BufferLayout& GetLayout() const = 0;

        virtual uint32_t GetRendererID() const = 0;

        static std::shared_ptr<VertexBuffer> Create(uint32_t size, const void* data = nullptr, BufferUsage usage = BufferUsage::StaticDraw);
    };

    class IndexBuffer
    {
    public:
        virtual ~IndexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual uint32_t GetCount() const = 0;
        virtual uint32_t GetRendererID() const = 0;

        static std::shared_ptr<IndexBuffer> Create(uint32_t* indices, uint32_t count);
    };
}


