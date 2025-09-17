#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/RenderTypes.h"

namespace Vortex
{
    struct FrameBufferSpec
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t ColorAttachmentCount = 1;
        bool HasDepth = true;
    };

    class FrameBuffer
    {
    public:
        virtual ~FrameBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual uint32_t GetRendererID() const = 0;
        virtual const FrameBufferSpec& GetSpec() const = 0;
        virtual Texture2DRef GetColorAttachment(uint32_t index = 0) const = 0;

        static std::shared_ptr<FrameBuffer> Create(const FrameBufferSpec& spec);
    };

    using FrameBufferRef = std::shared_ptr<FrameBuffer>;
}


