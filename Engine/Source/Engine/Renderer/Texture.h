#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "Core/Debug/ErrorCodes.h"
#include "Engine/Renderer/RenderTypes.h"

namespace Vortex
{
    enum class TextureFormat : uint32_t
    {
        None = 0,
        RGB8,
        RGBA8
    };

    enum class TextureFilter : uint32_t
    {
        Nearest = 0,
        Linear
    };

    enum class TextureWrap : uint32_t
    {
        ClampToEdge = 0,
        Repeat,
        MirroredRepeat
    };

    class Texture2D
    {
    public:
        virtual ~Texture2D() = default;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual TextureFormat GetFormat() const = 0;
        virtual uint32_t GetRendererID() const = 0;

        virtual void Bind(uint32_t slot = 0) const = 0;
        virtual void Unbind(uint32_t slot = 0) const = 0;

        struct CreateInfo
        {
            uint32_t Width = 0;
            uint32_t Height = 0;
            TextureFormat Format = TextureFormat::RGBA8;
            TextureFilter MinFilter = TextureFilter::Linear;
            TextureFilter MagFilter = TextureFilter::Linear;
            TextureWrap WrapS = TextureWrap::Repeat;
            TextureWrap WrapT = TextureWrap::Repeat;
            bool GenerateMips = true;
            const void* InitialData = nullptr;
            uint64_t InitialDataSize = 0;
        };

        static std::shared_ptr<Texture2D> Create(const CreateInfo& info);
    };

    using Texture2DRef = std::shared_ptr<Texture2D>;
}


