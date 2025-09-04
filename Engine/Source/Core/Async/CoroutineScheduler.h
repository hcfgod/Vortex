#pragma once

#include "Coroutine.h"
#include "Task.h"
#include "Core/Debug/ErrorCodes.h"

#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <chrono>
#include <memory>

namespace Vortex
{
    // ============================================================================
    // SCHEDULER CONFIGURATION
    // ============================================================================

    struct SchedulerConfig
    {
        // Threading configuration
        size_t WorkerThreadCount = 0;        // 0 = auto-detect based on hardware
        bool UseMainThread = true;           // Process coroutines on main thread
        bool UseDedicatedWorkers = true;     // Use dedicated worker threads
        
        // Performance tuning
        size_t MaxCoroutinesPerFrame = 1000; // Max coroutines to process per update
        std::chrono::microseconds TimeSlicePerCoroutine{100}; // Max time per coroutine
        std::chrono::milliseconds FrameBudget{16}; // Max time for coroutines per frame
        
        // Priority configuration
        bool EnablePriorityQueues = true;    // Use priority-based scheduling
        size_t MaxQueueSizePerPriority = 10000; // Max coroutines per priority queue
        
        // Debug configuration
        bool EnableProfiling = false;        // Enable detailed profiling
        bool LogSchedulingEvents = false;    // Log scheduling decisions
        bool EnableDeadlockDetection = true; // Detect potential deadlocks
        
        SchedulerConfig() = default;
    };

    // ============================================================================
    // SCHEDULER STATISTICS
    // ============================================================================

    struct SchedulerStats
    {
        // Execution statistics
        std::atomic<uint64_t> TotalCoroutinesProcessed{0};
        std::atomic<uint64_t> CoroutinesProcessedThisFrame{0};
        std::atomic<uint64_t> TotalYields{0};
        std::atomic<uint64_t> TotalContextSwitches{0};
        
        // Timing statistics
        std::chrono::microseconds AverageExecutionTime{0};
        std::chrono::microseconds TotalExecutionTime{0};
        std::chrono::microseconds LastFrameTime{0};
        
        // Queue statistics
        std::array<std::atomic<size_t>, 6> QueueSizes{}; // Per priority level
        std::atomic<size_t> DelayedCoroutines{0};
        
        // Thread statistics
        std::atomic<size_t> ActiveWorkerThreads{0};
        std::atomic<size_t> IdleWorkerThreads{0};
        
        // Performance statistics
        std::atomic<double> FrameUtilization{0.0}; // Percentage of frame budget used
        std::atomic<uint64_t> FramesOverBudget{0};
        
        void Reset()
        {
            CoroutinesProcessedThisFrame = 0;
            LastFrameTime = std::chrono::microseconds{0};
            for (auto& size : QueueSizes)
                size = 0;
            FrameUtilization = 0.0;
        }
    };

    // ============================================================================
    // COROUTINE SCHEDULER CLASS
    // ============================================================================

    class CoroutineScheduler
    {
    public:
        explicit CoroutineScheduler(const SchedulerConfig& config = {});
        ~CoroutineScheduler();

        // Non-copyable, non-movable
        CoroutineScheduler(const CoroutineScheduler&) = delete;
        CoroutineScheduler& operator=(const CoroutineScheduler&) = delete;
        CoroutineScheduler(CoroutineScheduler&&) = delete;
        CoroutineScheduler& operator=(CoroutineScheduler&&) = delete;

        // ============================================================================
        // LIFECYCLE MANAGEMENT
        // ============================================================================

        /**
         * @brief Initialize the scheduler
         */
        Result<void> Initialize();

        /**
         * @brief Shutdown the scheduler and wait for all tasks to complete
         */
        Result<void> Shutdown();

        /**
         * @brief Check if the scheduler is running
         */
        bool IsRunning() const noexcept { return m_IsRunning.load(); }

        // ============================================================================
        // COROUTINE SCHEDULING
        // ============================================================================

        /**
         * @brief Schedule a coroutine for execution
         */
        void Schedule(std::coroutine_handle<> handle, 
                     CoroutinePriority priority = CoroutinePriority::Normal);

        /**
         * @brief Schedule a coroutine to run after a delay
         */
        void ScheduleAfter(std::coroutine_handle<> handle, 
                          std::chrono::milliseconds delay,
                          CoroutinePriority priority = CoroutinePriority::Normal);

        /**
         * @brief Reschedule a coroutine with different priority
         */
        void Reschedule(std::coroutine_handle<> handle, CoroutinePriority newPriority);

        /**
         * @brief Schedule a coroutine on a specific thread
         */
        void ScheduleOnThread(std::coroutine_handle<> handle, 
                             std::thread::id threadId,
                             CoroutinePriority priority = CoroutinePriority::Normal);

        // ============================================================================
        // EXECUTION CONTROL
        // ============================================================================

        /**
         * @brief Process coroutines for the current frame (call from main thread)
         * @param maxTime Maximum time to spend processing coroutines
         * @return Number of coroutines processed
         */
        size_t ProcessCoroutines(std::chrono::milliseconds maxTime = std::chrono::milliseconds{16});

        /**
         * @brief Process a single batch of coroutines
         * @param maxCount Maximum number of coroutines to process
         * @param maxTime Maximum time to spend processing
         * @return Number of coroutines processed
         */
        size_t ProcessBatch(size_t maxCount = 100, 
                           std::chrono::microseconds maxTime = std::chrono::microseconds{1000});

        /**
         * @brief Yield the current thread to allow other coroutines to run
         */
        void YieldToScheduler();

        // ============================================================================
        // TASK MANAGEMENT
        // ============================================================================

        /**
         * @brief Schedule a task for execution
         */
        template<typename T>
        void ScheduleTask(Task<T>&& task, CoroutinePriority priority = CoroutinePriority::Normal)
        {
            // Extract the coroutine handle from the task and schedule it
            // This is a simplified version - in practice you'd need to access the handle
            // Schedule(task.GetHandle(), priority); // Would need friend access or public method
        }

        /**
         * @brief Run a task immediately and block until completion
         */
        template<typename T>
        auto RunTaskSync(Task<T>&& task)
        {
            return RunSync(std::move(task));
        }

        // ============================================================================
        // THREAD MANAGEMENT
        // ============================================================================

        /**
         * @brief Get the main thread ID
         */
        std::thread::id GetMainThreadId() const noexcept { return m_MainThreadId; }

        /**
         * @brief Check if running on the main thread
         */
        bool IsMainThread() const noexcept 
        { 
            return std::this_thread::get_id() == m_MainThreadId; 
        }

        /**
         * @brief Get the current worker thread ID (if any)
         */
        std::thread::id GetCurrentWorkerThreadId() const noexcept;

        /**
         * @brief Switch execution to a specific thread
         */
        void RequestThreadSwitch(std::coroutine_handle<> handle, std::thread::id targetThread);

        // ============================================================================
        // STATISTICS AND MONITORING
        // ============================================================================

        /**
         * @brief Get scheduler statistics
         */
        const SchedulerStats& GetStats() const noexcept { return m_Stats; }

        /**
         * @brief Reset statistics
         */
        void ResetStats() { m_Stats.Reset(); }

        /**
         * @brief Get detailed profiling information
         */
        std::string GetProfilingInfo() const;

        /**
         * @brief Check if the scheduler is over budget for the current frame
         */
        bool IsOverBudget() const noexcept;

        /**
         * @brief Get the current queue sizes
         */
        std::array<size_t, 6> GetQueueSizes() const;

        // ============================================================================
        // GLOBAL SCHEDULER ACCESS
        // ============================================================================

        /**
         * @brief Get the global scheduler instance
         */
        static CoroutineScheduler* GetGlobal() noexcept { return s_GlobalScheduler; }

        /**
         * @brief Set the global scheduler instance
         */
        static void SetGlobal(CoroutineScheduler* scheduler) noexcept { s_GlobalScheduler = scheduler; }

    private:
        // ============================================================================
        // INTERNAL STRUCTURES
        // ============================================================================

        struct ScheduledCoroutine
        {
            std::coroutine_handle<> Handle;
            CoroutinePriority Priority;
            std::chrono::steady_clock::time_point ScheduleTime;
            std::chrono::steady_clock::time_point NextRunTime;
            std::thread::id PreferredThread;
            size_t ExecutionCount = 0;
            
            ScheduledCoroutine(std::coroutine_handle<> handle, CoroutinePriority priority)
                : Handle(handle), Priority(priority), 
                  ScheduleTime(std::chrono::steady_clock::now()),
                  NextRunTime(ScheduleTime),
                  PreferredThread{}
            {}
        };

        struct DelayedCoroutine
        {
            std::coroutine_handle<> Handle;
            CoroutinePriority Priority;
            std::chrono::steady_clock::time_point WakeTime;
            
            DelayedCoroutine(std::coroutine_handle<> handle, 
                           CoroutinePriority priority,
                           std::chrono::steady_clock::time_point wakeTime)
                : Handle(handle), Priority(priority), WakeTime(wakeTime)
            {}
            
            bool operator>(const DelayedCoroutine& other) const
            {
                return WakeTime > other.WakeTime;
            }
        };

        // ============================================================================
        // PRIVATE METHODS
        // ============================================================================

        void WorkerThreadLoop(size_t threadIndex);
        void ProcessDelayedCoroutines();
        bool ExecuteCoroutine(const ScheduledCoroutine& coroutine);
        CoroutinePriority GetNextPriorityToProcess();
        void UpdateStatistics();

        // ============================================================================
        // MEMBER VARIABLES
        // ============================================================================

        SchedulerConfig m_Config;
        SchedulerStats m_Stats;
        
        // Threading
        std::thread::id m_MainThreadId;
        std::vector<std::unique_ptr<std::thread>> m_WorkerThreads;
        std::atomic<bool> m_IsRunning{false};
        std::atomic<bool> m_ShouldShutdown{false};
        
        // Priority queues
        std::array<std::queue<ScheduledCoroutine>, 6> m_PriorityQueues;
        std::array<std::mutex, 6> m_QueueMutexes;
        
        // Delayed execution
        std::priority_queue<DelayedCoroutine, std::vector<DelayedCoroutine>, std::greater<>> m_DelayedQueue;
        std::mutex m_DelayedQueueMutex;
        
        // Thread-specific queues
        std::unordered_map<std::thread::id, std::queue<ScheduledCoroutine>> m_ThreadQueues;
        std::mutex m_ThreadQueuesMutex;
        
        // Condition variables
        std::condition_variable m_WorkerCondition;
        std::mutex m_WorkerMutex;
        
        // Profiling
        std::chrono::steady_clock::time_point m_FrameStartTime;
        std::chrono::steady_clock::time_point m_LastFrameTime;
        
        // Global instance
        static CoroutineScheduler* s_GlobalScheduler;
    };

    // ============================================================================
    // GLOBAL SCHEDULER FUNCTIONS
    // ============================================================================

    /**
     * @brief Initialize the global coroutine scheduler
     */
    Result<void> InitializeGlobalScheduler(const SchedulerConfig& config = {});

    /**
     * @brief Shutdown the global coroutine scheduler
     */
    Result<void> ShutdownGlobalScheduler();

    /**
     * @brief Get the current scheduler (thread-local or global)
     */
    CoroutineScheduler* GetCurrentScheduler();

} // namespace Vortex::Async
