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

    // Optional generic bitfields for persistent mapping / storage flags (values match OpenGL)
    enum class BufferStorageFlags : uint32_t
    {
        None                 = 0,
        MapReadBit           = 0x0001,
        MapWriteBit          = 0x0002,
        MapPersistentBit     = 0x0040,
        MapCoherentBit       = 0x0080,
        DynamicStorageBit    = 0x0100,
        ClientStorageBit     = 0x0200
    };

    inline BufferStorageFlags operator|(BufferStorageFlags a, BufferStorageFlags b)
    {
        return static_cast<BufferStorageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline uint32_t ToFlags(BufferStorageFlags f) { return static_cast<uint32_t>(f); }

    enum class MapBufferAccess : uint32_t
    {
        None                    = 0,
        MapReadBit              = 0x0001,
        MapWriteBit             = 0x0002,
        MapInvalidateRangeBit   = 0x0004,
        MapInvalidateBufferBit  = 0x0008,
        MapFlushExplicitBit     = 0x0010,
        MapUnsynchronizedBit    = 0x0020,
        MapPersistentBit        = 0x0040,
        MapCoherentBit          = 0x0080
    };

    inline MapBufferAccess operator|(MapBufferAccess a, MapBufferAccess b)
    {
        return static_cast<MapBufferAccess>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline uint32_t ToFlags(MapBufferAccess f) { return static_cast<uint32_t>(f); }

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

    // ============================================================================
    // RENDER PASS SYSTEM ENUMS
    // ============================================================================

    /**
     * @brief High-level render domain categories for organizing render passes
     * 
     * Render domains define the semantic purpose of rendering content, allowing
     * the engine to apply appropriate effects (like lighting) only to relevant
     * content (e.g., World2D/World3D) while leaving UI unaffected.
     */
    enum class RenderDomain : uint8_t
    {
        Background = 0,   // Skybox, far backgrounds - rendered first
        World3D,          // 3D geometry, meshes, terrain
        World2D,          // 2D sprites, tilemaps, game content
        Effects,          // Particles, post-process targets
        UI,               // User interface elements - no lighting
        Overlay           // Debug overlays, always rendered on top
    };

    /**
     * @brief Convert RenderDomain to string for debugging
     */
    inline const char* RenderDomainToString(RenderDomain domain)
    {
        switch (domain)
        {
            case RenderDomain::Background: return "Background";
            case RenderDomain::World3D:    return "World3D";
            case RenderDomain::World2D:    return "World2D";
            case RenderDomain::Effects:    return "Effects";
            case RenderDomain::UI:         return "UI";
            case RenderDomain::Overlay:    return "Overlay";
            default:                       return "Unknown";
        }
    }

    /**
     * @brief Sort mode for render pass draw ordering
     * 
     * Controls how draw calls within a render pass are ordered before submission.
     */
    enum class RenderSortMode : uint8_t
    {
        None = 0,           // No sorting - draw in submission order
        FrontToBack,        // Sort by depth, closest first (opaque optimization)
        BackToFront,        // Sort by depth, farthest first (transparency)
        ByMaterial,         // Group by material/shader to minimize state changes
        ByTexture           // Group by texture to minimize texture binds
    };

    /**
     * @brief Convert RenderSortMode to string for debugging
     */
    inline const char* RenderSortModeToString(RenderSortMode mode)
    {
        switch (mode)
        {
            case RenderSortMode::None:        return "None";
            case RenderSortMode::FrontToBack: return "FrontToBack";
            case RenderSortMode::BackToFront: return "BackToFront";
            case RenderSortMode::ByMaterial:  return "ByMaterial";
            case RenderSortMode::ByTexture:   return "ByTexture";
            default:                          return "Unknown";
        }
    }

    /**
     * @brief Depth compare function for render state
     */
    enum class DepthCompareFunc : uint8_t
    {
        Never = 0,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };

    /**
     * @brief Blend factors for render state
     */
    enum class BlendFactor : uint8_t
    {
        Zero = 0,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha
    };

    /**
     * @brief Blend operation for render state
     */
    enum class BlendOp : uint8_t
    {
        Add = 0,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };
}