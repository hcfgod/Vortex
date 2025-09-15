#include "vxpch.h"
#include "RenderCommandQueue.h"
#include "RendererAPI.h"
#include "GraphicsContext.h"
#include "Core/Debug/Log.h"
#include "Core/Debug/Assert.h"

namespace Vortex 
{

    RenderCommandQueue::RenderCommandQueue(const Config& config)
        : m_Config(config) {}

    RenderCommandQueue::~RenderCommandQueue() {
        if (m_Initialized.load()) {
            FlushAll();
            Shutdown();
        }
    }

    Result<void> RenderCommandQueue::Initialize(GraphicsContext* context) {
        if (m_Initialized.load()) return Result<void>();
        if (!context) return Result<void>(ErrorCode::NullPointer, "GraphicsContext is null");

        m_GraphicsContext = context;
        m_RenderThreadID = std::this_thread::get_id();
        m_Initialized.store(true);
        ResetStatistics();
        VX_CORE_INFO("RenderCommandQueue initialized (render thread set)");
        return Result<void>();
    }

    Result<void> RenderCommandQueue::Shutdown() {
        if (!m_Initialized.load()) return Result<void>();

        ClearQueue();
        m_Initialized.store(false);
        m_GraphicsContext = nullptr;
        return Result<void>();
    }

    bool RenderCommandQueue::Submit(std::unique_ptr<RenderCommand> command, bool executeImmediate) {
        if (!command) return false;

        // Enforce that queued submissions happen on the render thread
        VX_CORE_ASSERT(executeImmediate || IsOnRenderThread(), "Queued render command submitted from non-render thread; pass executeImmediate=true if intentional");

        if (executeImmediate || IsOnRenderThread()) {
            std::lock_guard<std::mutex> lock(m_ExecutionMutex);
            auto start = std::chrono::high_resolution_clock::now();
            // Capture command info before moving
            const std::string cmdName = command->GetDebugName();
            const float cmdCost = command->GetEstimatedCost();
            auto res = ExecuteCommand(std::move(command));
            auto end = std::chrono::high_resolution_clock::now();
            m_Stats.ProcessedCommands++;
            UpdateStatistics(cmdName, cmdCost, std::chrono::duration_cast<std::chrono::microseconds>(end - start));
            return res.IsSuccess();
        }

        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            if (m_CommandQueue.size() >= m_Config.MaxQueuedCommands) {
                if (m_Config.WarnOnDrop) {
                    VX_CORE_WARN("RenderCommandQueue full ({}). Dropping command.", m_Config.MaxQueuedCommands);
                }
                m_Stats.DroppedCommands++;
                return false;
            }
            m_CommandQueue.push(std::move(command));
            m_Stats.QueuedCommands++;
            m_Stats.TotalCommandsThisFrame++;
        }
        return true;
    }

    size_t RenderCommandQueue::SubmitBatch(std::vector<std::unique_ptr<RenderCommand>> commands, bool executeImmediate) {
        size_t submitted = 0;
        for (auto& cmd : commands) {
            if (Submit(std::move(cmd), executeImmediate)) submitted++;
        }
        return submitted;
    }

    Result<void> RenderCommandQueue::ProcessCommands(size_t maxCommands) {
        if (!IsOnRenderThread()) {
            VX_CORE_WARN("ProcessCommands called from non-render thread");
        }

        const size_t processLimit = (maxCommands == 0) ? std::numeric_limits<size_t>::max() : maxCommands;
        size_t processed = 0;
        auto startBatch = std::chrono::high_resolution_clock::now();

        while (processed < processLimit) {
            std::unique_ptr<RenderCommand> cmd;
            {
                std::lock_guard<std::mutex> lock(m_QueueMutex);
                if (m_CommandQueue.empty()) break;
                cmd = std::move(m_CommandQueue.front());
                m_CommandQueue.pop();
            }

            auto start = std::chrono::high_resolution_clock::now();
            // Capture command info before moving
            const std::string cmdName = cmd->GetDebugName();
            const float cmdCost = cmd->GetEstimatedCost();
            auto res = ExecuteCommand(std::move(cmd));
            auto end = std::chrono::high_resolution_clock::now();

            if (!res.IsSuccess()) {
                VX_CORE_ERROR("Render command failed: {}", res.GetErrorMessage());
            }

            processed++;
            m_Stats.ProcessedCommands++;
            UpdateStatistics(cmdName, cmdCost, std::chrono::duration_cast<std::chrono::microseconds>(end - start));
        }

        m_Stats.LastProcessTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startBatch);
        return Result<void>();
    }

    Result<void> RenderCommandQueue::FlushAll() {
        return ProcessCommands(0);
    }

    size_t RenderCommandQueue::ClearQueue() {
        size_t cleared = 0;
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        while (!m_CommandQueue.empty()) {
            m_CommandQueue.pop();
            cleared++;
        }
        return cleared;
    }

    void RenderCommandQueue::ResetStatistics() {
        std::lock_guard<std::mutex> lock(m_StatsMutex);
        m_Stats = Statistics{};
    }

    size_t RenderCommandQueue::GetQueueSize() const {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        return m_CommandQueue.size();
    }

    bool RenderCommandQueue::IsQueueFull() const {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        return m_CommandQueue.size() >= m_Config.MaxQueuedCommands;
    }

    bool RenderCommandQueue::IsOnRenderThread() const {
        return std::this_thread::get_id() == m_RenderThreadID;
    }

    Result<void> RenderCommandQueue::ExecuteCommand(std::unique_ptr<RenderCommand> command) {
        if (!m_GraphicsContext) {
            return Result<void>(ErrorCode::InvalidState, "RenderCommandQueue not initialized");
        }
        return command->Execute(m_GraphicsContext);
    }

    void RenderCommandQueue::UpdateStatistics(const std::string& commandName, float estimatedCost, std::chrono::microseconds executionTime) {
        std::lock_guard<std::mutex> lock(m_StatsMutex);
        m_Stats.TotalCost += static_cast<float>(executionTime.count());
        m_Stats.EstimatedTotalCost += estimatedCost;
        m_Stats.EstimatedCostByCommand[commandName] += estimatedCost;
        m_Stats.CountByCommand[commandName] += 1;
        if (m_Stats.ProcessedCommands > 0) {
            m_Stats.AverageCost = m_Stats.EstimatedTotalCost / static_cast<float>(m_Stats.ProcessedCommands);
        }
    }

    // ===================== Global Render Command Queue =====================

    static std::unique_ptr<RenderCommandQueue> s_GlobalRenderQueue;

    RenderCommandQueue& GetRenderCommandQueue() {
        VX_CORE_ASSERT(s_GlobalRenderQueue, "Global RenderCommandQueue not initialized");
        return *s_GlobalRenderQueue;
    }

    Result<void> InitializeGlobalRenderQueue(GraphicsContext* context, const RenderCommandQueue::Config& config) {
        if (s_GlobalRenderQueue) {
            // Reinitialize with new context
            s_GlobalRenderQueue->Shutdown();
        } else {
            s_GlobalRenderQueue = std::make_unique<RenderCommandQueue>(config);
        }
        return s_GlobalRenderQueue->Initialize(context);
    }

    Result<void> ShutdownGlobalRenderQueue() {
        if (!s_GlobalRenderQueue) return Result<void>();
        auto res = s_GlobalRenderQueue->Shutdown();
        s_GlobalRenderQueue.reset();
        return res;
    }

} // namespace Vortex