#include "vxpch.h"
#include "RenderCommand.h"
#include "RendererAPI.h"
#include "GraphicsContext.h"
#include "Core/Debug/Log.h"

namespace Vortex 
{

    // --------------------------- ClearCommand ---------------------------
    Result<void> ClearCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->Clear(m_Flags, m_ClearColor, m_DepthValue, m_StencilValue);
    }

    // ------------------------ SetViewportCommand ------------------------
    Result<void> SetViewportCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->SetViewport(m_X, m_Y, m_Width, m_Height);
    }

    // ------------------------- SetScissorCommand ------------------------
    Result<void> SetScissorCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->SetScissor(m_Enabled, m_X, m_Y, m_Width, m_Height);
    }

    // ------------------------- DrawIndexedCommand ------------------------
    Result<void> DrawIndexedCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->DrawIndexed(m_IndexCount, m_InstanceCount, m_FirstIndex, m_BaseVertex, m_BaseInstance, m_PrimitiveTopology);
    }

    // ------------------------- DrawArraysCommand -------------------------
    Result<void> DrawArraysCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->DrawArrays(m_VertexCount, m_InstanceCount, m_FirstVertex, m_BaseInstance, m_PrimitiveTopology);
    }

    // ---------------------- BindVertexBufferCommand ----------------------
    Result<void> BindVertexBufferCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->BindVertexBuffer(m_Binding, m_Buffer, m_Offset, m_Stride);
    }

    // ----------------------- BindIndexBufferCommand ----------------------
    Result<void> BindIndexBufferCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        uint32_t type = (m_IndexType == IndexType::UInt16) ? 0u : 1u; // matches OpenGLRendererAPI mapping
        return renderer->BindIndexBuffer(m_Buffer, type, m_Offset);
    }

    // --------------------------- BindBufferCommand -----------------------
    Result<void> BindBufferCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->BindBuffer(m_Target, m_Buffer);
    }

    // --------------------------- BufferDataCommand -----------------------
    Result<void> BufferDataCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        const void* dataPtr = (m_Payload && !m_Payload->empty()) ? static_cast<const void*>(m_Payload->data()) : nullptr;
        return renderer->BufferData(m_Target, dataPtr, m_Size, m_Usage);
    }

    Result<void> BufferSubDataCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        const void* dataPtr = (m_Payload && !m_Payload->empty()) ? static_cast<const void*>(m_Payload->data()) : nullptr;
        return renderer->BufferSubData(m_Target, m_Offset, m_Size, dataPtr);
    }

    // --------------------- VertexAttribPointerCommand --------------------
    Result<void> VertexAttribPointerCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->VertexAttribPointer(m_Index, m_Size, m_Type, m_Normalized, m_Stride, m_Pointer);
    }

    Result<void> VertexAttribIPointerCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->VertexAttribIPointer(m_Index, m_Size, m_Type, m_Stride, m_Pointer);
    }

    Result<void> VertexAttribDivisorCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->VertexAttribDivisor(m_Index, m_Divisor);
    }

    // ----------------- EnableVertexAttribArrayCommand --------------------
    Result<void> EnableVertexAttribArrayCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->EnableVertexAttribArray(m_Index, m_Enabled);
    }

    // ----------------------- BindVertexArrayCommand ----------------------
    Result<void> BindVertexArrayCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->BindVertexArray(m_VAO);
    }

    // -------------------------- BindShaderCommand ------------------------
    Result<void> BindShaderCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->BindShader(m_Program);
    }

    // ------------------------- BindTextureCommand ------------------------
    Result<void> BindTextureCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->BindTexture(m_Slot, m_Texture, m_Sampler);
    }

    Result<void> BindBufferBaseCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->BindBufferBase(m_Target, m_Index, m_Buffer);
    }

    // ------------------------ SetDepthStateCommand -----------------------
    Result<void> SetDepthStateCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        uint32_t func = static_cast<uint32_t>(m_CompareFunc); // matches OpenGLRendererAPI mapping 0..7
        return renderer->SetDepthState(m_TestEnabled, m_WriteEnabled, func);
    }

    // ------------------------ SetBlendStateCommand -----------------------
    Result<void> SetBlendStateCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");

        // Map to renderer's factor/op enums (OpenGLRendererAPI expects 0..9 for factors, 0..4 for ops)
        auto mapBlendFactor = [](BlendFactor f) -> uint32_t {
            switch (f) {
                case Zero: return 0; case One: return 1; case SrcColor: return 2; case OneMinusSrcColor: return 3;
                case DstColor: return 4; case OneMinusDstColor: return 5; case SrcAlpha: return 6; case OneMinusSrcAlpha: return 7;
                case DstAlpha: return 8; case OneMinusDstAlpha: return 9; default: return 1; // default to One
            }
        };
        auto mapBlendOp = [](BlendOperation op) -> uint32_t {
            switch (op) {
                case Add: return 0; case Subtract: return 1; case ReverseSubtract: return 2; case Min: return 3; case Max: return 4;
                default: return 0;
            }
        };

        return renderer->SetBlendState(m_Enabled, mapBlendFactor(m_SrcFactor), mapBlendFactor(m_DstFactor), mapBlendOp(m_BlendOp));
    }

    // -------------------------- SetCullStateCommand ----------------------
    Result<void> SetCullStateCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        auto mapCullMode = [](CullMode m) -> uint32_t {
            switch (m) { case None: return 0; case Front: return 1; case Back: return 2; case FrontAndBack: return 2; }
            return 2;
        };
        auto mapFrontFace = [](FrontFace f) -> uint32_t { return (f == CounterClockwise) ? 0u : 1u; };
        return renderer->SetCullState(mapCullMode(m_CullMode), mapFrontFace(m_FrontFace));
    }

    // ------------------------ Debug group commands -----------------------
    Result<void> PushDebugGroupCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->PushDebugGroup(m_Name);
    }

    Result<void> PopDebugGroupCommand::Execute(GraphicsContext* /*context*/) {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->PopDebugGroup();
    }

    // --------------------------- Object lifetime ---------------------------
    Result<void> GenBuffersCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->GenBuffers(m_Count, m_OutBuffers);
    }

    Result<void> DeleteBuffersCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->DeleteBuffers(m_Count, m_Buffers);
    }

    Result<void> GenVertexArraysCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->GenVertexArrays(m_Count, m_OutArrays);
    }

    Result<void> DeleteVertexArraysCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->DeleteVertexArrays(m_Count, m_Arrays);
    }

    // ----------------------------- Texture commands -----------------------------
    Result<void> GenTexturesCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->GenTextures(m_Count, m_OutTextures);
    }

    Result<void> DeleteTexturesCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->DeleteTextures(m_Count, m_Textures);
    }

    Result<void> BindTextureTargetCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->BindTextureTarget(m_Target, m_Texture);
    }

    Result<void> TexImage2DCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        const void* dataPtr = (m_Payload && !m_Payload->empty()) ? static_cast<const void*>(m_Payload->data()) : nullptr;
        return renderer->TexImage2D(m_Target, m_Level, m_InternalFormat, m_Width, m_Height, m_Format, m_Type, dataPtr);
    }

    Result<void> TexParameteriCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->TexParameteri(m_Target, m_PName, m_Param);
    }

    Result<void> GenerateMipmapCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->GenerateMipmap(m_Target);
    }

    // ----------------------------- Framebuffer commands -----------------------------
    Result<void> GenFramebuffersCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->GenFramebuffers(m_Count, m_OutFbos);
    }

    Result<void> DeleteFramebuffersCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->DeleteFramebuffers(m_Count, m_Fbos);
    }

    Result<void> BindFramebufferCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->BindFramebuffer(m_Target, m_FBO);
    }

    Result<void> FramebufferTexture2DCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->FramebufferTexture2D(m_Target, m_Attachment, m_TexTarget, m_Texture, m_Level);
    }

    Result<void> CheckFramebufferStatusCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->CheckFramebufferStatus(m_Target, m_OutStatus);
    }

    Result<void> SetDrawBuffersCommand::Execute(GraphicsContext* /*context*/)
    {
        auto* renderer = GetRenderer();
        if (!renderer) return Result<void>(ErrorCode::InvalidState, "Renderer not initialized");
        return renderer->SetDrawBuffers(m_Count, m_Attachments);
    }

} // namespace Vortex