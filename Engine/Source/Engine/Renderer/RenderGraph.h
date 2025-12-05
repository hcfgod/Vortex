#pragma once

#include "RenderPass.h"
#include "FrameBuffer.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <chrono>

namespace Vortex
{
    /**
     * @brief Orchestrates render passes into a complete rendering pipeline
     * 
     * The RenderGraph manages the execution order of render passes and handles
     * resource allocation for intermediate render targets. It provides a flexible
     * system similar to Unity's Scriptable Render Pipeline.
     * 
     * Built-in passes are created via CreateDefault():
     * - Background: Skyboxes, far backgrounds
     * - World3D: 3D geometry (future)
     * - World2D: 2D sprites, game content
     * - Effects: Particles, post-processing
     * - UI: User interface (no lighting)
     * - Overlay: Debug overlays
     * 
     * Example usage:
     * @code
     * auto graph = RenderGraph::CreateDefault();
     * 
     * // Each frame:
     * graph->Begin();
     * 
     * graph->BeginPass("World2D");
     * Renderer2D::BeginScene(camera);
     * Renderer2D::DrawQuad(...);  // Game content - can have lighting
     * Renderer2D::EndScene();
     * graph->EndPass();
     * 
     * graph->BeginPass("UI");
     * Renderer2D::BeginScene(uiCamera);
     * Renderer2D::DrawQuad(...);  // UI - no lighting
     * Renderer2D::EndScene();
     * graph->EndPass();
     * 
     * graph->Execute();  // Composite and present
     * @endcode
     */
    class RenderGraph
    {
    public:
        RenderGraph();
        ~RenderGraph();

        // Non-copyable
        RenderGraph(const RenderGraph&) = delete;
        RenderGraph& operator=(const RenderGraph&) = delete;

        // ============================================================================
        // FACTORY
        // ============================================================================

        /**
         * @brief Create a render graph with default passes
         * 
         * Creates a graph with the following passes in order:
         * - Background (clear, depth write)
         * - World3D (depth test, no clear)
         * - World2D (depth test, blending for transparency)
         * - Effects (blending, depth read only)
         * - UI (no depth, blending)
         * - Overlay (no depth, blending)
         * 
         * @return Shared pointer to the created render graph
         */
        static std::shared_ptr<RenderGraph> CreateDefault();

        /**
         * @brief Create an empty render graph (no passes)
         * @return Shared pointer to the created render graph
         */
        static std::shared_ptr<RenderGraph> Create();

        // ============================================================================
        // PASS MANAGEMENT
        // ============================================================================

        /**
         * @brief Add a render pass to the end of the graph
         * @param pass The pass to add
         */
        void AddPass(RenderPassRef pass);

        /**
         * @brief Insert a pass after an existing pass
         * @param afterPassName Name of the existing pass
         * @param pass The pass to insert
         * @return true if insertion succeeded, false if afterPassName not found
         */
        bool InsertPassAfter(const std::string& afterPassName, RenderPassRef pass);

        /**
         * @brief Insert a pass before an existing pass
         * @param beforePassName Name of the existing pass
         * @param pass The pass to insert
         * @return true if insertion succeeded, false if beforePassName not found
         */
        bool InsertPassBefore(const std::string& beforePassName, RenderPassRef pass);

        /**
         * @brief Remove a pass by name
         * @param passName Name of the pass to remove
         * @return true if the pass was found and removed
         */
        bool RemovePass(const std::string& passName);

        /**
         * @brief Get a pass by name
         * @param passName Name of the pass
         * @return Pointer to the pass, or nullptr if not found
         */
        RenderPass* GetPass(const std::string& passName);
        const RenderPass* GetPass(const std::string& passName) const;

        /**
         * @brief Get all passes in execution order
         */
        const std::vector<RenderPassRef>& GetPasses() const { return m_Passes; }

        /**
         * @brief Get the number of passes in the graph
         */
        size_t GetPassCount() const { return m_Passes.size(); }

        /**
         * @brief Clear all passes from the graph
         */
        void ClearPasses();

        // ============================================================================
        // EXECUTION
        // ============================================================================

        /**
         * @brief Begin a new frame for the render graph
         * 
         * Call this at the start of each frame before any BeginPass() calls.
         * Resets internal state and prepares for a new frame.
         */
        void Begin();

        /**
         * @brief Begin a render pass by name
         * @param passName Name of the pass to begin
         * @return true if the pass was found and begun successfully
         */
        bool BeginPass(const std::string& passName);

        /**
         * @brief Begin a render pass directly
         * @param pass The pass to begin
         */
        void BeginPass(RenderPassRef pass);

        /**
         * @brief End the current render pass
         */
        void EndPass();

        /**
         * @brief Execute any pending compositing and finalize the frame
         * 
         * Call this at the end of the frame after all passes have been rendered.
         */
        void Execute();

        /**
         * @brief Check if a pass is currently active
         */
        bool IsPassActive() const { return m_CurrentPass != nullptr; }

        /**
         * @brief Get the currently active pass (if any)
         */
        RenderPass* GetCurrentPass() const { return m_CurrentPass.get(); }

        /**
         * @brief Get the current render domain (from active pass)
         */
        RenderDomain GetCurrentDomain() const;

        // ============================================================================
        // RESOURCE MANAGEMENT
        // ============================================================================

        /**
         * @brief Set the output framebuffer for the entire graph
         * @param target The final output target (nullptr for default backbuffer)
         */
        void SetOutputTarget(const FrameBufferRef& target) { m_OutputTarget = target; }

        /**
         * @brief Get the output framebuffer
         */
        FrameBufferRef GetOutputTarget() const { return m_OutputTarget; }

        /**
         * @brief Resize all internal framebuffers
         * @param width New width
         * @param height New height
         */
        void Resize(uint32_t width, uint32_t height);

        // ============================================================================
        // CALLBACKS
        // ============================================================================

        using FrameCallback = std::function<void(RenderGraph*)>;

        /**
         * @brief Set a callback to be invoked at the start of each frame
         */
        void SetOnFrameBegin(FrameCallback callback) { m_OnFrameBegin = std::move(callback); }

        /**
         * @brief Set a callback to be invoked at the end of each frame
         */
        void SetOnFrameEnd(FrameCallback callback) { m_OnFrameEnd = std::move(callback); }

        // ============================================================================
        // PER-PASS FRAMEBUFFER MANAGEMENT
        // ============================================================================

        /**
         * @brief Create an intermediate framebuffer for a pass
         * @param passName Name of the pass
         * @param width Framebuffer width (0 = use graph width)
         * @param height Framebuffer height (0 = use graph height)
         * @return The created framebuffer, or nullptr on failure
         * 
         * This allows a pass to render to its own intermediate buffer
         * for post-processing effects.
         */
        FrameBufferRef CreatePassFramebuffer(const std::string& passName, 
                                              uint32_t width = 0, uint32_t height = 0);

        /**
         * @brief Get the intermediate framebuffer for a pass
         * @param passName Name of the pass
         * @return The framebuffer, or nullptr if none
         */
        FrameBufferRef GetPassFramebuffer(const std::string& passName) const;

        /**
         * @brief Remove the intermediate framebuffer for a pass
         * @param passName Name of the pass
         */
        void RemovePassFramebuffer(const std::string& passName);

        // ============================================================================
        // STATISTICS AND DEBUG
        // ============================================================================

        /**
         * @brief Per-frame statistics for the render graph
         */
        struct FrameStatistics
        {
            uint32_t FrameNumber = 0;
            uint32_t PassesExecuted = 0;
            std::vector<std::string> ExecutedPassNames;
            std::unordered_map<std::string, float> PassTimingsMs;
            float TotalFrameTimeMs = 0.0f;

            void Reset()
            {
                PassesExecuted = 0;
                ExecutedPassNames.clear();
                PassTimingsMs.clear();
                TotalFrameTimeMs = 0.0f;
            }
        };

        /**
         * @brief Get the statistics from the last completed frame
         */
        const FrameStatistics& GetLastFrameStats() const { return m_LastFrameStats; }

        /**
         * @brief Get debug information about the render graph
         */
        std::string GetDebugInfo() const;

        /**
         * @brief Enable or disable debug logging
         */
        void SetDebugEnabled(bool enabled) { m_DebugEnabled = enabled; }

        /**
         * @brief Check if debug mode is enabled
         */
        bool IsDebugEnabled() const { return m_DebugEnabled; }

    private:
        /**
         * @brief Find a pass by name and return iterator
         */
        std::vector<RenderPassRef>::iterator FindPass(const std::string& name);
        std::vector<RenderPassRef>::const_iterator FindPass(const std::string& name) const;

        /**
         * @brief Build the pass lookup map
         */
        void RebuildPassMap();

    private:
        // Ordered list of render passes
        std::vector<RenderPassRef> m_Passes;

        // Fast name lookup
        std::unordered_map<std::string, size_t> m_PassIndexMap;

        // Current execution state
        RenderPassRef m_CurrentPass;
        bool m_FrameActive = false;

        // Output target (nullptr = default backbuffer)
        FrameBufferRef m_OutputTarget;

        // Per-pass intermediate framebuffers
        std::unordered_map<std::string, FrameBufferRef> m_PassFramebuffers;

        // Viewport size for internal framebuffers
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        // Callbacks
        FrameCallback m_OnFrameBegin;
        FrameCallback m_OnFrameEnd;

        // Statistics
        FrameStatistics m_CurrentFrameStats;
        FrameStatistics m_LastFrameStats;
        uint32_t m_FrameCounter = 0;
        std::chrono::high_resolution_clock::time_point m_FrameStartTime;
        std::chrono::high_resolution_clock::time_point m_PassStartTime;

        // Debug
        bool m_DebugEnabled = false;
    };

    // Type alias
    using RenderGraphRef = std::shared_ptr<RenderGraph>;

    // ============================================================================
    // GLOBAL ACCESS
    // ============================================================================

    /**
     * @brief Get the global render graph instance
     * @return Reference to the global render graph
     */
    RenderGraph& GetRenderGraph();

    /**
     * @brief Set the global render graph instance
     * @param graph The render graph to use globally
     */
    void SetRenderGraph(RenderGraphRef graph);

    /**
     * @brief Check if a global render graph has been set
     */
    bool HasRenderGraph();
}