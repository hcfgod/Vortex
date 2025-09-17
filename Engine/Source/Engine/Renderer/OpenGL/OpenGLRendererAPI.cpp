#include "vxpch.h"
#include "OpenGLRendererAPI.h"

#ifdef VX_USE_SDL
    #include "SDL/SDL_OpenGLGraphicsContext.h"
    // TODO: Support other includes (e.g. GLFW) as needed
#endif // VX_USE_SDL

#include "Core/Debug/Log.h"
#include "Core/Debug/Assert.h"
#include <glad/gl.h>
#include <glm/glm.hpp>

namespace Vortex
{
    // ============================================================================
    // OpenGL Constants and Enums
    // ============================================================================

    // Clear flags
    constexpr uint32_t CLEAR_COLOR_BIT   = 0x01;
    constexpr uint32_t CLEAR_DEPTH_BIT   = 0x02;
    constexpr uint32_t CLEAR_STENCIL_BIT = 0x04;

    // Index types
    constexpr uint32_t INDEX_TYPE_UINT16 = 0;
    constexpr uint32_t INDEX_TYPE_UINT32 = 1;

    // Depth functions
    constexpr uint32_t DEPTH_FUNC_NEVER    = 0;
    constexpr uint32_t DEPTH_FUNC_LESS     = 1;
    constexpr uint32_t DEPTH_FUNC_EQUAL    = 2;
    constexpr uint32_t DEPTH_FUNC_LEQUAL   = 3;
    constexpr uint32_t DEPTH_FUNC_GREATER  = 4;
    constexpr uint32_t DEPTH_FUNC_NOTEQUAL = 5;
    constexpr uint32_t DEPTH_FUNC_GEQUAL   = 6;
    constexpr uint32_t DEPTH_FUNC_ALWAYS   = 7;

    // Blend factors
    constexpr uint32_t BLEND_ZERO                = 0;
    constexpr uint32_t BLEND_ONE                 = 1;
    constexpr uint32_t BLEND_SRC_COLOR           = 2;
    constexpr uint32_t BLEND_ONE_MINUS_SRC_COLOR = 3;
    constexpr uint32_t BLEND_DST_COLOR           = 4;
    constexpr uint32_t BLEND_ONE_MINUS_DST_COLOR = 5;
    constexpr uint32_t BLEND_SRC_ALPHA           = 6;
    constexpr uint32_t BLEND_ONE_MINUS_SRC_ALPHA = 7;
    constexpr uint32_t BLEND_DST_ALPHA           = 8;
    constexpr uint32_t BLEND_ONE_MINUS_DST_ALPHA = 9;

    // Blend operations
    constexpr uint32_t BLEND_OP_ADD              = 0;
    constexpr uint32_t BLEND_OP_SUBTRACT         = 1;
    constexpr uint32_t BLEND_OP_REVERSE_SUBTRACT = 2;
    constexpr uint32_t BLEND_OP_MIN              = 3;
    constexpr uint32_t BLEND_OP_MAX              = 4;

    // Cull modes
    constexpr uint32_t CULL_MODE_NONE  = 0;
    constexpr uint32_t CULL_MODE_FRONT = 1;
    constexpr uint32_t CULL_MODE_BACK  = 2;

    // Front face
    constexpr uint32_t FRONT_FACE_CCW = 0;
    constexpr uint32_t FRONT_FACE_CW  = 1;

    // ============================================================================
    // Constructor/Destructor
    // ============================================================================

    OpenGLRendererAPI::OpenGLRendererAPI()
    {
        VX_CORE_INFO("OpenGL RendererAPI created");
    }

    OpenGLRendererAPI::~OpenGLRendererAPI()
    {
        if (m_Initialized)
        {
            Shutdown();
        }
        VX_CORE_INFO("OpenGL RendererAPI destroyed");
    }

    // ============================================================================
    // Initialization
    // ============================================================================

    Result<void> OpenGLRendererAPI::Initialize(GraphicsContext* context)
    {
        if (m_Initialized)
        {
            VX_CORE_WARN("OpenGL RendererAPI already initialized");
            return Result<void>();
        }

        if (!context)
        {
            VX_CORE_ERROR("Graphics context is null");
            return Result<void>(ErrorCode::NullPointer, "Graphics context is null");
        }

		GraphicsContext* oglCtx = nullptr;

        #ifdef VX_USE_SDL
            // Validate that the provided GraphicsContext is actually an OpenGLGraphicsContext
            oglCtx = dynamic_cast<SDL_OpenGLGraphicsContext*>(context);

            if (!oglCtx)
            {
                VX_CORE_ERROR("Context is not an OpenGLGraphicsContext");
                return Result<void>(ErrorCode::InvalidParameter, "Invalid context type");
            }
        #endif // VX_USE_SDL

		// TODO: Support other context types (e.g. GLFW) as needed

        m_GraphicsContext = context;

        if (!oglCtx->IsValid())
        {
            VX_CORE_ERROR("OpenGL context is not valid");
            return Result<void>(ErrorCode::InvalidState, "OpenGL context is not valid");
        }

        // Ensure context is current
        auto result = oglCtx->MakeCurrent();
        if (!result.IsSuccess())
        {
            return result;
        }

        // Initialize default state
        m_CurrentState.DepthCompareFunc = GL_LESS;
        m_CurrentState.BlendSrcFactor = GL_ONE;
        m_CurrentState.BlendDstFactor = GL_ZERO;
        m_CurrentState.BlendOp = GL_FUNC_ADD;
        m_CurrentState.CullMode = GL_BACK;
        m_CurrentState.FrontFace = GL_CCW;

        // Set initial OpenGL state
        glDepthFunc(GL_LESS);
        glBlendFunc(GL_ONE, GL_ZERO);
        glBlendEquation(GL_FUNC_ADD);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        m_Initialized = true;
        VX_CORE_INFO("OpenGL RendererAPI initialized successfully");

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::Shutdown()
    {
        if (!m_Initialized)
        {
            VX_CORE_WARN("OpenGL RendererAPI not initialized");
            return Result<void>();
        }

        VX_CORE_INFO("Shutting down OpenGL RendererAPI");

        // Reset state
        m_CurrentState = {};
        m_GraphicsContext = nullptr;
        m_Initialized = false;

        VX_CORE_INFO("OpenGL RendererAPI shutdown complete");
        return Result<void>();
    }

    // ============================================================================
    // Basic Rendering Operations
    // ============================================================================

    Result<void> OpenGLRendererAPI::Clear(uint32_t flags, const glm::vec4& color, float depth, int32_t stencil)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        GLbitfield glFlags = 0;

        // glClear is affected by the scissor test. ImGui (and potentially other systems)
        // enable scissor for clipping and may leave it enabled across frames/windows.
        // To guarantee a full-frame clear of the default framebuffer, temporarily
        // disable the scissor test and restore it afterwards.
        GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        GLint prevScissorBox[4] = { 0, 0, 0, 0 };
        if (scissorEnabled)
        {
            glGetIntegerv(GL_SCISSOR_BOX, prevScissorBox);
            glDisable(GL_SCISSOR_TEST);
        }

        if (flags & CLEAR_COLOR_BIT)
        {
            glClearColor(color.r, color.g, color.b, color.a);
            glFlags |= GL_COLOR_BUFFER_BIT;
        }

        if (flags & CLEAR_DEPTH_BIT)
        {
            glClearDepth(depth);
            glFlags |= GL_DEPTH_BUFFER_BIT;
        }

        if (flags & CLEAR_STENCIL_BIT)
        {
            glClearStencil(stencil);
            glFlags |= GL_STENCIL_BUFFER_BIT;
        }

        if (glFlags != 0)
        {
            glClear(glFlags);
        }

        // Restore scissor state after clearing
        if (scissorEnabled)
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(prevScissorBox[0], prevScissorBox[1], prevScissorBox[2], prevScissorBox[3]);
        }

        if (!CheckGLError("Clear"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "OpenGL clear failed");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glViewport(static_cast<GLint>(x), static_cast<GLint>(y), 
                  static_cast<GLsizei>(width), static_cast<GLsizei>(height));

        if (!CheckGLError("SetViewport"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to set viewport");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::SetScissor(bool enabled, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        if (enabled)
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(static_cast<GLint>(x), static_cast<GLint>(y), 
                     static_cast<GLsizei>(width), static_cast<GLsizei>(height));
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }

        if (!CheckGLError("SetScissor"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to set scissor test");
        }

        return Result<void>();
    }

    // ============================================================================
    // Drawing Operations
    // ============================================================================

    Result<void> OpenGLRendererAPI::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, 
                                                uint32_t firstIndex, int32_t baseVertex, uint32_t baseInstance,
                                                uint32_t primitiveTopology)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        // Compute index pointer including bound index buffer offset and firstIndex
        const GLenum glMode = ConvertPrimitiveTopology(primitiveTopology);
        const GLenum glIndexType = (m_CurrentState.CurrentIndexType == INDEX_TYPE_UINT16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
        const size_t indexSize = (glIndexType == GL_UNSIGNED_SHORT) ? sizeof(uint16_t) : sizeof(uint32_t);
        const uintptr_t indexByteOffset = static_cast<uintptr_t>(m_CurrentState.IndexBufferOffset + static_cast<uint64_t>(firstIndex) * static_cast<uint64_t>(indexSize));
        const GLvoid* indicesPtr = reinterpret_cast<const void*>(indexByteOffset);

        // Use BaseVertex variants when baseVertex != 0
        if (instanceCount <= 1)
        {
            if (baseVertex != 0 && (GLAD_GL_VERSION_3_2 || GLAD_GL_ARB_draw_elements_base_vertex))
            {
                glDrawElementsBaseVertex(glMode, static_cast<GLsizei>(indexCount), glIndexType, indicesPtr, static_cast<GLint>(baseVertex));
            }
            else
            {
                glDrawElements(glMode, static_cast<GLsizei>(indexCount), glIndexType, indicesPtr);
            }
        }
        else
        {
            if (baseVertex != 0 && (GLAD_GL_VERSION_3_2 || GLAD_GL_ARB_draw_elements_base_vertex))
            {
                glDrawElementsInstancedBaseVertex(glMode, static_cast<GLsizei>(indexCount), glIndexType,
                    indicesPtr, static_cast<GLsizei>(instanceCount), static_cast<GLint>(baseVertex));
            }
            else
            {
                glDrawElementsInstanced(glMode, static_cast<GLsizei>(indexCount), glIndexType,
                                       indicesPtr,
                                       static_cast<GLsizei>(instanceCount));
            }
        }

        if (!CheckGLError("DrawIndexed"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to draw indexed geometry");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::DrawArrays(uint32_t vertexCount, uint32_t instanceCount,
                                              uint32_t firstVertex, uint32_t baseInstance,
                                              uint32_t primitiveTopology)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        const GLenum glMode = ConvertPrimitiveTopology(primitiveTopology);
        if (instanceCount <= 1)
        {
            glDrawArrays(glMode, static_cast<GLint>(firstVertex), static_cast<GLsizei>(vertexCount));
        }
        else
        {
            glDrawArraysInstanced(glMode, static_cast<GLint>(firstVertex), 
                                 static_cast<GLsizei>(vertexCount), static_cast<GLsizei>(instanceCount));
        }

        if (!CheckGLError("DrawArrays"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to draw arrays");
        }

        return Result<void>();
    }

    // ============================================================================
    // Resource Binding
    // ============================================================================

    Result<void> OpenGLRendererAPI::BindVertexBuffer(uint32_t binding, uint32_t buffer, uint64_t offset, uint64_t stride)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        // For modern OpenGL, we'd use glVertexArrayVertexBuffer
        // For now, use legacy binding
        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        if (!CheckGLError("BindVertexBuffer"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to bind vertex buffer");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BindIndexBuffer(uint32_t buffer, uint32_t indexType, uint64_t offset)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);

        // Track index type and offset for later DrawIndexed
        m_CurrentState.CurrentIndexType = indexType; // 0=UInt16, 1=UInt32
        m_CurrentState.IndexBufferOffset = offset;

        if (!CheckGLError("BindIndexBuffer"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to bind index buffer");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BindBuffer(uint32_t target, uint32_t buffer)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glBindBuffer(ConvertBufferTarget(target), buffer);

        if (!CheckGLError("BindBuffer"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to bind buffer");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BufferData(uint32_t target, const void* data, uint64_t size, uint32_t usage)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glBufferData(ConvertBufferTarget(target), static_cast<GLsizeiptr>(size), data, ConvertBufferUsage(usage));

        if (!CheckGLError("BufferData"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to upload buffer data");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BufferSubData(uint32_t target, uint64_t offset, uint64_t size, const void* data)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glBufferSubData(ConvertBufferTarget(target), static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);

        if (!CheckGLError("BufferSubData"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to update buffer sub data");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BindBufferBase(uint32_t target, uint32_t index, uint32_t buffer)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glBindBufferBase(ConvertBufferTarget(target), index, buffer);

        if (!CheckGLError("BindBufferBase"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to bind buffer base");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::GenBuffers(uint32_t count, uint32_t* outBuffers)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glGenBuffers(static_cast<GLsizei>(count), reinterpret_cast<GLuint*>(outBuffers));

        if (!CheckGLError("GenBuffers"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to generate buffers");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::DeleteBuffers(uint32_t count, const uint32_t* buffers)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glDeleteBuffers(static_cast<GLsizei>(count), reinterpret_cast<const GLuint*>(buffers));

        if (!CheckGLError("DeleteBuffers"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to delete buffers");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::GenVertexArrays(uint32_t count, uint32_t* outArrays)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glGenVertexArrays(static_cast<GLsizei>(count), reinterpret_cast<GLuint*>(outArrays));

        if (!CheckGLError("GenVertexArrays"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to generate vertex arrays");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::DeleteVertexArrays(uint32_t count, const uint32_t* arrays)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glDeleteVertexArrays(static_cast<GLsizei>(count), reinterpret_cast<const GLuint*>(arrays));

        if (!CheckGLError("DeleteVertexArrays"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to delete vertex arrays");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::VertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                                                        bool normalized, uint64_t stride, uint64_t pointer)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glVertexAttribPointer(index, size, ConvertDataType(type), normalized ? GL_TRUE : GL_FALSE,
                              static_cast<GLsizei>(stride), reinterpret_cast<const void*>(static_cast<uintptr_t>(pointer)));

        if (!CheckGLError("VertexAttribPointer"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to set vertex attrib pointer");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::EnableVertexAttribArray(uint32_t index, bool enabled)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        if (enabled) glEnableVertexAttribArray(index); else glDisableVertexAttribArray(index);

        if (!CheckGLError("EnableVertexAttribArray"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to toggle vertex attrib array");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BindShader(uint32_t program)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        if (m_CurrentState.CurrentShaderProgram != program)
        {
            glUseProgram(program);
            m_CurrentState.CurrentShaderProgram = program;
        }

        if (!CheckGLError("BindShader"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to bind shader program");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BindTexture(uint32_t slot, uint32_t texture, uint32_t sampler)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        // Check if this texture is already bound to this slot
        auto it = m_CurrentState.BoundTextures.find(slot);
        if (it != m_CurrentState.BoundTextures.end() && it->second == texture)
        {
            return Result<void>(); // Already bound
        }

        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Update state tracking
        m_CurrentState.BoundTextures[slot] = texture;

        if (!CheckGLError("BindTexture"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to bind texture");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BindVertexArray(uint32_t vao)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        glBindVertexArray(vao);

        if (!CheckGLError("BindVertexArray"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to bind vertex array");
        }

        return Result<void>();
    }

    // ============================================================================
    // Texture Operations
    // ============================================================================

    Result<void> OpenGLRendererAPI::GenTextures(uint32_t count, uint32_t* outTextures)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glGenTextures(static_cast<GLsizei>(count), reinterpret_cast<GLuint*>(outTextures));
        if (!CheckGLError("GenTextures"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to generate textures");
        }
        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::DeleteTextures(uint32_t count, const uint32_t* textures)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glDeleteTextures(static_cast<GLsizei>(count), reinterpret_cast<const GLuint*>(textures));
        if (!CheckGLError("DeleteTextures"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to delete textures");
        }
        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BindTextureTarget(uint32_t target, uint32_t texture)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glBindTexture(ConvertTextureTarget(target), texture);
        if (!CheckGLError("BindTextureTarget"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to bind texture target");
        }
        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::TexImage2D(uint32_t target, int32_t level, uint32_t internalFormat,
                                               uint32_t width, uint32_t height, uint32_t format, uint32_t type,
                                               const void* data)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glTexImage2D(ConvertTextureTarget(target), level, static_cast<GLint>(internalFormat), static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                     format, type, data);
        if (!CheckGLError("TexImage2D"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to specify texture image");
        }
        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::TexParameteri(uint32_t target, uint32_t pname, int32_t param)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glTexParameteri(ConvertTextureTarget(target), ConvertTextureParam(pname), param);
        if (!CheckGLError("TexParameteri"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to set texture parameter");
        }
        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::GenerateMipmap(uint32_t target)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glGenerateMipmap(ConvertTextureTarget(target));
        if (!CheckGLError("GenerateMipmap"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to generate mipmaps");
        }
        return Result<void>();
    }

    // ============================================================================
    // Framebuffers
    // ============================================================================

    Result<void> OpenGLRendererAPI::GenFramebuffers(uint32_t count, uint32_t* outFbos)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glGenFramebuffers(static_cast<GLsizei>(count), reinterpret_cast<GLuint*>(outFbos));
        if (!CheckGLError("GenFramebuffers"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to generate framebuffers");
        }
        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::DeleteFramebuffers(uint32_t count, const uint32_t* fbos)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glDeleteFramebuffers(static_cast<GLsizei>(count), reinterpret_cast<const GLuint*>(fbos));
        if (!CheckGLError("DeleteFramebuffers"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to delete framebuffers");
        }
        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::BindFramebuffer(uint32_t target, uint32_t fbo)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glBindFramebuffer(ConvertFramebufferTarget(target), fbo);
        if (!CheckGLError("BindFramebuffer"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to bind framebuffer");
        }
        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::FramebufferTexture2D(uint32_t target, uint32_t attachment, uint32_t textarget, uint32_t texture, int32_t level)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        glFramebufferTexture2D(ConvertFramebufferTarget(target), ConvertFramebufferAttachment(attachment), ConvertTextureTarget(textarget), texture, level);
        if (!CheckGLError("FramebufferTexture2D"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to attach texture to framebuffer");
        }
        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::CheckFramebufferStatus(uint32_t target, uint32_t* outStatus)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }
        GLenum status = glCheckFramebufferStatus(ConvertFramebufferTarget(target));
        if (outStatus) *outStatus = status;
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            VX_CORE_ERROR("Framebuffer not complete: {}", static_cast<uint32_t>(status));
            return Result<void>(ErrorCode::RendererInitFailed, "Framebuffer incomplete");
        }
        return Result<void>();
    }

    // ============================================================================
    // Render State
    // ============================================================================

    Result<void> OpenGLRendererAPI::SetDepthState(bool testEnabled, bool writeEnabled, uint32_t compareFunc)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        // Update depth test enable/disable
        if (m_CurrentState.DepthTestEnabled != testEnabled)
        {
            if (testEnabled)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
            
            m_CurrentState.DepthTestEnabled = testEnabled;
        }

        // Update depth write
        if (m_CurrentState.DepthWriteEnabled != writeEnabled)
        {
            glDepthMask(writeEnabled ? GL_TRUE : GL_FALSE);
            m_CurrentState.DepthWriteEnabled = writeEnabled;
        }

        // Update depth function
        uint32_t glDepthFuncValue = ConvertDepthFunc(compareFunc);
        if (m_CurrentState.DepthCompareFunc != glDepthFuncValue)
        {
            glDepthFunc(glDepthFuncValue);
            m_CurrentState.DepthCompareFunc = glDepthFuncValue;
        }

        if (!CheckGLError("SetDepthState"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to set depth state");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::SetBlendState(bool enabled, uint32_t srcFactor, uint32_t dstFactor, uint32_t blendOp)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        // Update blend enable/disable
        if (m_CurrentState.BlendEnabled != enabled)
        {
            if (enabled)
                glEnable(GL_BLEND);
            else
                glDisable(GL_BLEND);
            
            m_CurrentState.BlendEnabled = enabled;
        }

        if (enabled)
        {
            uint32_t glSrcFactor = ConvertBlendFactor(srcFactor);
            uint32_t glDstFactor = ConvertBlendFactor(dstFactor);
            uint32_t glBlendOp = ConvertBlendOp(blendOp);

            // Update blend function
            if (m_CurrentState.BlendSrcFactor != glSrcFactor || m_CurrentState.BlendDstFactor != glDstFactor)
            {
                glBlendFunc(glSrcFactor, glDstFactor);
                m_CurrentState.BlendSrcFactor = glSrcFactor;
                m_CurrentState.BlendDstFactor = glDstFactor;
            }

            // Update blend equation
            if (m_CurrentState.BlendOp != glBlendOp)
            {
                glBlendEquation(glBlendOp);
                m_CurrentState.BlendOp = glBlendOp;
            }
        }

        if (!CheckGLError("SetBlendState"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to set blend state");
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::SetCullState(uint32_t cullMode, uint32_t frontFace)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        uint32_t glCullMode = ConvertCullMode(cullMode);
        uint32_t glFrontFaceMode = ConvertFrontFace(frontFace);

        // Update culling enable/disable using tracked state to avoid extra GL calls
        bool cullEnabled = (cullMode != CULL_MODE_NONE);
        if (m_CurrentState.CullEnabled != cullEnabled)
        {
            if (cullEnabled)
                glEnable(GL_CULL_FACE);
            else
                glDisable(GL_CULL_FACE);
            m_CurrentState.CullEnabled = cullEnabled;
        }

        if (cullEnabled)
        {
            // Update cull face
            if (m_CurrentState.CullMode != glCullMode)
            {
                glCullFace(glCullMode);
                m_CurrentState.CullMode = glCullMode;
            }
        }

        // Update front face
        if (m_CurrentState.FrontFace != glFrontFaceMode)
        {
            ::glFrontFace(glFrontFaceMode);
            m_CurrentState.FrontFace = glFrontFaceMode;
        }

        if (!CheckGLError("SetCullState"))
        {
            return Result<void>(ErrorCode::RendererInitFailed, "Failed to set cull state");
        }

        return Result<void>();
    }

    // ============================================================================
    // Debug and Profiling
    // ============================================================================

    Result<void> OpenGLRendererAPI::PushDebugGroup(const std::string& name)
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        if (GLAD_GL_KHR_debug)
        {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<GLsizei>(name.length()), name.c_str());
        }

        return Result<void>();
    }

    Result<void> OpenGLRendererAPI::PopDebugGroup()
    {
        auto validateResult = ValidateContext();
        if (!validateResult.IsSuccess())
        {
            return validateResult;
        }

        if (GLAD_GL_KHR_debug)
        {
            glPopDebugGroup();
        }

        return Result<void>();
    }

    std::string OpenGLRendererAPI::GetDebugInfo() const
    {
        #ifdef VX_USE_SDL
            const SDL_OpenGLGraphicsContext* oglCtx = dynamic_cast<const SDL_OpenGLGraphicsContext*>(GetGraphicsContext());
            if (!m_Initialized || !oglCtx)
            {
                return "OpenGL RendererAPI not initialized";
            }
            std::ostringstream info;
            info << "OpenGL RendererAPI Debug Info:\n";
            info << "  Vendor: " << oglCtx->GetOpenGLVendor() << "\n";
            info << "  Renderer: " << oglCtx->GetOpenGLRenderer() << "\n";
            info << "  Version: " << oglCtx->GetOpenGLVersion() << "\n";
            info << "  Depth Test: " << (m_CurrentState.DepthTestEnabled ? "Enabled" : "Disabled") << "\n";
            info << "  Blend: " << (m_CurrentState.BlendEnabled ? "Enabled" : "Disabled") << "\n";
            info << "  Current Shader: " << m_CurrentState.CurrentShaderProgram << "\n";
            info << "  Bound Textures: " << m_CurrentState.BoundTextures.size();
            return info.str();
        #endif // VX_USE_SDL

		// TODO: Support other context types (e.g. GLFW) as needed

		return "OpenGL RendererAPI not initialized";
    }

    // ============================================================================
    // Private Helper Methods
    // ============================================================================

    Result<void> OpenGLRendererAPI::ValidateContext() const
    {
        if (!m_Initialized)
        {
            return Result<void>(ErrorCode::InvalidState, "RendererAPI not initialized");
        }

        #ifdef VX_USE_SDL
            const SDL_OpenGLGraphicsContext* oglCtx = dynamic_cast<const SDL_OpenGLGraphicsContext*>(GetGraphicsContext());
            if (!oglCtx || !oglCtx->IsValid())
            {
                return Result<void>(ErrorCode::InvalidState, "OpenGL context is not valid");
            }
        #endif // VX_USE_SDL

        // TODO: Support other context types (e.g. GLFW) as needed

        return Result<void>();
    }

    uint32_t OpenGLRendererAPI::ConvertBlendFactor(uint32_t factor) const
    {
        switch (factor)
        {
            case BLEND_ZERO:                return GL_ZERO;
            case BLEND_ONE:                 return GL_ONE;
            case BLEND_SRC_COLOR:           return GL_SRC_COLOR;
            case BLEND_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
            case BLEND_DST_COLOR:           return GL_DST_COLOR;
            case BLEND_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
            case BLEND_SRC_ALPHA:           return GL_SRC_ALPHA;
            case BLEND_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
            case BLEND_DST_ALPHA:           return GL_DST_ALPHA;
            case BLEND_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
            default:                        return GL_ONE;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertBlendOp(uint32_t op) const
    {
        switch (op)
        {
            case BLEND_OP_ADD:              return GL_FUNC_ADD;
            case BLEND_OP_SUBTRACT:         return GL_FUNC_SUBTRACT;
            case BLEND_OP_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
            case BLEND_OP_MIN:              return GL_MIN;
            case BLEND_OP_MAX:              return GL_MAX;
            default:                        return GL_FUNC_ADD;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertDepthFunc(uint32_t func) const
    {
        switch (func)
        {
            case DEPTH_FUNC_NEVER:    return GL_NEVER;
            case DEPTH_FUNC_LESS:     return GL_LESS;
            case DEPTH_FUNC_EQUAL:    return GL_EQUAL;
            case DEPTH_FUNC_LEQUAL:   return GL_LEQUAL;
            case DEPTH_FUNC_GREATER:  return GL_GREATER;
            case DEPTH_FUNC_NOTEQUAL: return GL_NOTEQUAL;
            case DEPTH_FUNC_GEQUAL:   return GL_GEQUAL;
            case DEPTH_FUNC_ALWAYS:   return GL_ALWAYS;
            default:                  return GL_LESS;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertCullMode(uint32_t mode) const
    {
        switch (mode)
        {
            case CULL_MODE_FRONT: return GL_FRONT;
            case CULL_MODE_BACK:  return GL_BACK;
            case CULL_MODE_NONE:
            default:              return GL_BACK;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertFrontFace(uint32_t face) const
    {
        switch (face)
        {
            case FRONT_FACE_CW:  return GL_CW;
            case FRONT_FACE_CCW: return GL_CCW;
            default:             return GL_CCW;
        }
    }

    std::string OpenGLRendererAPI::GetGLErrorString(uint32_t error) const
    {
        switch (error)
        {
            case GL_NO_ERROR:                      return "GL_NO_ERROR";
            case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
            case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
            case GL_STACK_OVERFLOW:                return "GL_STACK_OVERFLOW";
            case GL_STACK_UNDERFLOW:               return "GL_STACK_UNDERFLOW";
            case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
            case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
            default:                               return fmt::format("Unknown error {}", error);
        }
    }

    bool OpenGLRendererAPI::CheckGLError(const std::string& operation) const
    {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            VX_CORE_ERROR("OpenGL Error in {}: {}", operation, GetGLErrorString(error));
            return false;
        }
        return true;
    }

    uint32_t OpenGLRendererAPI::ConvertBufferTarget(uint32_t target) const
    {
        switch (static_cast<BufferTarget>(target))
        {
            case BufferTarget::ArrayBuffer:          return GL_ARRAY_BUFFER;
            case BufferTarget::ElementArrayBuffer:   return GL_ELEMENT_ARRAY_BUFFER;
            case BufferTarget::UniformBuffer:        return GL_UNIFORM_BUFFER;
            case BufferTarget::ShaderStorageBuffer:  return GL_SHADER_STORAGE_BUFFER;
            default:                                  return GL_ARRAY_BUFFER;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertBufferUsage(uint32_t usage) const
    {
        switch (static_cast<BufferUsage>(usage))
        {
            case BufferUsage::StaticDraw:  return GL_STATIC_DRAW;
            case BufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
            case BufferUsage::StreamDraw:  return GL_STREAM_DRAW;
            default:                       return GL_STATIC_DRAW;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertDataType(uint32_t type) const
    {
        switch (static_cast<DataType>(type))
        {
            case DataType::Byte:          return GL_BYTE;
            case DataType::UnsignedByte:  return GL_UNSIGNED_BYTE;
            case DataType::Short:         return GL_SHORT;
            case DataType::UnsignedShort: return GL_UNSIGNED_SHORT;
            case DataType::Int:           return GL_INT;
            case DataType::UnsignedInt:   return GL_UNSIGNED_INT;
            case DataType::HalfFloat:     return GL_HALF_FLOAT;
            case DataType::Float:         return GL_FLOAT;
            case DataType::DoubleType:    return GL_DOUBLE;
            default:                      return GL_FLOAT;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertPrimitiveTopology(uint32_t topology) const
    {
        switch (static_cast<PrimitiveTopology>(topology))
        {
            case PrimitiveTopology::Points:         return GL_POINTS;
            case PrimitiveTopology::Lines:          return GL_LINES;
            case PrimitiveTopology::LineStrip:      return GL_LINE_STRIP;
            case PrimitiveTopology::LineLoop:       return GL_LINE_LOOP;
            case PrimitiveTopology::Triangles:      return GL_TRIANGLES;
            case PrimitiveTopology::TriangleStrip:  return GL_TRIANGLE_STRIP;
            case PrimitiveTopology::TriangleFan:    return GL_TRIANGLE_FAN;
            default:                                return GL_TRIANGLES;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertTextureTarget(uint32_t target) const
    {
        switch (static_cast<TextureTarget>(target))
        {
            case TextureTarget::Texture2D: return GL_TEXTURE_2D;
            default: return GL_TEXTURE_2D;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertTextureParam(uint32_t pname) const
    {
        switch (static_cast<TextureParamName>(pname))
        {
            case TextureParamName::MinFilter: return GL_TEXTURE_MIN_FILTER;
            case TextureParamName::MagFilter: return GL_TEXTURE_MAG_FILTER;
            case TextureParamName::WrapS:     return GL_TEXTURE_WRAP_S;
            case TextureParamName::WrapT:     return GL_TEXTURE_WRAP_T;
            default: return GL_TEXTURE_MIN_FILTER;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertFramebufferTarget(uint32_t target) const
    {
        switch (static_cast<FramebufferTarget>(target))
        {
            case FramebufferTarget::Framebuffer:      return GL_FRAMEBUFFER;
            case FramebufferTarget::ReadFramebuffer:  return GL_READ_FRAMEBUFFER;
            case FramebufferTarget::DrawFramebuffer:  return GL_DRAW_FRAMEBUFFER;
            default:                                  return GL_FRAMEBUFFER;
        }
    }

    uint32_t OpenGLRendererAPI::ConvertFramebufferAttachment(uint32_t attachment) const
    {
        switch (static_cast<FramebufferAttachment>(attachment))
        {
            case FramebufferAttachment::Color0:       return GL_COLOR_ATTACHMENT0;
            case FramebufferAttachment::Color1:       return GL_COLOR_ATTACHMENT1;
            case FramebufferAttachment::Color2:       return GL_COLOR_ATTACHMENT2;
            case FramebufferAttachment::Color3:       return GL_COLOR_ATTACHMENT3;
            case FramebufferAttachment::Depth:        return GL_DEPTH_ATTACHMENT;
            case FramebufferAttachment::Stencil:      return GL_STENCIL_ATTACHMENT;
            case FramebufferAttachment::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
            default:                                  return GL_COLOR_ATTACHMENT0;
        }
    }
}
