#pragma once

#include "RenderCommand.h"
#include "RendererAPI.h"
#include "RenderTypes.h"
#include "Core/Debug/ErrorCodes.h"
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <unordered_map>
#include <cstring>

namespace Vortex
{
    // Forward declarations
    class GraphicsContext;

    /**
     * @brief Thread-safe queue for render commands with automatic dispatching
     *
     * The RenderCommandQueue allows multiple threads to submit render commands
     * and ensures they are executed on the main rendering thread in the correct order.
     * It provides both immediate execution and queued execution modes.
     */
    class RenderCommandQueue
    {
    public:
        /**
         * @brief Statistics about the render command queue
         */
        struct Statistics
        {
            size_t QueuedCommands = 0;
            size_t ProcessedCommands = 0;
            size_t TotalCommandsThisFrame = 0;
            size_t DroppedCommands = 0;  // Commands dropped due to full queue
            float AverageCost = 0.0f;    // Average estimated cost of processed commands
            float TotalCost = 0.0f;      // Total execution time (microseconds) accumulated
            float EstimatedTotalCost = 0.0f; // Sum of GetEstimatedCost() across processed
            std::unordered_map<std::string, float> EstimatedCostByCommand; // Sum per command name
            std::unordered_map<std::string, size_t> CountByCommand;        // Count per command name
            std::chrono::microseconds LastProcessTime{ 0 };
        };

        /**
         * @brief Configuration for the render command queue
         */
        struct Config
        {
            size_t MaxQueuedCommands;  // Maximum commands in queue
            size_t MaxCommandsPerFrame; // Maximum commands to process per frame
            bool EnableProfiling;       // Enable detailed profiling
            bool WarnOnDrop;            // Log warnings when commands are dropped

            Config() : MaxQueuedCommands(10000), MaxCommandsPerFrame(1000), EnableProfiling(true), WarnOnDrop(true) {}
        };

        /**
         * @brief Constructor
         * @param config Queue configuration
         */
        explicit RenderCommandQueue(const Config& config = Config{});

        /**
         * @brief Destructor - ensures all commands are processed or properly cleaned up
         */
        ~RenderCommandQueue();

        // Non-copyable, non-movable (due to thread synchronization)
        RenderCommandQueue(const RenderCommandQueue&) = delete;
        RenderCommandQueue& operator=(const RenderCommandQueue&) = delete;
        RenderCommandQueue(RenderCommandQueue&&) = delete;
        RenderCommandQueue& operator=(RenderCommandQueue&&) = delete;

        /**
         * @brief Initialize the command queue with graphics context
         * @param context The graphics context to execute commands on
         * @return Success/failure result
         */
        Result<void> Initialize(GraphicsContext* context);

        /**
         * @brief Shutdown the command queue and clean up resources
         * @return Success/failure result
         */
        Result<void> Shutdown();

        /**
         * @brief Submit a render command to the queue
         * @param command Unique pointer to the command to submit
         * @param executeImmediate If true, execute immediately on calling thread (thread-safe)
         * @return True if command was successfully queued/executed, false if queue is full
         */
        bool Submit(std::unique_ptr<RenderCommand> command, bool executeImmediate = false);

        /**
         * @brief Submit multiple render commands at once
         * @param commands Vector of commands to submit
         * @param executeImmediate If true, execute immediately on calling thread
         * @return Number of commands successfully queued/executed
         */
        size_t SubmitBatch(std::vector<std::unique_ptr<RenderCommand>> commands, bool executeImmediate = false);

        /**
         * @brief Process all queued commands (call from main render thread)
         * @param maxCommands Maximum number of commands to process this frame (0 = unlimited)
         * @return Success/failure result
         */
        Result<void> ProcessCommands(size_t maxCommands = 0);

        /**
         * @brief Flush all queued commands immediately (blocking)
         * @return Success/failure result
         */
        Result<void> FlushAll();

        /**
         * @brief Clear all queued commands without executing them
         * @return Number of commands cleared
         */
        size_t ClearQueue();

        /**
         * @brief Get current queue statistics
         * @return Statistics structure
         */
        const Statistics& GetStatistics() const { return m_Stats; }

        /**
         * @brief Reset statistics counters
         */
        void ResetStatistics();

        /**
         * @brief Check if the queue is currently initialized
         * @return True if initialized, false otherwise
         */
        bool IsInitialized() const { return m_Initialized.load(); }

        /**
         * @brief Get the current queue size
         * @return Number of commands currently in queue
         */
        size_t GetQueueSize() const;

        /**
         * @brief Check if the queue is full
         * @return True if queue is at maximum capacity
         */
        bool IsQueueFull() const;

        /**
         * @brief Check if we're currently on the render thread
         * @return True if called from the render thread
         */
        bool IsOnRenderThread() const;

        // ============================================================================
        // CONVENIENCE METHODS FOR COMMON COMMANDS
        // ============================================================================

        /**
         * @brief Submit a clear command
         * @param flags Clear flags (color, depth, stencil)
         * @param color Clear color
         * @param depth Clear depth value
         * @param stencil Clear stencil value
         * @param executeImmediate Whether to execute immediately
         * @return True if successfully submitted
         */
        bool Clear(uint32_t flags = ClearCommand::All,
            const glm::vec4& color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            float depth = 1.0f,
            int32_t stencil = 0,
            bool executeImmediate = false)
        {
            return Submit(std::make_unique<ClearCommand>(flags, color, depth, stencil), executeImmediate);
        }

        /**
         * @brief Submit a viewport command
         * @param x Viewport X coordinate
         * @param y Viewport Y coordinate
         * @param width Viewport width
         * @param height Viewport height
         * @param executeImmediate Whether to execute immediately
         * @return True if successfully submitted
         */
        bool SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool executeImmediate = false)
        {
            return Submit(std::make_unique<SetViewportCommand>(x, y, width, height), executeImmediate);
        }

        /**
         * @brief Submit a draw indexed command
         * @param indexCount Number of indices to draw
         * @param instanceCount Number of instances
         * @param firstIndex First index offset
         * @param baseVertex Base vertex offset
         * @param baseInstance Base instance offset
         * @param executeImmediate Whether to execute immediately
         * @return True if successfully submitted
         */
        bool DrawIndexed(uint32_t indexCount,
            uint32_t instanceCount = 1,
            uint32_t firstIndex = 0,
            int32_t baseVertex = 0,
            uint32_t baseInstance = 0,
            uint32_t primitiveTopology = static_cast<uint32_t>(PrimitiveTopology::Triangles),
            bool executeImmediate = false)
        {
            return Submit(std::make_unique<DrawIndexedCommand>(indexCount, instanceCount, firstIndex, baseVertex, baseInstance, primitiveTopology), executeImmediate);
        }

        bool DrawArrays(uint32_t vertexCount,
            uint32_t instanceCount = 1,
            uint32_t firstVertex = 0,
            uint32_t baseInstance = 0,
            uint32_t primitiveTopology = static_cast<uint32_t>(PrimitiveTopology::Triangles),
            bool executeImmediate = false)
        {
            return Submit(std::make_unique<DrawArraysCommand>(vertexCount, instanceCount, firstVertex, baseInstance, primitiveTopology), executeImmediate);
        }

        /**
         * @brief Submit a bind shader command
         * @param program Shader program ID
         * @param executeImmediate Whether to execute immediately
         * @return True if successfully submitted
         */
        bool BindShader(uint32_t program, bool executeImmediate = false)
        {
            return Submit(std::make_unique<BindShaderCommand>(program), executeImmediate);
        }

        /**
         * @brief Bind a vertex array object (VAO)
         */
        bool BindVertexArray(uint32_t vao, bool executeImmediate = false)
        {
            return Submit(std::make_unique<BindVertexArrayCommand>(vao), executeImmediate);
        }

        // Generic buffer and vertex attribute helpers
        bool BindBuffer(uint32_t target, uint32_t buffer, bool executeImmediate = false)
        {
            return Submit(std::make_unique<BindBufferCommand>(target, buffer), executeImmediate);
        }

        bool BufferData(uint32_t target, const void* data, uint64_t size, uint32_t usage, bool executeImmediate = false)
        {
            auto payload = std::make_shared<BufferDataCommand::ByteVector>();
            if (data && size > 0)
            {
                payload->resize(static_cast<size_t>(size));
                std::memcpy(payload->data(), data, static_cast<size_t>(size));
            }
            return Submit(std::make_unique<BufferDataCommand>(target, std::move(payload), usage), executeImmediate);
        }

        bool BufferData(uint32_t target, std::shared_ptr<BufferDataCommand::ByteVector> payload, uint32_t usage, bool executeImmediate = false)
        {
            return Submit(std::make_unique<BufferDataCommand>(target, std::move(payload), usage), executeImmediate);
        }

        bool BindTexture(uint32_t slot, uint32_t texture, uint32_t sampler = 0, bool executeImmediate = false)
        {
            return Submit(std::make_unique<BindTextureCommand>(slot, texture, sampler), executeImmediate);
        }

        bool VertexAttribPointer(uint32_t index, int32_t size, uint32_t type, bool normalized,
                                 uint64_t stride, uint64_t pointer, bool executeImmediate = false)
        {
            return Submit(std::make_unique<VertexAttribPointerCommand>(index, size, type, normalized, stride, pointer), executeImmediate);
        }

        bool EnableVertexAttribArray(uint32_t index, bool enabled, bool executeImmediate = false)
        {
            return Submit(std::make_unique<EnableVertexAttribArrayCommand>(index, enabled), executeImmediate);
        }

        // Object lifetime helpers
        bool GenBuffers(uint32_t count, uint32_t* outBuffers, bool executeImmediate = true)
        {
            return Submit(std::make_unique<GenBuffersCommand>(count, outBuffers), executeImmediate);
        }

        bool DeleteBuffers(uint32_t count, const uint32_t* buffers, bool executeImmediate = true)
        {
            return Submit(std::make_unique<DeleteBuffersCommand>(count, buffers), executeImmediate);
        }

        bool GenVertexArrays(uint32_t count, uint32_t* outArrays, bool executeImmediate = true)
        {
            return Submit(std::make_unique<GenVertexArraysCommand>(count, outArrays), executeImmediate);
        }

        bool DeleteVertexArrays(uint32_t count, const uint32_t* arrays, bool executeImmediate = true)
        {
            return Submit(std::make_unique<DeleteVertexArraysCommand>(count, arrays), executeImmediate);
        }

        // Texture helpers
        bool GenTextures(uint32_t count, uint32_t* outTextures, bool executeImmediate = true)
        {
            return Submit(std::make_unique<GenTexturesCommand>(count, outTextures), executeImmediate);
        }

        bool DeleteTextures(uint32_t count, const uint32_t* textures, bool executeImmediate = true)
        {
            return Submit(std::make_unique<DeleteTexturesCommand>(count, textures), executeImmediate);
        }

        bool BindTextureTarget(uint32_t target, uint32_t texture, bool executeImmediate = false)
        {
            return Submit(std::make_unique<BindTextureTargetCommand>(target, texture), executeImmediate);
        }

        bool TexImage2D(uint32_t target, int32_t level, uint32_t internalFormat,
                        uint32_t width, uint32_t height, uint32_t format, uint32_t type,
                        const void* data, uint64_t sizeBytes, bool executeImmediate = false)
        {
            auto payload = std::make_shared<TexImage2DCommand::ByteVector>();
            if (data && sizeBytes > 0)
            {
                payload->resize(static_cast<size_t>(sizeBytes));
                std::memcpy(payload->data(), data, static_cast<size_t>(sizeBytes));
            }
            return Submit(std::make_unique<TexImage2DCommand>(target, level, internalFormat, width, height, format, type, std::move(payload)), executeImmediate);
        }

        bool TexParameteri(uint32_t target, uint32_t pname, int32_t param, bool executeImmediate = false)
        {
            return Submit(std::make_unique<TexParameteriCommand>(target, pname, param), executeImmediate);
        }

        bool GenerateMipmap(uint32_t target, bool executeImmediate = false)
        {
            return Submit(std::make_unique<GenerateMipmapCommand>(target), executeImmediate);
        }

        /**
         * @brief Push a debug group for profiling
         * @param name Debug group name
         * @param executeImmediate Whether to execute immediately
         * @return True if successfully submitted
         */
        bool PushDebugGroup(const std::string& name, bool executeImmediate = false)
        {
            return Submit(std::make_unique<PushDebugGroupCommand>(name), executeImmediate);
        }

        /**
         * @brief Pop a debug group
         * @param executeImmediate Whether to execute immediately
         * @return True if successfully submitted
         */
        bool PopDebugGroup(bool executeImmediate = false)
        {
            return Submit(std::make_unique<PopDebugGroupCommand>(), executeImmediate);
        }

        // Lightweight helpers for pipeline state
        bool SetDepthState(bool testEnabled, bool writeEnabled, SetDepthStateCommand::CompareFunction func = SetDepthStateCommand::Less, bool executeImmediate = false)
        {
            return Submit(std::make_unique<SetDepthStateCommand>(testEnabled, writeEnabled, func), executeImmediate);
        }

        bool SetBlendState(bool enabled,
            SetBlendStateCommand::BlendFactor srcFactor = SetBlendStateCommand::SrcAlpha,
            SetBlendStateCommand::BlendFactor dstFactor = SetBlendStateCommand::OneMinusSrcAlpha,
            SetBlendStateCommand::BlendOperation op = SetBlendStateCommand::Add,
            bool executeImmediate = false)
        {
            return Submit(std::make_unique<SetBlendStateCommand>(enabled, srcFactor, dstFactor, op), executeImmediate);
        }

        bool SetCullState(SetCullStateCommand::CullMode mode, SetCullStateCommand::FrontFace face = SetCullStateCommand::CounterClockwise, bool executeImmediate = false)
        {
            return Submit(std::make_unique<SetCullStateCommand>(mode, face), executeImmediate);
        }

    private:
        /**
         * @brief Execute a single command safely
         * @param command Command to execute
         * @return Success/failure result
         */
        Result<void> ExecuteCommand(std::unique_ptr<RenderCommand> command);

        /**
         * @brief Update statistics with command execution
         * @param commandName The name of the command that was executed
         * @param estimatedCost The estimated cost of the command
         * @param executionTime Time taken to execute
         */
        void UpdateStatistics(const std::string& commandName, float estimatedCost, std::chrono::microseconds executionTime);

        // Configuration
        Config m_Config;

        // State
        std::atomic<bool> m_Initialized{ false };
        GraphicsContext* m_GraphicsContext = nullptr;
        std::thread::id m_RenderThreadID;

        // Command queue
        mutable std::mutex m_QueueMutex;
        std::queue<std::unique_ptr<RenderCommand>> m_CommandQueue;

        // Statistics
        mutable std::mutex m_StatsMutex;
        Statistics m_Stats;

        // For immediate execution thread safety
        mutable std::mutex m_ExecutionMutex;
    };

    // ============================================================================
    // GLOBAL RENDER COMMAND QUEUE ACCESS
    // ============================================================================

    /**
     * @brief Get the global render command queue instance
     * @return Reference to the global queue
     */
    RenderCommandQueue& GetRenderCommandQueue();

    /**
     * @brief Initialize the global render command queue
     * @param context Graphics context to use
     * @param config Queue configuration
     * @return Success/failure result
     */
    Result<void> InitializeGlobalRenderQueue(GraphicsContext* context, const RenderCommandQueue::Config& config = {});

    /**
     * @brief Shutdown the global render command queue
     * @return Success/failure result
     */
    Result<void> ShutdownGlobalRenderQueue();

    // ============================================================================
    // CONVENIENCE MACROS FOR RENDER COMMANDS
    // ============================================================================

#define VX_RENDER_COMMAND(command) \
        ::Vortex::GetRenderCommandQueue().Submit(std::make_unique<command>)

#define VX_RENDER_COMMAND_IMMEDIATE(command) \
        ::Vortex::GetRenderCommandQueue().Submit(std::make_unique<command>, true)

#define VX_RENDER_CLEAR(flags, color, depth, stencil) \
        ::Vortex::GetRenderCommandQueue().Clear(flags, color, depth, stencil)

#define VX_RENDER_VIEWPORT(x, y, w, h) \
        ::Vortex::GetRenderCommandQueue().SetViewport(x, y, w, h)

#define VX_RENDER_DRAW_INDEXED(indexCount, instanceCount, topology) \
        ::Vortex::GetRenderCommandQueue().DrawIndexed(indexCount, instanceCount, 0, 0, 0, topology)

#define VX_RENDER_DRAW_ARRAYS(vertexCount, instanceCount, topology) \
        ::Vortex::GetRenderCommandQueue().DrawArrays(vertexCount, instanceCount, 0, 0, topology)

#define VX_RENDER_BIND_SHADER(program) \
        ::Vortex::GetRenderCommandQueue().BindShader(program)

#define VX_RENDER_DEBUG_PUSH(name) \
        ::Vortex::GetRenderCommandQueue().PushDebugGroup(name)

#define VX_RENDER_DEBUG_POP() \
        ::Vortex::GetRenderCommandQueue().PopDebugGroup()

#define VX_RENDER_SET_DEPTH(test, write, func) \
        ::Vortex::GetRenderCommandQueue().SetDepthState(test, write, func)

#define VX_RENDER_SET_BLEND(enabled, src, dst, op) \
        ::Vortex::GetRenderCommandQueue().SetBlendState(enabled, src, dst, op)

#define VX_RENDER_SET_CULL(mode, face) \
        ::Vortex::GetRenderCommandQueue().SetCullState(mode, face)

    // Convenience wrapper for binding textures to slots
#define VX_RENDER_BIND_TEXTURE(slot, texture, sampler) \
        ::Vortex::GetRenderCommandQueue().Submit(std::make_unique<BindTextureCommand>(slot, texture, sampler))
}