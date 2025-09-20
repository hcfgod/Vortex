#pragma once

#include "Core/Debug/ErrorCodes.h"
#include "RenderTypes.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

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
                          uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t baseInstance = 0,
                          uint32_t primitiveTopology = static_cast<uint32_t>(PrimitiveTopology::Triangles))
            : m_IndexCount(indexCount), m_InstanceCount(instanceCount)
            , m_FirstIndex(firstIndex), m_BaseVertex(baseVertex), m_BaseInstance(baseInstance)
            , m_PrimitiveTopology(primitiveTopology) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "DrawIndexed"; }
        float GetEstimatedCost() const override { return m_IndexCount / 3.0f * m_InstanceCount * 0.001f; }

    private:
        uint32_t m_IndexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstIndex;
        int32_t m_BaseVertex;
        uint32_t m_BaseInstance;
        uint32_t m_PrimitiveTopology;
    };

    /**
     * @brief Command to draw arrays (non-indexed)
     */
    class DrawArraysCommand : public RenderCommand
    {
    public:
        DrawArraysCommand(uint32_t vertexCount, uint32_t instanceCount = 1, 
                         uint32_t firstVertex = 0, uint32_t baseInstance = 0,
                         uint32_t primitiveTopology = static_cast<uint32_t>(PrimitiveTopology::Triangles))
            : m_VertexCount(vertexCount), m_InstanceCount(instanceCount)
            , m_FirstVertex(firstVertex), m_BaseInstance(baseInstance)
            , m_PrimitiveTopology(primitiveTopology) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "DrawArrays"; }
        float GetEstimatedCost() const override { return m_VertexCount / 3.0f * m_InstanceCount * 0.001f; }

    private:
        uint32_t m_VertexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstVertex;
        uint32_t m_BaseInstance;
        uint32_t m_PrimitiveTopology;
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
     * @brief Command to bind a buffer to a target
     */
    class BindBufferCommand : public RenderCommand
    {
    public:
        BindBufferCommand(uint32_t target, uint32_t buffer)
            : m_Target(target), m_Buffer(buffer) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BindBuffer"; }
        float GetEstimatedCost() const override { return 0.01f; }

    private:
        uint32_t m_Target;
        uint32_t m_Buffer;
    };

    /**
     * @brief Command to upload data to a buffer
     */
    class BufferDataCommand : public RenderCommand
    {
    public:
        using ByteVector = std::vector<uint8_t>;

        BufferDataCommand(uint32_t target, std::shared_ptr<ByteVector> payload, uint32_t usage)
            : m_Target(target), m_Payload(std::move(payload)), m_Size(m_Payload ? static_cast<uint64_t>(m_Payload->size()) : 0ull), m_Usage(usage) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BufferData"; }
        float GetEstimatedCost() const override { return static_cast<float>(m_Size) * 1e-9f; }

    private:
        uint32_t m_Target;
        std::shared_ptr<ByteVector> m_Payload;
        uint64_t m_Size;
        uint32_t m_Usage;
    };

    /**
     * @brief Command to update a sub-range of a buffer
     */
    class BufferSubDataCommand : public RenderCommand
    {
    public:
        using ByteVector = std::vector<uint8_t>;

        BufferSubDataCommand(uint32_t target, uint64_t offset, std::shared_ptr<ByteVector> payload)
            : m_Target(target), m_Offset(offset), m_Payload(std::move(payload)), m_Size(m_Payload ? static_cast<uint64_t>(m_Payload->size()) : 0ull) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BufferSubData"; }
        float GetEstimatedCost() const override { return static_cast<float>(m_Size) * 1e-9f; }

    private:
        uint32_t m_Target;
        uint64_t m_Offset;
        std::shared_ptr<ByteVector> m_Payload;
        uint64_t m_Size;
    };

    /**
     * @brief Command to set vertex attribute pointer
     */
    class VertexAttribPointerCommand : public RenderCommand
    {
    public:
        VertexAttribPointerCommand(uint32_t index, int32_t size, uint32_t type, bool normalized,
                                   uint64_t stride, uint64_t pointer)
            : m_Index(index), m_Size(size), m_Type(type), m_Normalized(normalized),
              m_Stride(stride), m_Pointer(pointer) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "VertexAttribPointer"; }
        float GetEstimatedCost() const override { return 0.01f; }

    private:
        uint32_t m_Index;
        int32_t m_Size;
        uint32_t m_Type;
        bool m_Normalized;
        uint64_t m_Stride;
        uint64_t m_Pointer;
    };

    // Integer variant (no normalization)
    class VertexAttribIPointerCommand : public RenderCommand
    {
    public:
        VertexAttribIPointerCommand(uint32_t index, int32_t size, uint32_t type,
                                    uint64_t stride, uint64_t pointer)
            : m_Index(index), m_Size(size), m_Type(type), m_Stride(stride), m_Pointer(pointer) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "VertexAttribIPointer"; }
        float GetEstimatedCost() const override { return 0.01f; }

    private:
        uint32_t m_Index;
        int32_t m_Size;
        uint32_t m_Type;
        uint64_t m_Stride;
        uint64_t m_Pointer;
    };

    // Attribute divisor (instancing)
    class VertexAttribDivisorCommand : public RenderCommand
    {
    public:
        VertexAttribDivisorCommand(uint32_t index, uint32_t divisor)
            : m_Index(index), m_Divisor(divisor) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "VertexAttribDivisor"; }
        float GetEstimatedCost() const override { return 0.005f; }

    private:
        uint32_t m_Index;
        uint32_t m_Divisor;
    };

    /**
     * @brief Command to enable/disable vertex attribute array
     */
    class EnableVertexAttribArrayCommand : public RenderCommand
    {
    public:
        EnableVertexAttribArrayCommand(uint32_t index, bool enabled)
            : m_Index(index), m_Enabled(enabled) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "EnableVertexAttribArray"; }
        float GetEstimatedCost() const override { return 0.005f; }

    private:
        uint32_t m_Index;
        bool m_Enabled;
    };

    /**
     * @brief Command to bind a vertex array object (VAO)
     */
    class BindVertexArrayCommand : public RenderCommand
    {
    public:
        explicit BindVertexArrayCommand(uint32_t vao) : m_VAO(vao) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BindVertexArray"; }
        float GetEstimatedCost() const override { return 0.01f; }

    private:
        uint32_t m_VAO;
    };

    // ============================================================================
    // OBJECT LIFETIME COMMANDS
    // ============================================================================

    class GenBuffersCommand : public RenderCommand
    {
    public:
        GenBuffersCommand(uint32_t count, uint32_t* outBuffers)
            : m_Count(count), m_OutBuffers(outBuffers) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "GenBuffers"; }
        float GetEstimatedCost() const override { return 0.01f * m_Count; }

    private:
        uint32_t m_Count;
        uint32_t* m_OutBuffers;
    };

    class DeleteBuffersCommand : public RenderCommand
    {
    public:
        DeleteBuffersCommand(uint32_t count, const uint32_t* buffers)
            : m_Count(count), m_Buffers(buffers) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "DeleteBuffers"; }
        float GetEstimatedCost() const override { return 0.01f * m_Count; }

    private:
        uint32_t m_Count;
        const uint32_t* m_Buffers;
    };

    class GenVertexArraysCommand : public RenderCommand
    {
    public:
        GenVertexArraysCommand(uint32_t count, uint32_t* outArrays)
            : m_Count(count), m_OutArrays(outArrays) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "GenVertexArrays"; }
        float GetEstimatedCost() const override { return 0.01f * m_Count; }

    private:
        uint32_t m_Count;
        uint32_t* m_OutArrays;
    };

    class DeleteVertexArraysCommand : public RenderCommand
    {
    public:
        DeleteVertexArraysCommand(uint32_t count, const uint32_t* arrays)
            : m_Count(count), m_Arrays(arrays) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "DeleteVertexArrays"; }
        float GetEstimatedCost() const override { return 0.01f * m_Count; }

    private:
        uint32_t m_Count;
        const uint32_t* m_Arrays;
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

    /**
     * @brief Bind a buffer to an indexed binding point (UBO/SSBO)
     */
    class BindBufferBaseCommand : public RenderCommand
    {
    public:
        BindBufferBaseCommand(uint32_t target, uint32_t index, uint32_t buffer)
            : m_Target(target), m_Index(index), m_Buffer(buffer) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BindBufferBase"; }
        float GetEstimatedCost() const override { return 0.01f; }

    private:
        uint32_t m_Target;
        uint32_t m_Index;
        uint32_t m_Buffer;
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

    // ============================================================================
    // TEXTURE COMMANDS
    // ============================================================================

    /**
     * @brief Generate one or more texture object names
     */
    class GenTexturesCommand : public RenderCommand
    {
    public:
        GenTexturesCommand(uint32_t count, uint32_t* outTextures)
            : m_Count(count), m_OutTextures(outTextures) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "GenTextures"; }
        float GetEstimatedCost() const override { return 0.01f * m_Count; }

    private:
        uint32_t m_Count;
        uint32_t* m_OutTextures;
    };

    /**
     * @brief Delete one or more textures
     */
    class DeleteTexturesCommand : public RenderCommand
    {
    public:
        DeleteTexturesCommand(uint32_t count, const uint32_t* textures)
            : m_Count(count), m_Textures(textures) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "DeleteTextures"; }
        float GetEstimatedCost() const override { return 0.01f * m_Count; }

    private:
        uint32_t m_Count;
        const uint32_t* m_Textures;
    };

    /**
     * @brief Bind a texture to a target (e.g., GL_TEXTURE_2D)
     */
    class BindTextureTargetCommand : public RenderCommand
    {
    public:
        BindTextureTargetCommand(uint32_t target, uint32_t texture)
            : m_Target(target), m_Texture(texture) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BindTextureTarget"; }
        float GetEstimatedCost() const override { return 0.01f; }

    private:
        uint32_t m_Target;
        uint32_t m_Texture;
    };

    /**
     * @brief Upload a 2D texture image
     */
    class TexImage2DCommand : public RenderCommand
    {
    public:
        using ByteVector = std::vector<uint8_t>;

        TexImage2DCommand(uint32_t target,
                          int32_t level,
                          uint32_t internalFormat,
                          uint32_t width,
                          uint32_t height,
                          uint32_t format,
                          uint32_t type,
                          std::shared_ptr<ByteVector> payload)
            : m_Target(target), m_Level(level), m_InternalFormat(internalFormat),
              m_Width(width), m_Height(height), m_Format(format), m_Type(type),
              m_Payload(std::move(payload)) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "TexImage2D"; }
        float GetEstimatedCost() const override { return static_cast<float>(m_Width) * static_cast<float>(m_Height) * 1e-6f; }

    private:
        uint32_t m_Target;
        int32_t m_Level;
        uint32_t m_InternalFormat;
        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_Format;
        uint32_t m_Type;
        std::shared_ptr<ByteVector> m_Payload;
    };

    /**
     * @brief Set integer texture parameter
     */
    class TexParameteriCommand : public RenderCommand
    {
    public:
        TexParameteriCommand(uint32_t target, uint32_t pname, int32_t param)
            : m_Target(target), m_PName(pname), m_Param(param) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "TexParameteri"; }
        float GetEstimatedCost() const override { return 0.005f; }

    private:
        uint32_t m_Target;
        uint32_t m_PName;
        int32_t m_Param;
    };

    /**
     * @brief Generate mipmaps for a texture target
     */
    class GenerateMipmapCommand : public RenderCommand
    {
    public:
        explicit GenerateMipmapCommand(uint32_t target)
            : m_Target(target) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "GenerateMipmap"; }
        float GetEstimatedCost() const override { return 0.05f; }

    private:
        uint32_t m_Target;
    };

    // ============================================================================
    // FRAMEBUFFER COMMANDS
    // ============================================================================

    class GenFramebuffersCommand : public RenderCommand
    {
    public:
        GenFramebuffersCommand(uint32_t count, uint32_t* outFbos)
            : m_Count(count), m_OutFbos(outFbos) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "GenFramebuffers"; }
        float GetEstimatedCost() const override { return 0.01f * m_Count; }

    private:
        uint32_t m_Count;
        uint32_t* m_OutFbos;
    };

    class DeleteFramebuffersCommand : public RenderCommand
    {
    public:
        DeleteFramebuffersCommand(uint32_t count, const uint32_t* fbos)
            : m_Count(count), m_Fbos(fbos) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "DeleteFramebuffers"; }
        float GetEstimatedCost() const override { return 0.01f * m_Count; }

    private:
        uint32_t m_Count;
        const uint32_t* m_Fbos;
    };

    class BindFramebufferCommand : public RenderCommand
    {
    public:
        BindFramebufferCommand(uint32_t target, uint32_t fbo)
            : m_Target(target), m_FBO(fbo) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "BindFramebuffer"; }
        float GetEstimatedCost() const override { return 0.01f; }

    private:
        uint32_t m_Target;
        uint32_t m_FBO;
    };

    class FramebufferTexture2DCommand : public RenderCommand
    {
    public:
        FramebufferTexture2DCommand(uint32_t target, uint32_t attachment, uint32_t textarget, uint32_t texture, int32_t level)
            : m_Target(target), m_Attachment(attachment), m_TexTarget(textarget), m_Texture(texture), m_Level(level) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "FramebufferTexture2D"; }
        float GetEstimatedCost() const override { return 0.02f; }

    private:
        uint32_t m_Target;
        uint32_t m_Attachment;
        uint32_t m_TexTarget;
        uint32_t m_Texture;
        int32_t m_Level;
    };

    class CheckFramebufferStatusCommand : public RenderCommand
    {
    public:
        CheckFramebufferStatusCommand(uint32_t target, uint32_t* outStatus)
            : m_Target(target), m_OutStatus(outStatus) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "CheckFramebufferStatus"; }
        float GetEstimatedCost() const override { return 0.005f; }

    private:
        uint32_t m_Target;
        uint32_t* m_OutStatus;
    };

    class SetDrawBuffersCommand : public RenderCommand
    {
    public:
        SetDrawBuffersCommand(uint32_t count, const uint32_t* attachments)
            : m_Count(count), m_Attachments(attachments) {}

        Result<void> Execute(GraphicsContext* context) override;
        std::string GetDebugName() const override { return "SetDrawBuffers"; }
        float GetEstimatedCost() const override { return 0.01f * m_Count; }

    private:
        uint32_t m_Count;
        const uint32_t* m_Attachments;
    };
}
