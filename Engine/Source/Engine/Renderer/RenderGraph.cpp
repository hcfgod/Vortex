#include "vxpch.h"
#include "RenderGraph.h"
#include "RenderCommandQueue.h"
#include "RenderCommand.h"
#include "Core/Debug/Log.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Vortex
{
    // ============================================================================
    // GLOBAL INSTANCE
    // ============================================================================

    static RenderGraphRef s_GlobalRenderGraph = nullptr;

    RenderGraph& GetRenderGraph()
    {
        if (!s_GlobalRenderGraph)
        {
            VX_CORE_WARN("GetRenderGraph() called but no global render graph set, creating default");
            s_GlobalRenderGraph = RenderGraph::CreateDefault();
        }
        return *s_GlobalRenderGraph;
    }

    void SetRenderGraph(RenderGraphRef graph)
    {
        s_GlobalRenderGraph = std::move(graph);
        VX_CORE_INFO("Global render graph set");
    }

    bool HasRenderGraph()
    {
        return s_GlobalRenderGraph != nullptr;
    }

    // ============================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ============================================================================

    RenderGraph::RenderGraph()
    {
        VX_CORE_TRACE("RenderGraph created");
    }

    RenderGraph::~RenderGraph()
    {
        // End any active pass
        if (m_CurrentPass && m_CurrentPass->IsActive())
        {
            m_CurrentPass->End();
        }
        m_CurrentPass = nullptr;

        VX_CORE_TRACE("RenderGraph destroyed");
    }

    // ============================================================================
    // FACTORY
    // ============================================================================

    std::shared_ptr<RenderGraph> RenderGraph::Create()
    {
        return std::make_shared<RenderGraph>();
    }

    std::shared_ptr<RenderGraph> RenderGraph::CreateDefault()
    {
        auto graph = std::make_shared<RenderGraph>();

        // --- Background Pass ---
        // First pass, clears everything, sets up depth buffer
        {
            RenderPassSpec spec;
            spec.Name = "Background";
            spec.Domain = RenderDomain::Background;
            spec.ClearFlags = ClearCommand::Color | ClearCommand::Depth | ClearCommand::Stencil;
            spec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
            spec.DepthTestEnabled = true;
            spec.DepthWriteEnabled = true;
            spec.BlendingEnabled = false;
            spec.SortMode = RenderSortMode::None;
            graph->AddPass(RenderPass::Create(spec));
        }

        // --- World3D Pass ---
        // For 3D geometry, uses depth testing, no clear (uses background's clear)
        {
            RenderPassSpec spec;
            spec.Name = "World3D";
            spec.Domain = RenderDomain::World3D;
            spec.ClearFlags = 0;  // Don't clear, render on top of background
            spec.DepthTestEnabled = true;
            spec.DepthWriteEnabled = true;
            spec.BlendingEnabled = false;
            spec.SortMode = RenderSortMode::FrontToBack;  // Opaque optimization
            graph->AddPass(RenderPass::Create(spec));
        }

        // --- World2D Pass ---
        // For 2D game content, depth tested, blending for transparency
        {
            RenderPassSpec spec;
            spec.Name = "World2D";
            spec.Domain = RenderDomain::World2D;
            spec.ClearFlags = 0;  // Don't clear
            spec.DepthTestEnabled = false;  // 2D typically doesn't need depth
            spec.DepthWriteEnabled = false;
            spec.BlendingEnabled = true;
            spec.SrcBlendFactor = BlendFactor::SrcAlpha;
            spec.DstBlendFactor = BlendFactor::OneMinusSrcAlpha;
            spec.SortMode = RenderSortMode::BackToFront;  // For proper transparency
            graph->AddPass(RenderPass::Create(spec));
        }

        // --- Effects Pass ---
        // For particles, post-processing sources
        {
            RenderPassSpec spec;
            spec.Name = "Effects";
            spec.Domain = RenderDomain::Effects;
            spec.ClearFlags = 0;
            spec.DepthTestEnabled = true;
            spec.DepthWriteEnabled = false;  // Read-only depth for particles
            spec.BlendingEnabled = true;
            spec.SrcBlendFactor = BlendFactor::SrcAlpha;
            spec.DstBlendFactor = BlendFactor::One;  // Additive for effects
            spec.SortMode = RenderSortMode::BackToFront;
            graph->AddPass(RenderPass::Create(spec));
        }

        // --- UI Pass ---
        // For user interface - no depth, always on top of game content
        {
            RenderPassSpec spec;
            spec.Name = "UI";
            spec.Domain = RenderDomain::UI;
            spec.ClearFlags = 0;
            spec.DepthTestEnabled = false;  // UI doesn't use depth
            spec.DepthWriteEnabled = false;
            spec.BlendingEnabled = true;
            spec.SrcBlendFactor = BlendFactor::SrcAlpha;
            spec.DstBlendFactor = BlendFactor::OneMinusSrcAlpha;
            spec.SortMode = RenderSortMode::None;  // UI rendered in order
            graph->AddPass(RenderPass::Create(spec));
        }

        // --- Overlay Pass ---
        // For debug overlays, always visible
        {
            RenderPassSpec spec;
            spec.Name = "Overlay";
            spec.Domain = RenderDomain::Overlay;
            spec.ClearFlags = 0;
            spec.DepthTestEnabled = false;
            spec.DepthWriteEnabled = false;
            spec.BlendingEnabled = true;
            spec.SrcBlendFactor = BlendFactor::SrcAlpha;
            spec.DstBlendFactor = BlendFactor::OneMinusSrcAlpha;
            spec.SortMode = RenderSortMode::None;
            graph->AddPass(RenderPass::Create(spec));
        }

        VX_CORE_INFO("Created default RenderGraph with {} passes", graph->GetPassCount());
        return graph;
    }

    // ============================================================================
    // PASS MANAGEMENT
    // ============================================================================

    void RenderGraph::AddPass(RenderPassRef pass)
    {
        if (!pass)
        {
            VX_CORE_WARN("RenderGraph::AddPass() called with nullptr");
            return;
        }

        // Check for duplicate name
        if (m_PassIndexMap.count(pass->GetName()) > 0)
        {
            VX_CORE_WARN("RenderGraph already contains a pass named '{}', skipping", pass->GetName());
            return;
        }

        m_PassIndexMap[pass->GetName()] = m_Passes.size();
        m_Passes.push_back(std::move(pass));

        VX_CORE_TRACE("RenderGraph: Added pass '{}'", m_Passes.back()->GetName());
    }

    bool RenderGraph::InsertPassAfter(const std::string& afterPassName, RenderPassRef pass)
    {
        if (!pass)
        {
            VX_CORE_WARN("RenderGraph::InsertPassAfter() called with nullptr");
            return false;
        }

        auto it = FindPass(afterPassName);
        if (it == m_Passes.end())
        {
            VX_CORE_WARN("RenderGraph: Pass '{}' not found for InsertPassAfter", afterPassName);
            return false;
        }

        // Insert after the found pass
        m_Passes.insert(it + 1, std::move(pass));
        RebuildPassMap();

        VX_CORE_TRACE("RenderGraph: Inserted pass '{}' after '{}'", 
                      (*(it + 1))->GetName(), afterPassName);
        return true;
    }

    bool RenderGraph::InsertPassBefore(const std::string& beforePassName, RenderPassRef pass)
    {
        if (!pass)
        {
            VX_CORE_WARN("RenderGraph::InsertPassBefore() called with nullptr");
            return false;
        }

        auto it = FindPass(beforePassName);
        if (it == m_Passes.end())
        {
            VX_CORE_WARN("RenderGraph: Pass '{}' not found for InsertPassBefore", beforePassName);
            return false;
        }

        std::string newPassName = pass->GetName();
        m_Passes.insert(it, std::move(pass));
        RebuildPassMap();

        VX_CORE_TRACE("RenderGraph: Inserted pass '{}' before '{}'", 
                      newPassName, beforePassName);
        return true;
    }

    bool RenderGraph::RemovePass(const std::string& passName)
    {
        auto it = FindPass(passName);
        if (it == m_Passes.end())
        {
            VX_CORE_WARN("RenderGraph: Pass '{}' not found for removal", passName);
            return false;
        }

        m_Passes.erase(it);
        RebuildPassMap();

        VX_CORE_TRACE("RenderGraph: Removed pass '{}'", passName);
        return true;
    }

    RenderPass* RenderGraph::GetPass(const std::string& passName)
    {
        auto mapIt = m_PassIndexMap.find(passName);
        if (mapIt == m_PassIndexMap.end())
        {
            return nullptr;
        }
        return m_Passes[mapIt->second].get();
    }

    const RenderPass* RenderGraph::GetPass(const std::string& passName) const
    {
        auto mapIt = m_PassIndexMap.find(passName);
        if (mapIt == m_PassIndexMap.end())
        {
            return nullptr;
        }
        return m_Passes[mapIt->second].get();
    }

    void RenderGraph::ClearPasses()
    {
        // End any active pass first
        if (m_CurrentPass && m_CurrentPass->IsActive())
        {
            m_CurrentPass->End();
        }
        m_CurrentPass = nullptr;

        m_Passes.clear();
        m_PassIndexMap.clear();

        VX_CORE_TRACE("RenderGraph: Cleared all passes");
    }

    // ============================================================================
    // EXECUTION
    // ============================================================================

    void RenderGraph::Begin()
    {
        if (m_FrameActive)
        {
            VX_CORE_WARN("RenderGraph::Begin() called while frame already active");
            return;
        }

        m_FrameActive = true;
        m_FrameCounter++;

        // Reset current frame statistics
        m_CurrentFrameStats.Reset();
        m_CurrentFrameStats.FrameNumber = m_FrameCounter;
        m_FrameStartTime = std::chrono::high_resolution_clock::now();

        // Invoke frame begin callback
        if (m_OnFrameBegin)
        {
            m_OnFrameBegin(this);
        }

        if (m_DebugEnabled)
        {
            VX_CORE_TRACE("RenderGraph: Frame {} begun", m_FrameCounter);
        }
    }

    bool RenderGraph::BeginPass(const std::string& passName)
    {
        auto* pass = GetPass(passName);
        if (!pass)
        {
            VX_CORE_WARN("RenderGraph: Pass '{}' not found", passName);
            return false;
        }

        // Find the shared_ptr for this pass
        auto it = FindPass(passName);
        if (it != m_Passes.end())
        {
            BeginPass(*it);
            return true;
        }

        return false;
    }

    void RenderGraph::BeginPass(RenderPassRef pass)
    {
        if (!pass)
        {
            VX_CORE_WARN("RenderGraph::BeginPass() called with nullptr");
            return;
        }

        // End any currently active pass
        if (m_CurrentPass && m_CurrentPass->IsActive())
        {
            if (m_DebugEnabled)
            {
                VX_CORE_WARN("RenderGraph: Ending previous pass '{}' before starting '{}'",
                             m_CurrentPass->GetName(), pass->GetName());
            }
            EndPass();  // Use EndPass to properly record timing
        }

        // Start timing for this pass
        m_PassStartTime = std::chrono::high_resolution_clock::now();

        m_CurrentPass = pass;
        m_CurrentPass->Begin();

        if (m_DebugEnabled)
        {
            VX_CORE_TRACE("RenderGraph: Begun pass '{}' (Domain: {})",
                          pass->GetName(), RenderDomainToString(pass->GetDomain()));
        }
    }

    void RenderGraph::EndPass()
    {
        if (!m_CurrentPass)
        {
            VX_CORE_WARN("RenderGraph::EndPass() called with no active pass");
            return;
        }

        // Calculate pass timing
        auto passEndTime = std::chrono::high_resolution_clock::now();
        float passTimeMs = std::chrono::duration<float, std::milli>(passEndTime - m_PassStartTime).count();

        // Record statistics
        std::string passName = m_CurrentPass->GetName();
        m_CurrentFrameStats.PassesExecuted++;
        m_CurrentFrameStats.ExecutedPassNames.push_back(passName);
        m_CurrentFrameStats.PassTimingsMs[passName] = passTimeMs;

        if (m_DebugEnabled)
        {
            VX_CORE_TRACE("RenderGraph: Ending pass '{}' ({:.3f}ms)", passName, passTimeMs);
        }

        m_CurrentPass->End();
        m_CurrentPass = nullptr;
    }

    void RenderGraph::Execute()
    {
        // End any active pass
        if (m_CurrentPass && m_CurrentPass->IsActive())
        {
            if (m_DebugEnabled)
            {
                VX_CORE_WARN("RenderGraph::Execute() called with active pass '{}', ending it",
                             m_CurrentPass->GetName());
            }
            EndPass();  // Use EndPass to properly record timing
        }

        // Calculate total frame time
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        m_CurrentFrameStats.TotalFrameTimeMs = 
            std::chrono::duration<float, std::milli>(frameEndTime - m_FrameStartTime).count();

        // Copy current stats to last frame stats (for reading during next frame)
        m_LastFrameStats = m_CurrentFrameStats;

        // If we have an output target, bind it for any final compositing
        if (m_OutputTarget)
        {
            GetRenderCommandQueue().BindFramebuffer(
                FramebufferTarget::Framebuffer,
                m_OutputTarget->GetRendererID()
            );
        }
        else
        {
            // Ensure we're back to default framebuffer
            GetRenderCommandQueue().BindFramebuffer(FramebufferTarget::Framebuffer, 0);
        }

        // Invoke frame end callback
        if (m_OnFrameEnd)
        {
            m_OnFrameEnd(this);
        }

        m_FrameActive = false;

        if (m_DebugEnabled)
        {
            VX_CORE_TRACE("RenderGraph: Frame {} executed ({} passes, {:.3f}ms total)",
                          m_LastFrameStats.FrameNumber,
                          m_LastFrameStats.PassesExecuted,
                          m_LastFrameStats.TotalFrameTimeMs);
        }
    }

    RenderDomain RenderGraph::GetCurrentDomain() const
    {
        if (m_CurrentPass)
        {
            return m_CurrentPass->GetDomain();
        }
        // Default to World2D if no pass is active
        return RenderDomain::World2D;
    }

    // ============================================================================
    // RESOURCE MANAGEMENT
    // ============================================================================

    void RenderGraph::Resize(uint32_t width, uint32_t height)
    {
        if (m_Width == width && m_Height == height)
        {
            return;
        }

        m_Width = width;
        m_Height = height;

        // Resize all per-pass framebuffers
        for (auto& [passName, fb] : m_PassFramebuffers)
        {
            if (fb)
            {
                // Recreate framebuffer with new size
                FrameBufferSpec spec = fb->GetSpec();
                spec.Width = width;
                spec.Height = height;
                fb = FrameBuffer::Create(spec);

                // Update the pass's target
                if (auto* pass = GetPass(passName))
                {
                    pass->SetTarget(fb);
                }

                VX_CORE_TRACE("RenderGraph: Resized framebuffer for pass '{}' to {}x{}", 
                              passName, width, height);
            }
        }

        VX_CORE_TRACE("RenderGraph: Resized to {}x{}", width, height);
    }

    // ============================================================================
    // PER-PASS FRAMEBUFFER MANAGEMENT
    // ============================================================================

    FrameBufferRef RenderGraph::CreatePassFramebuffer(const std::string& passName,
                                                       uint32_t width, uint32_t height)
    {
        // Use graph dimensions if not specified
        if (width == 0) width = m_Width > 0 ? m_Width : 1280;
        if (height == 0) height = m_Height > 0 ? m_Height : 720;

        // Get the pass
        auto* pass = GetPass(passName);
        if (!pass)
        {
            VX_CORE_WARN("RenderGraph::CreatePassFramebuffer: Pass '{}' not found", passName);
            return nullptr;
        }

        // Create framebuffer spec based on pass settings
        FrameBufferSpec spec;
        spec.Width = width;
        spec.Height = height;
        spec.ColorAttachmentCount = 1;
        spec.HasDepth = pass->GetSpec().DepthTestEnabled || pass->GetSpec().DepthWriteEnabled;

        // Create the framebuffer
        auto fb = FrameBuffer::Create(spec);
        if (!fb)
        {
            VX_CORE_ERROR("RenderGraph::CreatePassFramebuffer: Failed to create framebuffer for '{}'", 
                          passName);
            return nullptr;
        }

        // Store and assign to pass
        m_PassFramebuffers[passName] = fb;
        pass->SetTarget(fb);

        VX_CORE_INFO("RenderGraph: Created {}x{} framebuffer for pass '{}'", 
                     width, height, passName);

        return fb;
    }

    FrameBufferRef RenderGraph::GetPassFramebuffer(const std::string& passName) const
    {
        auto it = m_PassFramebuffers.find(passName);
        if (it != m_PassFramebuffers.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void RenderGraph::RemovePassFramebuffer(const std::string& passName)
    {
        auto it = m_PassFramebuffers.find(passName);
        if (it != m_PassFramebuffers.end())
        {
            // Clear the pass's target
            if (auto* pass = GetPass(passName))
            {
                pass->SetTarget(nullptr);
            }

            m_PassFramebuffers.erase(it);
            VX_CORE_TRACE("RenderGraph: Removed framebuffer for pass '{}'", passName);
        }
    }

    // ============================================================================
    // DEBUG
    // ============================================================================

    std::string RenderGraph::GetDebugInfo() const
    {
        std::stringstream ss;
        ss << "RenderGraph Debug Info:\n";
        ss << "  Frame: " << m_FrameCounter << "\n";
        ss << "  Pass Count: " << m_Passes.size() << "\n";
        ss << "  Frame Active: " << (m_FrameActive ? "Yes" : "No") << "\n";
        ss << "  Current Pass: " << (m_CurrentPass ? m_CurrentPass->GetName() : "None") << "\n";
        ss << "  Output Target: " << (m_OutputTarget ? "Custom" : "Default") << "\n";
        ss << "  Viewport Size: " << m_Width << "x" << m_Height << "\n";
        ss << "\n  Passes:\n";

        for (size_t i = 0; i < m_Passes.size(); ++i)
        {
            const auto& pass = m_Passes[i];
            bool hasFramebuffer = m_PassFramebuffers.count(pass->GetName()) > 0;
            ss << "    [" << i << "] " << pass->GetName()
               << " (Domain: " << RenderDomainToString(pass->GetDomain()) << ")"
               << (pass->IsActive() ? " [ACTIVE]" : "")
               << (hasFramebuffer ? " [FBO]" : "")
               << "\n";
        }

        // Last frame statistics
        ss << "\n  Last Frame Stats:\n";
        ss << "    Passes Executed: " << m_LastFrameStats.PassesExecuted << "\n";
        ss << "    Total Time: " << std::fixed << std::setprecision(3) 
           << m_LastFrameStats.TotalFrameTimeMs << "ms\n";
        
        if (!m_LastFrameStats.ExecutedPassNames.empty())
        {
            ss << "    Pass Timings:\n";
            for (const auto& name : m_LastFrameStats.ExecutedPassNames)
            {
                auto it = m_LastFrameStats.PassTimingsMs.find(name);
                if (it != m_LastFrameStats.PassTimingsMs.end())
                {
                    ss << "      " << name << ": " << std::fixed << std::setprecision(3) 
                       << it->second << "ms\n";
                }
            }
        }

        return ss.str();
    }

    // ============================================================================
    // PRIVATE HELPERS
    // ============================================================================

    std::vector<RenderPassRef>::iterator RenderGraph::FindPass(const std::string& name)
    {
        return std::find_if(m_Passes.begin(), m_Passes.end(),
            [&name](const RenderPassRef& pass) { return pass->GetName() == name; });
    }

    std::vector<RenderPassRef>::const_iterator RenderGraph::FindPass(const std::string& name) const
    {
        return std::find_if(m_Passes.begin(), m_Passes.end(),
            [&name](const RenderPassRef& pass) { return pass->GetName() == name; });
    }

    void RenderGraph::RebuildPassMap()
    {
        m_PassIndexMap.clear();
        for (size_t i = 0; i < m_Passes.size(); ++i)
        {
            m_PassIndexMap[m_Passes[i]->GetName()] = i;
        }
    }
}

