#pragma once

#include "Engine/Renderer/UniformBuffer.h"

namespace Vortex
{
    class OpenGLUniformBuffer : public UniformBuffer
    {
    public:
        OpenGLUniformBuffer(uint32_t size, uint32_t bindingIndex, const void* initialData);
        ~OpenGLUniformBuffer() override;

        void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
        void BindBase(uint32_t bindingIndex) const override;
        uint32_t GetRendererID() const override { return m_RendererID; }

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_BindingIndex = 0;
        uint32_t m_Size = 0;
    };
}


