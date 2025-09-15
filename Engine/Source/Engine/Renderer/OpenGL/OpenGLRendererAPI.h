#pragma once

#include "Engine/Renderer/RendererAPI.h"
#include <unordered_map>
#include <string>

namespace Vortex
{
    class OpenGLGraphicsContext;

    /**
     * @brief OpenGL implementation of RendererAPI
     */
    class OpenGLRendererAPI : public RendererAPI
    {
    public:
        OpenGLRendererAPI();
        ~OpenGLRendererAPI() override;

        // ============================================================================
        // RendererAPI Interface Implementation
        // ============================================================================

        GraphicsAPI GetAPI() const override { return GraphicsAPI::OpenGL; }
        const char* GetName() const override { return "OpenGL"; }

        Result<void> Initialize(GraphicsContext* context) override;
        Result<void> Shutdown() override;
        bool IsInitialized() const override { return m_Initialized; }

        // Basic rendering operations
        Result<void> Clear(uint32_t flags, const glm::vec4& color, float depth, int32_t stencil) override;
        Result<void> SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        Result<void> SetScissor(bool enabled, uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

        // Drawing operations
        Result<void> DrawIndexed(uint32_t indexCount, uint32_t instanceCount, 
                                uint32_t firstIndex, int32_t baseVertex, uint32_t baseInstance,
                                uint32_t primitiveTopology) override;
        Result<void> DrawArrays(uint32_t vertexCount, uint32_t instanceCount,
                               uint32_t firstVertex, uint32_t baseInstance,
                               uint32_t primitiveTopology) override;

        // Resource binding
        Result<void> BindVertexBuffer(uint32_t binding, uint32_t buffer, uint64_t offset, uint64_t stride) override;
        Result<void> BindIndexBuffer(uint32_t buffer, uint32_t indexType, uint64_t offset) override;
        Result<void> BindShader(uint32_t program) override;
        Result<void> BindTexture(uint32_t slot, uint32_t texture, uint32_t sampler) override;
        Result<void> BindVertexArray(uint32_t vao) override;

        // Generic buffer and vertex attrib
        Result<void> BindBuffer(uint32_t target, uint32_t buffer) override;
        Result<void> BufferData(uint32_t target, const void* data, uint64_t size, uint32_t usage) override;
        Result<void> VertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                                         bool normalized, uint64_t stride, uint64_t pointer) override;
        Result<void> EnableVertexAttribArray(uint32_t index, bool enabled) override;

        // Object lifetime
        Result<void> GenBuffers(uint32_t count, uint32_t* outBuffers) override;
        Result<void> DeleteBuffers(uint32_t count, const uint32_t* buffers) override;
        Result<void> GenVertexArrays(uint32_t count, uint32_t* outArrays) override;
        Result<void> DeleteVertexArrays(uint32_t count, const uint32_t* arrays) override;

        // Render state
        Result<void> SetDepthState(bool testEnabled, bool writeEnabled, uint32_t compareFunc) override;
        Result<void> SetBlendState(bool enabled, uint32_t srcFactor, uint32_t dstFactor, uint32_t blendOp) override;
        Result<void> SetCullState(uint32_t cullMode, uint32_t frontFace) override;

        // Debug and profiling
        Result<void> PushDebugGroup(const std::string& name) override;
        Result<void> PopDebugGroup() override;
        std::string GetDebugInfo() const override;

    private:
        bool m_Initialized = false;
        
        // Current state tracking
        struct RenderState
        {
            bool DepthTestEnabled = true;
            bool DepthWriteEnabled = true;
            uint32_t DepthCompareFunc = 0; // GL_LESS
            
            bool BlendEnabled = false;
            uint32_t BlendSrcFactor = 0; // GL_ONE
            uint32_t BlendDstFactor = 0; // GL_ZERO
            uint32_t BlendOp = 0; // GL_FUNC_ADD
            
            uint32_t CullMode = 0; // GL_BACK
            uint32_t FrontFace = 0; // GL_CCW
            
            uint32_t CurrentShaderProgram = 0;
            std::unordered_map<uint32_t, uint32_t> BoundTextures; // slot -> texture ID

            // Index buffer tracking: 0 = UInt16, 1 = UInt32 (matches BindIndexBuffer mapping)
            uint32_t CurrentIndexType = 1;
            uint64_t IndexBufferOffset = 0;
        } m_CurrentState;

        /**
         * @brief Validate that OpenGL context is current and ready
         * @return Success/failure result
         */
        Result<void> ValidateContext() const;

        /**
         * @brief Convert engine blend factor to OpenGL blend factor
         * @param factor Engine blend factor
         * @return OpenGL blend factor
         */
        uint32_t ConvertBlendFactor(uint32_t factor) const;

        /**
         * @brief Convert engine blend operation to OpenGL blend operation
         * @param op Engine blend operation
         * @return OpenGL blend operation
         */
        uint32_t ConvertBlendOp(uint32_t op) const;

        /**
         * @brief Convert engine depth function to OpenGL depth function
         * @param func Engine depth function
         * @return OpenGL depth function
         */
        uint32_t ConvertDepthFunc(uint32_t func) const;

        /**
         * @brief Convert engine cull mode to OpenGL cull mode
         * @param mode Engine cull mode
         * @return OpenGL cull mode
         */
        uint32_t ConvertCullMode(uint32_t mode) const;

        /**
         * @brief Convert engine front face to OpenGL front face
         * @param face Engine front face
         * @return OpenGL front face
         */
        uint32_t ConvertFrontFace(uint32_t face) const;

        /**
         * @brief Get OpenGL error string
         * @param error OpenGL error code
         * @return Human-readable error string
         */
        std::string GetGLErrorString(uint32_t error) const;

        /**
         * @brief Check for OpenGL errors and log them
         * @param operation Description of the operation that might have failed
         * @return True if no errors, false if errors were found
         */
        bool CheckGLError(const std::string& operation) const;

        // Converters from engine enums to GL
        uint32_t ConvertBufferTarget(uint32_t target) const;
        uint32_t ConvertBufferUsage(uint32_t usage) const;
        uint32_t ConvertDataType(uint32_t type) const;
        uint32_t ConvertPrimitiveTopology(uint32_t topology) const;
    };
}
