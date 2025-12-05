#include "vxpch.h"
#include "RenderPass.h"
#include "RenderCommandQueue.h"
#include "Engine/Systems/RenderSystem.h"
#include "Engine/Systems/SystemAccessors.h"
#include "Core/Debug/Log.h"

namespace Vortex
{
    // ============================================================================
    // FACTORY
    // ============================================================================

    std::shared_ptr<RenderPass> RenderPass::Create(const RenderPassSpec& spec)
    {
        return std::shared_ptr<RenderPass>(new RenderPass(spec));
    }

    // ============================================================================
    // CONSTRUCTOR
    // ============================================================================

    RenderPass::RenderPass(const RenderPassSpec& spec)
        : m_Spec(spec)
    {
        VX_CORE_TRACE("RenderPass '{}' created (Domain: {})", 
                      m_Spec.Name, RenderDomainToString(m_Spec.Domain));
    }

    // ============================================================================
    // LIFECYCLE
    // ============================================================================

    void RenderPass::Begin()
    {
        if (m_Active)
        {
            VX_CORE_WARN("RenderPass '{}' Begin() called while already active", m_Spec.Name);
            return;
        }

        m_Active = true;

        // Invoke begin callback if set
        if (m_OnBegin)
        {
            m_OnBegin(this);
        }

        // Only bind framebuffer if this pass has a specific target set
        // If no target is set, we respect whatever framebuffer is currently bound
        // (e.g., the scene framebuffer set by RenderSystem/EditorLayer)
        if (m_Spec.Target)
        {
            GetRenderCommandQueue().BindFramebuffer(
                FramebufferTarget::Framebuffer, 
                m_Spec.Target->GetRendererID()
            );

            // Set viewport to match target size
            const auto& fbSpec = m_Spec.Target->GetSpec();
            uint32_t vpX = m_Spec.ViewportX;
            uint32_t vpY = m_Spec.ViewportY;
            uint32_t vpW = (m_Spec.ViewportWidth > 0) ? m_Spec.ViewportWidth : fbSpec.Width;
            uint32_t vpH = (m_Spec.ViewportHeight > 0) ? m_Spec.ViewportHeight : fbSpec.Height;

            if (vpW > 0 && vpH > 0)
            {
                GetRenderCommandQueue().SetViewport(vpX, vpY, vpW, vpH);
            }

            // Clear if specified (only when we have our own target)
            if (m_Spec.ClearFlags != 0)
            {
                GetRenderCommandQueue().Clear(
                    m_Spec.ClearFlags,
                    m_Spec.ClearColor,
                    m_Spec.ClearDepth,
                    m_Spec.ClearStencil
                );
            }
        }
        // If no target, don't change framebuffer or viewport - use whatever is currently set

        // Apply render state
        ApplyRenderState();

        // Note: Per-frame logging removed to reduce spam. 
        // Use RenderGraph debug mode for pass tracking.
    }

    void RenderPass::End()
    {
        if (!m_Active)
        {
            VX_CORE_WARN("RenderPass '{}' End() called while not active", m_Spec.Name);
            return;
        }

        // Restore render state to defaults
        RestoreRenderState();

        // Invoke end callback if set
        if (m_OnEnd)
        {
            m_OnEnd(this);
        }

        m_Active = false;

        // Note: Per-frame logging removed to reduce spam.
        // Use RenderGraph debug mode for pass tracking.
    }

    // ============================================================================
    // ACCESSORS
    // ============================================================================

    Texture2DRef RenderPass::GetOutputTexture() const
    {
        if (m_Spec.Target)
        {
            return m_Spec.Target->GetColorAttachment(0);
        }
        return nullptr;
    }

    glm::uvec2 RenderPass::GetViewportSize() const
    {
        // If viewport size is explicitly set, use that
        if (m_Spec.ViewportWidth > 0 && m_Spec.ViewportHeight > 0)
        {
            return { m_Spec.ViewportWidth, m_Spec.ViewportHeight };
        }

        // If we have a target framebuffer, use its size
        if (m_Spec.Target)
        {
            const auto& spec = m_Spec.Target->GetSpec();
            return { spec.Width, spec.Height };
        }

        // Fall back to RenderSystem's current viewport size
        if (auto* rs = Sys<RenderSystem>())
        {
            return rs->GetCurrentViewportSize();
        }

        return { 0, 0 };
    }

    // ============================================================================
    // MODIFICATION
    // ============================================================================

    void RenderPass::SetTarget(const FrameBufferRef& target)
    {
        if (m_Active)
        {
            VX_CORE_WARN("Cannot change RenderPass target while pass is active");
            return;
        }
        m_Spec.Target = target;
    }

    // ============================================================================
    // RENDER STATE
    // ============================================================================

    void RenderPass::ApplyRenderState()
    {
        // Map our DepthCompareFunc to RenderCommand's CompareFunction
        auto mapDepthFunc = [](DepthCompareFunc func) -> uint32_t
        {
            switch (func)
            {
                case DepthCompareFunc::Never:        return SetDepthStateCommand::Never;
                case DepthCompareFunc::Less:         return SetDepthStateCommand::Less;
                case DepthCompareFunc::Equal:        return SetDepthStateCommand::Equal;
                case DepthCompareFunc::LessEqual:    return SetDepthStateCommand::LessEqual;
                case DepthCompareFunc::Greater:      return SetDepthStateCommand::Greater;
                case DepthCompareFunc::NotEqual:     return SetDepthStateCommand::NotEqual;
                case DepthCompareFunc::GreaterEqual: return SetDepthStateCommand::GreaterEqual;
                case DepthCompareFunc::Always:       return SetDepthStateCommand::Always;
                default:                             return SetDepthStateCommand::Less;
            }
        };

        // Map our BlendFactor to RenderCommand's BlendFactor
        auto mapBlendFactor = [](BlendFactor factor) -> uint32_t
        {
            switch (factor)
            {
                case BlendFactor::Zero:                  return SetBlendStateCommand::Zero;
                case BlendFactor::One:                   return SetBlendStateCommand::One;
                case BlendFactor::SrcColor:              return SetBlendStateCommand::SrcColor;
                case BlendFactor::OneMinusSrcColor:      return SetBlendStateCommand::OneMinusSrcColor;
                case BlendFactor::DstColor:              return SetBlendStateCommand::DstColor;
                case BlendFactor::OneMinusDstColor:      return SetBlendStateCommand::OneMinusDstColor;
                case BlendFactor::SrcAlpha:              return SetBlendStateCommand::SrcAlpha;
                case BlendFactor::OneMinusSrcAlpha:      return SetBlendStateCommand::OneMinusSrcAlpha;
                case BlendFactor::DstAlpha:              return SetBlendStateCommand::DstAlpha;
                case BlendFactor::OneMinusDstAlpha:      return SetBlendStateCommand::OneMinusDstAlpha;
                case BlendFactor::ConstantColor:         return SetBlendStateCommand::ConstantColor;
                case BlendFactor::OneMinusConstantColor: return SetBlendStateCommand::OneMinusConstantColor;
                case BlendFactor::ConstantAlpha:         return SetBlendStateCommand::ConstantAlpha;
                case BlendFactor::OneMinusConstantAlpha: return SetBlendStateCommand::OneMinusConstantAlpha;
                default:                                 return SetBlendStateCommand::One;
            }
        };

        // Map our BlendOp to RenderCommand's BlendOperation
        auto mapBlendOp = [](BlendOp op) -> uint32_t
        {
            switch (op)
            {
                case BlendOp::Add:             return SetBlendStateCommand::Add;
                case BlendOp::Subtract:        return SetBlendStateCommand::Subtract;
                case BlendOp::ReverseSubtract: return SetBlendStateCommand::ReverseSubtract;
                case BlendOp::Min:             return SetBlendStateCommand::Min;
                case BlendOp::Max:             return SetBlendStateCommand::Max;
                default:                       return SetBlendStateCommand::Add;
            }
        };

        // Apply depth state
        GetRenderCommandQueue().SetDepthState(
            m_Spec.DepthTestEnabled,
            m_Spec.DepthWriteEnabled,
            static_cast<SetDepthStateCommand::CompareFunction>(mapDepthFunc(m_Spec.DepthCompare))
        );

        // Apply blend state
        GetRenderCommandQueue().SetBlendState(
            m_Spec.BlendingEnabled,
            static_cast<SetBlendStateCommand::BlendFactor>(mapBlendFactor(m_Spec.SrcBlendFactor)),
            static_cast<SetBlendStateCommand::BlendFactor>(mapBlendFactor(m_Spec.DstBlendFactor)),
            static_cast<SetBlendStateCommand::BlendOperation>(mapBlendOp(m_Spec.BlendOperation))
        );

        // Apply cull state
        if (m_Spec.CullingEnabled)
        {
            SetCullStateCommand::CullMode cullMode = m_Spec.CullBackFace ? 
                SetCullStateCommand::Back : SetCullStateCommand::Front;
            GetRenderCommandQueue().SetCullState(cullMode, SetCullStateCommand::CounterClockwise);
        }
        else
        {
            GetRenderCommandQueue().SetCullState(SetCullStateCommand::None, SetCullStateCommand::CounterClockwise);
        }
    }

    void RenderPass::RestoreRenderState()
    {
        // Restore to sensible defaults
        // Depth: enabled, write enabled, Less compare
        GetRenderCommandQueue().SetDepthState(true, true, SetDepthStateCommand::Less);

        // Blending: disabled
        GetRenderCommandQueue().SetBlendState(false);

        // Culling: disabled
        GetRenderCommandQueue().SetCullState(SetCullStateCommand::None, SetCullStateCommand::CounterClockwise);
    }
}