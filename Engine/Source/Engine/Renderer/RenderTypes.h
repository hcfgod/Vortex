#pragma once

namespace Vortex
{
    // API-agnostic render enums used across the engine

    enum class BufferTarget : uint32_t
    {
        ArrayBuffer = 0,
        ElementArrayBuffer = 1,
        UniformBuffer = 2,
        ShaderStorageBuffer = 3
    };

    enum class BufferUsage : uint32_t
    {
        StaticDraw = 0,
        DynamicDraw = 1,
        StreamDraw = 2
    };

    enum class DataType : uint32_t
    {
        Byte = 0,
        UnsignedByte = 1,
        Short = 2,
        UnsignedShort = 3,
        Int = 4,
        UnsignedInt = 5,
        HalfFloat = 6,
        Float = 7,
        DoubleType = 8
    };

    enum class PrimitiveTopology : uint32_t
    {
        Points = 0,
        Lines = 1,
        LineStrip = 2,
        LineLoop = 3,
        Triangles = 4,
        TriangleStrip = 5,
        TriangleFan = 6
    };

    // Texture enums for API-agnostic calls
    enum class TextureTarget : uint32_t
    {
        Texture2D = 0
    };

    enum class TextureParamName : uint32_t
    {
        MinFilter = 0,
        MagFilter = 1,
        WrapS     = 2,
        WrapT     = 3
    };

    // Framebuffer enums for API-agnostic calls
    enum class FramebufferTarget : uint32_t
    {
        Framebuffer = 0,      // both read+draw
        ReadFramebuffer = 1,
        DrawFramebuffer = 2
    };

    enum class FramebufferAttachment : uint32_t
    {
        Color0 = 0,
        Color1 = 1,
        Color2 = 2,
        Color3 = 3,
        Depth = 100,
        Stencil = 101,
        DepthStencil = 102
    };
}