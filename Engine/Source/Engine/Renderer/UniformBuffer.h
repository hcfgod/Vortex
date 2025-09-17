#pragma once

#include <cstdint>
#include <memory>

namespace Vortex
{
    class UniformBuffer
    {
    public:
        virtual ~UniformBuffer() = default;

        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
        virtual void BindBase(uint32_t bindingIndex) const = 0;
        virtual uint32_t GetRendererID() const = 0;

        static std::shared_ptr<UniformBuffer> Create(uint32_t size, uint32_t bindingIndex, const void* initialData = nullptr);
    };

    using UniformBufferRef = std::shared_ptr<UniformBuffer>;
}


