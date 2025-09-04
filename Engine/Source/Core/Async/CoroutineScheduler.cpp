#include "vxpch.h"
#include "CoroutineScheduler.h"
#include "Core/Debug/Log.h"
#include "Core/Debug/Assert.h"

#include <algorithm>
#include <sstream>

namespace Vortex
{
    // ============================================================================
    // STATIC MEMBERS
    // ============================================================================

    CoroutineScheduler* CoroutineScheduler::s_GlobalScheduler = nullptr;

    // ============================================================================
    // CONSTRUCTOR/DESTRUCTOR
    // ============================================================================

    CoroutineScheduler::CoroutineScheduler(const SchedulerConfig& config)
        : m_Config(config), m_MainThreadId(std::this_thread::get_id())
    {
        if (m_Config.WorkerThreadCount == 0)
        {
            // Auto-detect based on hardware
            m_Config.WorkerThreadCount = std::max(1u, std::thread::hardware_concurrency() - 1);
        }
        
        VX_CORE_INFO("CoroutineScheduler created with {} worker threads", m_Config.WorkerThreadCount);
    }

    CoroutineScheduler::~CoroutineScheduler()
    {
        if (m_IsRunning.load())
        {
            Shutdown();
        }
    }

    // ============================================================================
    // LIFECYCLE MANAGEMENT
    // ============================================================================

    Result<void> CoroutineScheduler::Initialize()
    {
        if (m_IsRunning.load())
        {
            return Result<void>(ErrorCode::InvalidState, "Scheduler already running");
        }

        try
        {
            m_IsRunning.store(true);
            m_ShouldShutdown.store(false);
            
            // Start worker threads if enabled
            if (m_Config.UseDedicatedWorkers && m_Config.WorkerThreadCount > 0)
            {
                m_WorkerThreads.reserve(m_Config.WorkerThreadCount);
                
                for (size_t i = 0; i < m_Config.WorkerThreadCount; ++i)
                {
                    auto thread = std::make_unique<std::thread>(&CoroutineScheduler::WorkerThreadLoop, this, i);
                    m_WorkerThreads.push_back(std::move(thread));
                }
                
                VX_CORE_INFO("Started {} worker threads", m_Config.WorkerThreadCount);
            }

            m_FrameStartTime = std::chrono::steady_clock::now();
            m_LastFrameTime = m_FrameStartTime;
            
            VX_CORE_INFO("CoroutineScheduler initialized successfully");
            return Result<void>();
        }
        catch (const std::exception& e)
        {
            m_IsRunning.store(false);
            return Result<void>(ErrorCode::SystemInitializationFailed, 
                              std::string("Failed to initialize scheduler: ") + e.what());
        }
    }

    Result<void> CoroutineScheduler::Shutdown()
    {
        if (!m_IsRunning.load())
        {
            return Result<void>();
        }

        VX_CORE_INFO("Shutting down CoroutineScheduler...");
        
        // Signal shutdown
        m_ShouldShutdown.store(true);
        m_WorkerCondition.notify_all();
        
        // Wait for worker threads to finish
        for (auto& thread : m_WorkerThreads)
        {
            if (thread && thread->joinable())
            {
                thread->join();
            }
        }
        m_WorkerThreads.clear();
        
        // Clean up any remaining coroutines
        size_t totalDestroyed = 0;
        
        // Clean up priority queues
        for (size_t i = 0; i < m_PriorityQueues.size(); ++i)
        {
            std::lock_guard<std::mutex> lock(m_QueueMutexes[i]);
            while (!m_PriorityQueues[i].empty())
            {
                auto& coroutine = m_PriorityQueues[i].front();
                if (coroutine.Handle)
                {
                    coroutine.Handle.destroy();
                    totalDestroyed++;
                }
                m_PriorityQueues[i].pop();
            }
        }
        
        // Clean up delayed queue
        {
            std::lock_guard<std::mutex> lock(m_DelayedQueueMutex);
            while (!m_DelayedQueue.empty())
            {
                auto& coroutine = const_cast<DelayedCoroutine&>(m_DelayedQueue.top());
                if (coroutine.Handle)
                {
                    coroutine.Handle.destroy();
                    totalDestroyed++;
                }
                m_DelayedQueue.pop();
            }
        }
        
        // Clean up thread-specific queues
        {
            std::lock_guard<std::mutex> lock(m_ThreadQueuesMutex);
            for (auto& [threadId, queue] : m_ThreadQueues)
            {
                while (!queue.empty())
                {
                    auto& coroutine = queue.front();
                    if (coroutine.Handle)
                    {
                        coroutine.Handle.destroy();
                        totalDestroyed++;
                    }
                    queue.pop();
                }
            }
            m_ThreadQueues.clear();
        }
        
        if (totalDestroyed > 0)
        {
            VX_CORE_WARN("Destroyed {} unfinished coroutines during shutdown", totalDestroyed);
        }
        
        m_IsRunning.store(false);
        VX_CORE_INFO("CoroutineScheduler shutdown complete");
        
        return Result<void>();
    }

    // ============================================================================
    // COROUTINE SCHEDULING
    // ============================================================================

    void CoroutineScheduler::Schedule(std::coroutine_handle<> handle, CoroutinePriority priority)
    {
        if (!m_IsRunning.load())
        {
            VX_CORE_WARN("Attempt to schedule coroutine on inactive scheduler");
            return;
        }

        if (!handle || handle.done())
        {
            return; // Invalid or completed coroutine
        }

        auto priorityIndex = static_cast<size_t>(priority);
        VX_CORE_ASSERT(priorityIndex < m_PriorityQueues.size(), "Invalid priority level");
        
        {
            std::lock_guard<std::mutex> lock(m_QueueMutexes[priorityIndex]);
            
            // Check queue size limit
            if (m_PriorityQueues[priorityIndex].size() >= m_Config.MaxQueueSizePerPriority)
            {
                VX_CORE_WARN("Priority queue {} is full, dropping coroutine", priorityIndex);
                handle.destroy();
                return;
            }
            
            m_PriorityQueues[priorityIndex].emplace(handle, priority);
            m_Stats.QueueSizes[priorityIndex].store(m_PriorityQueues[priorityIndex].size());
        }
        
        // Notify worker threads
        m_WorkerCondition.notify_one();
        
        if (m_Config.LogSchedulingEvents)
        {
            VX_CORE_TRACE("Scheduled coroutine with priority {}", static_cast<int>(priority));
        }
    }

    void CoroutineScheduler::ScheduleAfter(std::coroutine_handle<> handle, 
                                          std::chrono::milliseconds delay,
                                          CoroutinePriority priority)
    {
        if (!m_IsRunning.load() || !handle || handle.done())
        {
            return;
        }

        auto wakeTime = std::chrono::steady_clock::now() + delay;
        
        {
            std::lock_guard<std::mutex> lock(m_DelayedQueueMutex);
            m_DelayedQueue.emplace(handle, priority, wakeTime);
            m_Stats.DelayedCoroutines.store(m_DelayedQueue.size());
        }
        
        if (m_Config.LogSchedulingEvents)
        {
            VX_CORE_TRACE("Scheduled coroutine with {}ms delay", delay.count());
        }
    }

    void CoroutineScheduler::Reschedule(std::coroutine_handle<> handle, CoroutinePriority newPriority)
    {
        Schedule(handle, newPriority);
    }

    void CoroutineScheduler::ScheduleOnThread(std::coroutine_handle<> handle, 
                                             std::thread::id threadId,
                                             CoroutinePriority priority)
    {
        if (!m_IsRunning.load() || !handle || handle.done())
        {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_ThreadQueuesMutex);
            m_ThreadQueues[threadId].emplace(handle, priority);
        }
        
        // If it's for the main thread, notify via main processing
        if (threadId == m_MainThreadId)
        {
            // Will be picked up in ProcessCoroutines
        }
        else
        {
            // Notify worker threads
            m_WorkerCondition.notify_all();
        }
        
        if (m_Config.LogSchedulingEvents)
        {
            VX_CORE_TRACE("Scheduled coroutine on specific thread");
        }
    }

    // ============================================================================
    // EXECUTION CONTROL
    // ============================================================================

    size_t CoroutineScheduler::ProcessCoroutines(std::chrono::milliseconds maxTime)
    {
        if (!m_IsRunning.load())
        {
            return 0;
        }

        m_FrameStartTime = std::chrono::steady_clock::now();
        size_t processed = 0;
        
        // Process delayed coroutines first
        ProcessDelayedCoroutines();
        
        // Process thread-specific coroutines for main thread
        auto currentThreadId = std::this_thread::get_id();
        {
            std::lock_guard<std::mutex> lock(m_ThreadQueuesMutex);
            auto it = m_ThreadQueues.find(currentThreadId);
            if (it != m_ThreadQueues.end())
            {
                auto& queue = it->second;
                while (!queue.empty() && processed < m_Config.MaxCoroutinesPerFrame)
                {
                    auto coroutine = queue.front();
                    queue.pop();
                    
                    if (ExecuteCoroutine(coroutine))
                    {
                        processed++;
                    }
                    
                    // Check time budget
                    auto elapsed = std::chrono::steady_clock::now() - m_FrameStartTime;
                    if (elapsed >= maxTime)
                    {
                        break;
                    }
                }
            }
        }
        
        // Process priority queues
        while (processed < m_Config.MaxCoroutinesPerFrame)
        {
            auto priority = GetNextPriorityToProcess();
            if (priority == static_cast<CoroutinePriority>(-1))
            {
                break; // No more coroutines
            }
            
            auto priorityIndex = static_cast<size_t>(priority);
            ScheduledCoroutine coroutine(nullptr, priority);
            
            {
                std::lock_guard<std::mutex> lock(m_QueueMutexes[priorityIndex]);
                if (m_PriorityQueues[priorityIndex].empty())
                {
                    continue;
                }
                
                coroutine = m_PriorityQueues[priorityIndex].front();
                m_PriorityQueues[priorityIndex].pop();
                m_Stats.QueueSizes[priorityIndex].store(m_PriorityQueues[priorityIndex].size());
            }
            
            if (ExecuteCoroutine(coroutine))
            {
                processed++;
            }
            
            // Check time budget
            auto elapsed = std::chrono::steady_clock::now() - m_FrameStartTime;
            if (elapsed >= maxTime)
            {
                if (elapsed > m_Config.FrameBudget)
                {
                    m_Stats.FramesOverBudget++;
                }
                break;
            }
        }
        
        // Update statistics
        m_Stats.CoroutinesProcessedThisFrame = processed;
        m_Stats.TotalCoroutinesProcessed += processed;
        
        auto frameTime = std::chrono::steady_clock::now() - m_FrameStartTime;
        m_Stats.LastFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(frameTime);
        
        if (m_Config.FrameBudget.count() > 0)
        {
            double utilization = static_cast<double>(frameTime.count()) / 
                               static_cast<double>(m_Config.FrameBudget.count()) * 100.0;
            m_Stats.FrameUtilization = utilization;
        }
        
        return processed;
    }

    size_t CoroutineScheduler::ProcessBatch(size_t maxCount, std::chrono::microseconds maxTime)
    {
        if (!m_IsRunning.load())
        {
            return 0;
        }

        auto startTime = std::chrono::steady_clock::now();
        size_t processed = 0;
        
        while (processed < maxCount)
        {
            auto priority = GetNextPriorityToProcess();
            if (priority == static_cast<CoroutinePriority>(-1))
            {
                break; // No more coroutines
            }
            
            auto priorityIndex = static_cast<size_t>(priority);
            ScheduledCoroutine coroutine(nullptr, priority);
            
            {
                std::lock_guard<std::mutex> lock(m_QueueMutexes[priorityIndex]);
                if (m_PriorityQueues[priorityIndex].empty())
                {
                    continue;
                }
                
                coroutine = m_PriorityQueues[priorityIndex].front();
                m_PriorityQueues[priorityIndex].pop();
                m_Stats.QueueSizes[priorityIndex].store(m_PriorityQueues[priorityIndex].size());
            }
            
            if (ExecuteCoroutine(coroutine))
            {
                processed++;
            }
            
            // Check time budget
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed >= maxTime)
            {
                break;
            }
        }
        
        return processed;
    }

    void CoroutineScheduler::YieldToScheduler()
    {
        // This is typically called from within a coroutine
        // The coroutine should use co_await Yield() instead
        std::this_thread::yield();
    }

    // ============================================================================
    // PRIVATE METHODS
    // ============================================================================

    void CoroutineScheduler::WorkerThreadLoop(size_t threadIndex)
    {
        VX_CORE_TRACE("Worker thread {} started", threadIndex);
        
        while (!m_ShouldShutdown.load())
        {
            std::unique_lock<std::mutex> lock(m_WorkerMutex);
            
            // Wait for work or shutdown signal
            m_WorkerCondition.wait_for(lock, std::chrono::milliseconds(100), [this]() {
                return m_ShouldShutdown.load() || GetNextPriorityToProcess() != static_cast<CoroutinePriority>(-1);
            });
            
            if (m_ShouldShutdown.load())
            {
                break;
            }
            
            lock.unlock();
            
            // Process available coroutines
            size_t processed = ProcessBatch(50, std::chrono::microseconds(1000));
            
            if (processed == 0)
            {
                // No work available, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            
            m_Stats.ActiveWorkerThreads.store(m_WorkerThreads.size());
        }
        
        VX_CORE_TRACE("Worker thread {} stopped", threadIndex);
    }

    void CoroutineScheduler::ProcessDelayedCoroutines()
    {
        std::lock_guard<std::mutex> lock(m_DelayedQueueMutex);
        auto now = std::chrono::steady_clock::now();
        
        while (!m_DelayedQueue.empty())
        {
            const auto& delayedCoroutine = m_DelayedQueue.top();
            
            if (delayedCoroutine.WakeTime > now)
            {
                break; // Not ready yet
            }
            
            // Move to appropriate priority queue
            Schedule(delayedCoroutine.Handle, delayedCoroutine.Priority);
            m_DelayedQueue.pop();
        }
        
        m_Stats.DelayedCoroutines.store(m_DelayedQueue.size());
    }

    bool CoroutineScheduler::ExecuteCoroutine(const ScheduledCoroutine& coroutine)
    {
        if (!coroutine.Handle || coroutine.Handle.done())
        {
            return false;
        }

        auto startTime = std::chrono::steady_clock::now();
        
        try
        {
            coroutine.Handle.resume();
            
            auto endTime = std::chrono::steady_clock::now();
            auto executionTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            
            // Update statistics
            m_Stats.TotalExecutionTime += executionTime;
            
            if (m_Config.EnableProfiling)
            {
                // Update average execution time (simple moving average)
                auto totalCoroutines = m_Stats.TotalCoroutinesProcessed.load();
                if (totalCoroutines > 0)
                {
                    m_Stats.AverageExecutionTime = std::chrono::microseconds(
                        (m_Stats.AverageExecutionTime.count() * (totalCoroutines - 1) + executionTime.count()) / totalCoroutines
                    );
                }
                else
                {
                    m_Stats.AverageExecutionTime = executionTime;
                }
            }
            
            // Check if coroutine completed
            if (coroutine.Handle.done())
            {
                // Coroutine completed, destroy it
                coroutine.Handle.destroy();
            }
            else
            {
                // Coroutine yielded, reschedule if needed
                // This would typically be handled by the awaitable that caused the suspension
            }
            
            return true;
        }
        catch (const std::exception& e)
        {
            VX_CORE_ERROR("Exception in coroutine execution: {}", e.what());
            
            // Destroy the coroutine to prevent further issues
            if (coroutine.Handle && !coroutine.Handle.done())
            {
                coroutine.Handle.destroy();
            }
            
            return false;
        }
    }

    CoroutinePriority CoroutineScheduler::GetNextPriorityToProcess()
    {
        // Process higher priority queues first
        for (int i = 0; i < static_cast<int>(m_PriorityQueues.size()); ++i)
        {
            std::lock_guard<std::mutex> lock(m_QueueMutexes[i]);
            if (!m_PriorityQueues[i].empty())
            {
                return static_cast<CoroutinePriority>(i);
            }
        }
        
        return static_cast<CoroutinePriority>(-1); // No work available
    }

    void CoroutineScheduler::UpdateStatistics()
    {
        // Statistics are updated in real-time by other methods
        // This method could be used for periodic cleanup or aggregation
    }

    // ============================================================================
    // PUBLIC QUERY METHODS
    // ============================================================================

    std::thread::id CoroutineScheduler::GetCurrentWorkerThreadId() const noexcept
    {
        auto currentId = std::this_thread::get_id();
        
        // Check if current thread is a worker thread
        for (const auto& thread : m_WorkerThreads)
        {
            if (thread && thread->get_id() == currentId)
            {
                return currentId;
            }
        }
        
        return {}; // Not a worker thread
    }

    void CoroutineScheduler::RequestThreadSwitch(std::coroutine_handle<> handle, std::thread::id targetThread)
    {
        ScheduleOnThread(handle, targetThread);
    }

    std::string CoroutineScheduler::GetProfilingInfo() const
    {
        std::ostringstream oss;
        oss << "CoroutineScheduler Profiling Info:\n";
        oss << "  Total Coroutines Processed: " << m_Stats.TotalCoroutinesProcessed.load() << "\n";
        oss << "  Coroutines This Frame: " << m_Stats.CoroutinesProcessedThisFrame.load() << "\n";
        oss << "  Average Execution Time: " << m_Stats.AverageExecutionTime.count() << " μs\n";
        oss << "  Total Execution Time: " << m_Stats.TotalExecutionTime.count() << " μs\n";
        oss << "  Last Frame Time: " << m_Stats.LastFrameTime.count() << " μs\n";
        oss << "  Frame Utilization: " << m_Stats.FrameUtilization.load() << "%\n";
        oss << "  Frames Over Budget: " << m_Stats.FramesOverBudget.load() << "\n";
        oss << "  Delayed Coroutines: " << m_Stats.DelayedCoroutines.load() << "\n";
        oss << "  Active Worker Threads: " << m_Stats.ActiveWorkerThreads.load() << "\n";
        
        oss << "  Queue Sizes by Priority:\n";
        for (size_t i = 0; i < m_Stats.QueueSizes.size(); ++i)
        {
            oss << "    Priority " << i << ": " << m_Stats.QueueSizes[i].load() << "\n";
        }
        
        return oss.str();
    }

    bool CoroutineScheduler::IsOverBudget() const noexcept
    {
        auto elapsed = std::chrono::steady_clock::now() - m_FrameStartTime;
        return elapsed > m_Config.FrameBudget;
    }

    std::array<size_t, 6> CoroutineScheduler::GetQueueSizes() const
    {
        std::array<size_t, 6> sizes;
        for (size_t i = 0; i < sizes.size(); ++i)
        {
            sizes[i] = m_Stats.QueueSizes[i].load();
        }
        return sizes;
    }

    // ============================================================================
    // GLOBAL SCHEDULER FUNCTIONS
    // ============================================================================

    static std::unique_ptr<CoroutineScheduler> s_GlobalSchedulerInstance;

    Result<void> InitializeGlobalScheduler(const SchedulerConfig& config)
    {
        if (s_GlobalSchedulerInstance)
        {
            VX_CORE_WARN("Global coroutine scheduler already initialized");
            return Result<void>();
        }

        try
        {
            s_GlobalSchedulerInstance = std::make_unique<CoroutineScheduler>(config);
            CoroutineScheduler::SetGlobal(s_GlobalSchedulerInstance.get());
            
            auto result = s_GlobalSchedulerInstance->Initialize();
            if (!result.IsSuccess())
            {
                s_GlobalSchedulerInstance.reset();
                CoroutineScheduler::SetGlobal(nullptr);
                return result;
            }
            
            VX_CORE_INFO("Global coroutine scheduler initialized successfully");
            return Result<void>();
        }
        catch (const std::exception& e)
        {
            return Result<void>(ErrorCode::SystemInitializationFailed, 
                              std::string("Failed to initialize global scheduler: ") + e.what());
        }
    }

    Result<void> ShutdownGlobalScheduler()
    {
        if (!s_GlobalSchedulerInstance)
        {
            return Result<void>();
        }

        auto result = s_GlobalSchedulerInstance->Shutdown();
        s_GlobalSchedulerInstance.reset();
        CoroutineScheduler::SetGlobal(nullptr);
        
        VX_CORE_INFO("Global coroutine scheduler shutdown complete");
        return result;
    }

    CoroutineScheduler* GetCurrentScheduler()
    {
        return CoroutineScheduler::GetGlobal();
    }

} // namespace Vortex::Async
