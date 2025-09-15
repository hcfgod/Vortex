#pragma once

#include "vxpch.h"
#include "GraphicsContext.h"
#include "RenderCommand.h"
#include "RenderTypes.h"
#include "Core/Debug/ErrorCodes.h"

namespace Vortex
{
    // Enums moved to RenderTypes.h
    /**
     * @brief Abstract renderer API interface
     * 
     * This class defines the interface that all rendering API implementations must provide.
     * It serves as the bridge between the engine's high-level rendering systems and the
     * low-level graphics API calls (OpenGL, DirectX, Vulkan, etc.)
     */
    class RendererAPI
    {
    public:
        virtual ~RendererAPI() = default;

        /**
         * @brief Get the graphics API type for this renderer
         * @return The graphics API enum
         */
        virtual GraphicsAPI GetAPI() const = 0;

        /**
         * @brief Get a human-readable name for this renderer
         * @return Renderer name string
         */
        virtual const char* GetName() const = 0;

        // ============================================================================
        // INITIALIZATION AND CLEANUP
        // ============================================================================

        /**
         * @brief Initialize the renderer API
         * @param context Graphics context to use
         * @return Success/failure result
         */
        virtual Result<void> Initialize(GraphicsContext* context) = 0;

        /**
         * @brief Shutdown the renderer API and cleanup resources
         * @return Success/failure result
         */
        virtual Result<void> Shutdown() = 0;

        /**
         * @brief Check if the renderer is currently initialized
         * @return True if initialized, false otherwise
         */
        virtual bool IsInitialized() const = 0;

        // ============================================================================
        // BASIC RENDERING OPERATIONS
        // ============================================================================

        /**
         * @brief Clear the current framebuffer
         * @param flags Clear flags (color, depth, stencil)
         * @param color Clear color
         * @param depth Clear depth value
         * @param stencil Clear stencil value
         * @return Success/failure result
         */
        virtual Result<void> Clear(uint32_t flags, const glm::vec4& color, float depth, int32_t stencil) = 0;

        /**
         * @brief Set the viewport
         * @param x Viewport X coordinate
         * @param y Viewport Y coordinate
         * @param width Viewport width
         * @param height Viewport height
         * @return Success/failure result
         */
        virtual Result<void> SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

        /**
         * @brief Set the scissor test rectangle
         * @param enabled Whether scissor test is enabled
         * @param x Scissor rectangle X coordinate
         * @param y Scissor rectangle Y coordinate
         * @param width Scissor rectangle width
         * @param height Scissor rectangle height
         * @return Success/failure result
         */
        virtual Result<void> SetScissor(bool enabled, uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

        // ============================================================================
        // DRAWING OPERATIONS
        // ============================================================================

        /**
         * @brief Draw indexed geometry
         * @param indexCount Number of indices to draw
         * @param instanceCount Number of instances to draw
         * @param firstIndex Offset into the index buffer
         * @param baseVertex Vertex offset added to each index
         * @param baseInstance Instance offset
         * @param primitiveTopology Primitive topology to use (triangles, lines, etc.)
         * @return Success/failure result
         */
        virtual Result<void> DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                         uint32_t firstIndex, int32_t baseVertex, uint32_t baseInstance,
                                         uint32_t primitiveTopology) = 0;

        /**
         * @brief Draw non-indexed geometry
         * @param vertexCount Number of vertices to draw
         * @param instanceCount Number of instances to draw
         * @param firstVertex First vertex to draw
         * @param baseInstance Instance offset
         * @param primitiveTopology Primitive topology to use (triangles, lines, etc.)
         * @return Success/failure result
         */
        virtual Result<void> DrawArrays(uint32_t vertexCount, uint32_t instanceCount,
                                        uint32_t firstVertex, uint32_t baseInstance,
                                        uint32_t primitiveTopology) = 0;

        // ============================================================================
        // RESOURCE BINDING
        // ============================================================================

        /**
         * @brief Bind a vertex buffer
         * @param binding Binding point
         * @param buffer Buffer ID
         * @param offset Offset into the buffer
         * @param stride Vertex stride
         * @return Success/failure result
         */
        virtual Result<void> BindVertexBuffer(uint32_t binding, uint32_t buffer, uint64_t offset, uint64_t stride) = 0;

        /**
         * @brief Bind an index buffer
         * @param buffer Buffer ID
         * @param indexType Type of indices (16-bit or 32-bit)
         * @param offset Offset into the buffer
         * @return Success/failure result
         */
        virtual Result<void> BindIndexBuffer(uint32_t buffer, uint32_t indexType, uint64_t offset) = 0;

        /**
         * @brief Bind a buffer to a target (generic)
         */
        virtual Result<void> BindBuffer(uint32_t target, uint32_t buffer) = 0;

        /**
         * @brief Upload data to a buffer target
         */
        virtual Result<void> BufferData(uint32_t target, const void* data, uint64_t size, uint32_t usage) = 0;

        /**
         * @brief Define an array of generic vertex attribute data
         */
        virtual Result<void> VertexAttribPointer(uint32_t index, int32_t size, uint32_t type,
                                                 bool normalized, uint64_t stride, uint64_t pointer) = 0;

        /**
         * @brief Enable or disable a generic vertex attribute array
         */
        virtual Result<void> EnableVertexAttribArray(uint32_t index, bool enabled) = 0;

        /**
         * @brief Bind a vertex array object
         * @param vao Vertex array object ID
         * @return Success/failure result
         */
        virtual Result<void> BindVertexArray(uint32_t vao) = 0;

        /**
         * @brief Bind a shader program
         * @param program Program ID
         * @return Success/failure result
         */
        virtual Result<void> BindShader(uint32_t program) = 0;

        /**
         * @brief Bind a texture
         * @param slot Texture slot
         * @param texture Texture ID
         * @param sampler Sampler ID (optional)
         * @return Success/failure result
         */
        virtual Result<void> BindTexture(uint32_t slot, uint32_t texture, uint32_t sampler) = 0;

        // ============================================================================
        // RENDER STATE
        // ============================================================================

        /**
         * @brief Set depth testing state
         * @param testEnabled Enable depth testing
         * @param writeEnabled Enable depth writing
         * @param compareFunc Depth comparison function
         * @return Success/failure result
         */
        virtual Result<void> SetDepthState(bool testEnabled, bool writeEnabled, uint32_t compareFunc) = 0;

        /**
         * @brief Set blending state
         * @param enabled Enable blending
         * @param srcFactor Source blend factor
         * @param dstFactor Destination blend factor
         * @param blendOp Blend operation
         * @return Success/failure result
         */
        virtual Result<void> SetBlendState(bool enabled, uint32_t srcFactor, uint32_t dstFactor, uint32_t blendOp) = 0;

        /**
         * @brief Set culling state
         * @param cullMode Face culling mode
         * @param frontFace Front face winding order
         * @return Success/failure result
         */
        virtual Result<void> SetCullState(uint32_t cullMode, uint32_t frontFace) = 0;

        // ============================================================================
        // DEBUG AND PROFILING
        // ============================================================================

        /**
         * @brief Push a debug group for profiling
         * @param name Debug group name
         * @return Success/failure result
         */
        virtual Result<void> PushDebugGroup(const std::string& name) = 0;

        /**
         * @brief Pop a debug group
         * @return Success/failure result
         */
        virtual Result<void> PopDebugGroup() = 0;

        /**
         * @brief Get debug information about the renderer state
         * @return Debug info string
         */
        virtual std::string GetDebugInfo() const = 0;

        // ============================================================================
        // FACTORY METHODS
        // ============================================================================

        /**
         * @brief Create a renderer API for the specified graphics API
         * @param api The graphics API to create a renderer for
         * @return Unique pointer to the renderer API, nullptr on failure
         */
        static std::unique_ptr<RendererAPI> Create(GraphicsAPI api);

        /**
         * @brief Create a renderer API for the current/default graphics API
         * @return Unique pointer to the renderer API, nullptr on failure
         */
        static std::unique_ptr<RendererAPI> Create();

    protected:
        // Provide derived classes a safe accessor to the active graphics context
        GraphicsContext* GetGraphicsContext() const { return m_GraphicsContext; }
        GraphicsContext* m_GraphicsContext = nullptr;
    };

    /**
     * @brief Renderer API manager for global state
     * 
     * This singleton manages the current renderer API and provides
     * global access to rendering functionality.
     */
    class RendererAPIManager
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the manager instance
         */
        static RendererAPIManager& GetInstance();

        /**
         * @brief Initialize the renderer API manager
         * @param api Graphics API to use
         * @param context Graphics context
         * @return Success/failure result
         */
        Result<void> Initialize(GraphicsAPI api, GraphicsContext* context);

        /**
         * @brief Shutdown the renderer API manager
         * @return Success/failure result
         */
        Result<void> Shutdown();

        /**
         * @brief Get the current renderer API
         * @return Pointer to the current renderer API, nullptr if not initialized
         */
        RendererAPI* GetRenderer() const { return m_RendererAPI.get(); }

        /**
         * @brief Get the current graphics API
         * @return Current graphics API enum
         */
        GraphicsAPI GetCurrentAPI() const { return m_CurrentAPI; }

        /**
         * @brief Check if the manager is initialized
         * @return True if initialized, false otherwise
         */
        bool IsInitialized() const { return m_RendererAPI != nullptr && m_RendererAPI->IsInitialized(); }

    private:
        RendererAPIManager() = default;
        ~RendererAPIManager() = default;

        RendererAPIManager(const RendererAPIManager&) = delete;
        RendererAPIManager& operator=(const RendererAPIManager&) = delete;

        std::unique_ptr<RendererAPI> m_RendererAPI;
        GraphicsAPI m_CurrentAPI = GraphicsAPI::None;
    };

    // ============================================================================
    // GLOBAL ACCESS FUNCTIONS
    // ============================================================================

    /**
     * @brief Get the current renderer API instance
     * @return Pointer to current renderer API, nullptr if not initialized
     */
    RendererAPI* GetRenderer();

    /**
     * @brief Initialize the global renderer API
     * @param api Graphics API to use
     * @param context Graphics context
     * @return Success/failure result
     */
    Result<void> InitializeRenderer(GraphicsAPI api, GraphicsContext* context);

    /**
     * @brief Shutdown the global renderer API
     * @return Success/failure result
     */
    Result<void> ShutdownRenderer();
}