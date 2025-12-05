#pragma once

#include "RenderTypes.h"
#include "FrameBuffer.h"
#include "RenderCommand.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <functional>

namespace Vortex
{
    // Forward declarations
    class RenderCommandQueue;

    /**
     * @brief Specification for creating a render pass
     * 
     * Defines all configuration options for a render pass including target,
     * clear behavior, render state, and sorting mode.
     */
    struct RenderPassSpec
    {
        // Identity
        std::string Name = "Unnamed";
        RenderDomain Domain = RenderDomain::World2D;

        // Target framebuffer (nullptr = default backbuffer or previous pass output)
        FrameBufferRef Target = nullptr;

        // Clear behavior
        uint32_t ClearFlags = 0;  // Use ClearCommand::ClearFlags
        glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        float ClearDepth = 1.0f;
        int32_t ClearStencil = 0;

        // Depth state
        bool DepthTestEnabled = true;
        bool DepthWriteEnabled = true;
        DepthCompareFunc DepthCompare = DepthCompareFunc::Less;

        // Blending state
        bool BlendingEnabled = false;
        BlendFactor SrcBlendFactor = BlendFactor::SrcAlpha;
        BlendFactor DstBlendFactor = BlendFactor::OneMinusSrcAlpha;
        BlendOp BlendOperation = BlendOp::Add;

        // Culling
        bool CullingEnabled = false;
        bool CullBackFace = true;  // true = cull back faces, false = cull front faces

        // Sorting
        RenderSortMode SortMode = RenderSortMode::None;

        // Viewport override (0 = use target size)
        uint32_t ViewportX = 0;
        uint32_t ViewportY = 0;
        uint32_t ViewportWidth = 0;
        uint32_t ViewportHeight = 0;
    };

    /**
     * @brief A render pass encapsulates a stage in the rendering pipeline
     * 
     * Each render pass represents a distinct rendering phase with its own
     * render target, clear settings, and render state. Passes can be organized
     * into a RenderGraph for complex rendering pipelines.
     * 
     * Example usage:
     * @code
     * RenderPassSpec spec;
     * spec.Name = "World2D";
     * spec.Domain = RenderDomain::World2D;
     * spec.ClearFlags = ClearCommand::Color | ClearCommand::Depth;
     * spec.DepthTestEnabled = true;
     * spec.BlendingEnabled = false;
     * 
     * auto pass = RenderPass::Create(spec);
     * pass->Begin();
     * // ... render content ...
     * pass->End();
     * @endcode
     */
    class RenderPass
    {
    public:
        /**
         * @brief Create a render pass with the given specification
         * @param spec The render pass specification
         * @return Shared pointer to the created render pass
         */
        static std::shared_ptr<RenderPass> Create(const RenderPassSpec& spec);

        virtual ~RenderPass() = default;

        // ============================================================================
        // LIFECYCLE
        // ============================================================================

        /**
         * @brief Begin the render pass
         * 
         * Binds the target framebuffer, applies render state, and clears if specified.
         * Must be paired with End().
         */
        virtual void Begin();

        /**
         * @brief End the render pass
         * 
         * Flushes any pending draw calls and restores previous render state.
         * Must be paired with Begin().
         */
        virtual void End();

        /**
         * @brief Check if this pass is currently active (between Begin/End)
         */
        bool IsActive() const { return m_Active; }

        // ============================================================================
        // ACCESSORS
        // ============================================================================

        /**
         * @brief Get the specification used to create this pass
         */
        const RenderPassSpec& GetSpec() const { return m_Spec; }

        /**
         * @brief Get the name of this pass
         */
        const std::string& GetName() const { return m_Spec.Name; }

        /**
         * @brief Get the render domain of this pass
         */
        RenderDomain GetDomain() const { return m_Spec.Domain; }

        /**
         * @brief Get the target framebuffer (may be nullptr for default)
         */
        FrameBufferRef GetTarget() const { return m_Spec.Target; }

        /**
         * @brief Get the output texture from this pass (color attachment 0)
         * @return Texture reference, or nullptr if rendering to default backbuffer
         */
        Texture2DRef GetOutputTexture() const;

        /**
         * @brief Get the effective viewport size for this pass
         * @return Width and height as uvec2
         */
        glm::uvec2 GetViewportSize() const;

        // ============================================================================
        // MODIFICATION (for dynamic passes)
        // ============================================================================

        /**
         * @brief Update the target framebuffer
         * @param target New target framebuffer (nullptr for default)
         */
        void SetTarget(const FrameBufferRef& target);

        /**
         * @brief Update clear color
         */
        void SetClearColor(const glm::vec4& color) { m_Spec.ClearColor = color; }

        /**
         * @brief Update clear flags
         */
        void SetClearFlags(uint32_t flags) { m_Spec.ClearFlags = flags; }

        /**
         * @brief Enable or disable depth testing
         */
        void SetDepthTestEnabled(bool enabled) { m_Spec.DepthTestEnabled = enabled; }

        /**
         * @brief Enable or disable blending
         */
        void SetBlendingEnabled(bool enabled) { m_Spec.BlendingEnabled = enabled; }

        // ============================================================================
        // CALLBACKS (for extensibility)
        // ============================================================================

        using PassCallback = std::function<void(RenderPass*)>;

        /**
         * @brief Set a callback to be invoked when the pass begins
         */
        void SetOnBegin(PassCallback callback) { m_OnBegin = std::move(callback); }

        /**
         * @brief Set a callback to be invoked when the pass ends
         */
        void SetOnEnd(PassCallback callback) { m_OnEnd = std::move(callback); }

    protected:
        RenderPass(const RenderPassSpec& spec);

        /**
         * @brief Apply the render state for this pass
         */
        void ApplyRenderState();

        /**
         * @brief Restore the render state to defaults
         */
        void RestoreRenderState();

    protected:
        RenderPassSpec m_Spec;
        bool m_Active = false;

        // Callbacks
        PassCallback m_OnBegin;
        PassCallback m_OnEnd;

        // State tracking for restoration
        bool m_PreviousDepthTest = true;
        bool m_PreviousDepthWrite = true;
        bool m_PreviousBlending = false;
    };

    // Type alias for convenience
    using RenderPassRef = std::shared_ptr<RenderPass>;
}