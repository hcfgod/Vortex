#pragma once

#include "vxpch.h"
#include "Core/Debug/ErrorCodes.h"
#include <glm/glm.hpp>

namespace Vortex
{
    // Forward declarations
    class GraphicsContext;

    /**
     * @brief Abstract base class for all render commands
     * 
     * Render commands encapsulate specific rendering operations that can be executed
     * on any graphics API. They are designed to be lightweight, copyable objects
     * that can be queued and executed later on the rendering thread.
     */
    class RenderCommand
    {
    public:
        virtual ~RenderCommand() = default;

        /**
         * @brief Execute this render command
         * @param context The graphics context to execute the command on
         * @return Success/failure result
         */
        virtual Result<void> Execute(GraphicsContext* context) = 0;

        /**
         * @brief Get debug information about this command
         * @return Debug string describing the command
         */
        virtual std::string GetDebugName() const = 0;

        /**
         * @brief Get the estimated GPU cost of this command (for profiling)
         * @return Estimated relative cost (1.0 = baseline draw call)
         */
        virtual float GetEstimatedCost() const { return 1.0f; }
    };

    // ============================================================================
    // BASIC RENDER COMMANDS
    // ============================================================================

    /**
     * @brief Command to clear the screen with specified parameters
     */
    class ClearCommand : public RenderCommand
    {
    public:
        enum ClearFlags : uint32_t
        {
            Color = 1 << 0,
            Depth = 1 << 1,
            Stencil = 1 << 2,
            All = Color | Depth | Stencil
        };

        ClearCommand(uint32_t flags = ClearFlags::All, 
                    const glm::vec4& color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    float depth = 1.0f, 
                    int32_t stencil = 0)
            : m_Flags(flags), m_ClearColor(color), m_DepthValue(depth), m_StencilValue(stencil) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "Clear"; }
        float GetEstimatedCost() const override { return 0.1f; }

    private:
        uint32_t m_Flags;
        glm::vec4 m_ClearColor;
        float m_DepthValue;
        int32_t m_StencilValue;
    };

    /**
     * @brief Command to set the viewport
     */
    class SetViewportCommand : public RenderCommand
    {
    public:
        SetViewportCommand(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
            : m_X(x), m_Y(y), m_Width(width), m_Height(height) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "SetViewport"; }
        float GetEstimatedCost() const override { return 0.05f; }

    private:
        uint32_t m_X, m_Y, m_Width, m_Height;
    };

    /**
     * @brief Command to set the scissor test rectangle
     */
    class SetScissorCommand : public RenderCommand
    {
    public:
        SetScissorCommand(bool enabled, uint32_t x = 0, uint32_t y = 0, uint32_t width = 0, uint32_t height = 0)
            : m_Enabled(enabled), m_X(x), m_Y(y), m_Width(width), m_Height(height) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "SetScissor"; }
        float GetEstimatedCost() const override { return 0.05f; }

    private:
        bool m_Enabled;
        uint32_t m_X, m_Y, m_Width, m_Height;
    };

    // ============================================================================
    // DRAWING COMMANDS
    // ============================================================================

    /**
     * @brief Command to draw indexed triangles
     */
    class DrawIndexedCommand : public RenderCommand
    {
    public:
        DrawIndexedCommand(uint32_t indexCount, uint32_t instanceCount = 1, 
                          uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t baseInstance = 0)
            : m_IndexCount(indexCount), m_InstanceCount(instanceCount)
            , m_FirstIndex(firstIndex), m_BaseVertex(baseVertex), m_BaseInstance(baseInstance) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "DrawIndexed"; }
        float GetEstimatedCost() const override { return m_IndexCount / 3.0f * m_InstanceCount * 0.001f; }

    private:
        uint32_t m_IndexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstIndex;
        int32_t m_BaseVertex;
        uint32_t m_BaseInstance;
    };

    /**
     * @brief Command to draw arrays (non-indexed)
     */
    class DrawArraysCommand : public RenderCommand
    {
    public:
        DrawArraysCommand(uint32_t vertexCount, uint32_t instanceCount = 1, 
                         uint32_t firstVertex = 0, uint32_t baseInstance = 0)
            : m_VertexCount(vertexCount), m_InstanceCount(instanceCount)
            , m_FirstVertex(firstVertex), m_BaseInstance(baseInstance) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "DrawArrays"; }
        float GetEstimatedCost() const override { return m_VertexCount / 3.0f * m_InstanceCount * 0.001f; }

    private:
        uint32_t m_VertexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstVertex;
        uint32_t m_BaseInstance;
    };

    // ============================================================================
    // RESOURCE BINDING COMMANDS
    // ============================================================================

    /**
     * @brief Command to bind a vertex buffer
     */
    class BindVertexBufferCommand : public RenderCommand
    {
    public:
        BindVertexBufferCommand(uint32_t binding, uint32_t buffer, uint64_t offset = 0, uint64_t stride = 0)
            : m_Binding(binding), m_Buffer(buffer), m_Offset(offset), m_Stride(stride) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BindVertexBuffer"; }
        float GetEstimatedCost() const override { return 0.02f; }

    private:
        uint32_t m_Binding;
        uint32_t m_Buffer;
        uint64_t m_Offset;
        uint64_t m_Stride;
    };

    /**
     * @brief Command to bind an index buffer
     */
    class BindIndexBufferCommand : public RenderCommand
    {
    public:
        enum IndexType : uint32_t
        {
            UInt16 = 0,
            UInt32 = 1
        };

        BindIndexBufferCommand(uint32_t buffer, IndexType type, uint64_t offset = 0)
            : m_Buffer(buffer), m_IndexType(type), m_Offset(offset) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BindIndexBuffer"; }
        float GetEstimatedCost() const override { return 0.02f; }

    private:
        uint32_t m_Buffer;
        IndexType m_IndexType;
        uint64_t m_Offset;
    };

    /**
     * @brief Command to bind a shader program
     */
    class BindShaderCommand : public RenderCommand
    {
    public:
        BindShaderCommand(uint32_t program) : m_Program(program) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BindShader"; }
        float GetEstimatedCost() const override { return 0.1f; }

    private:
        uint32_t m_Program;
    };

    /**
     * @brief Command to bind a texture
     */
    class BindTextureCommand : public RenderCommand
    {
    public:
        BindTextureCommand(uint32_t slot, uint32_t texture, uint32_t sampler = 0)
            : m_Slot(slot), m_Texture(texture), m_Sampler(sampler) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BindTexture"; }
        float GetEstimatedCost() const override { return 0.05f; }

    private:
        uint32_t m_Slot;
        uint32_t m_Texture;
        uint32_t m_Sampler;
    };

    // ============================================================================
    // STATE COMMANDS
    // ============================================================================

    /**
     * @brief Command to set depth testing state
     */
    class SetDepthStateCommand : public RenderCommand
    {
    public:
        enum CompareFunction : uint32_t
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

        SetDepthStateCommand(bool testEnabled, bool writeEnabled, CompareFunction func = Less)
            : m_TestEnabled(testEnabled), m_WriteEnabled(writeEnabled), m_CompareFunc(func) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "SetDepthState"; }
        float GetEstimatedCost() const override { return 0.02f; }

    private:
        bool m_TestEnabled;
        bool m_WriteEnabled;
        CompareFunction m_CompareFunc;
    };

    /**
     * @brief Command to set blending state
     */
    class SetBlendStateCommand : public RenderCommand
    {
    public:
        enum BlendFactor : uint32_t
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

        enum BlendOperation : uint32_t
        {
            Add = 0,
            Subtract,
            ReverseSubtract,
            Min,
            Max
        };

        SetBlendStateCommand(bool enabled, BlendFactor srcFactor = SrcAlpha, BlendFactor dstFactor = OneMinusSrcAlpha, 
                            BlendOperation op = Add)
            : m_Enabled(enabled), m_SrcFactor(srcFactor), m_DstFactor(dstFactor), m_BlendOp(op) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "SetBlendState"; }
        float GetEstimatedCost() const override { return 0.02f; }

    private:
        bool m_Enabled;
        BlendFactor m_SrcFactor;
        BlendFactor m_DstFactor;
        BlendOperation m_BlendOp;
    };

    /**
     * @brief Command to set culling state
     */
    class SetCullStateCommand : public RenderCommand
    {
    public:
        enum CullMode : uint32_t
        {
            None = 0,
            Front,
            Back,
            FrontAndBack
        };

        enum FrontFace : uint32_t
        {
            Clockwise = 0,
            CounterClockwise
        };

        SetCullStateCommand(CullMode mode, FrontFace face = CounterClockwise)
            : m_CullMode(mode), m_FrontFace(face) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "SetCullState"; }
        float GetEstimatedCost() const override { return 0.02f; }

    private:
        CullMode m_CullMode;
        FrontFace m_FrontFace;
    };

    // ============================================================================
    // DEBUG COMMANDS
    // ============================================================================

    /**
     * @brief Command to push a debug group for profiling/debugging
     */
    class PushDebugGroupCommand : public RenderCommand
    {
    public:
        PushDebugGroupCommand(const std::string& name) : m_Name(name) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "PushDebugGroup"; }
        float GetEstimatedCost() const override { return 0.001f; }

    private:
        std::string m_Name;
    };

    /**
     * @brief Command to pop a debug group
     */
    class PopDebugGroupCommand : public RenderCommand
    {
    public:
        PopDebugGroupCommand() = default;

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "PopDebugGroup"; }
        float GetEstimatedCost() const override { return 0.001f; }
    };
}
