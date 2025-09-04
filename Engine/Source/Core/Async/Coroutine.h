#pragma once

#include "vxpch.h"
#include "Core/Debug/ErrorCodes.h"
#include "Core/Debug/Assert.h"

#include <coroutine>
#include <type_traits>
#include <memory>
#include <chrono>
#include <future>
#include <exception>
#include <optional>

namespace Vortex
{
    // ============================================================================
    // FORWARD DECLARATIONS
    // ============================================================================
    
    class CoroutineScheduler;
    template<typename T = void>
    class Task;
    template<typename T = void>
    class Generator;

    // Forward declare for awaitable methods
    CoroutineScheduler* GetCurrentScheduler();

    // ============================================================================
    // COROUTINE CONCEPTS AND TRAITS
    // ============================================================================

    template<typename T>
    concept CoroutineHandle = requires(T t) {
        typename T::promise_type;
        { t.address() } -> std::convertible_to<void*>;
        { t.done() } -> std::convertible_to<bool>;
        t.resume();
        t.destroy();
    };

    template<typename T>
    concept Awaitable = requires(T t) {
        { t.await_ready() } -> std::convertible_to<bool>;
        { t.await_suspend(std::declval<std::coroutine_handle<>>()) };
        { t.await_resume() };
    };

    // ============================================================================
    // COROUTINE PRIORITY SYSTEM
    // ============================================================================

    enum class CoroutinePriority : uint8_t
    {
        Immediate = 0,    // Execute immediately, don't yield to scheduler
        Critical = 1,     // Engine critical tasks (render commands, input)
        High = 2,         // Important gameplay tasks
        Normal = 3,       // Regular tasks (default)
        Low = 4,          // Background tasks (asset loading)
        Idle = 5          // Only when CPU is idle
    };

    // ============================================================================
    // AWAITABLE PRIMITIVES
    // ============================================================================

    /**
     * @brief Suspend execution for a specific duration
     */
    class SleepAwaitable
    {
    public:
        explicit SleepAwaitable(std::chrono::milliseconds duration)
            : m_Duration(duration), m_StartTime(std::chrono::steady_clock::now()) {}

        bool await_ready() const noexcept { return false; }
        
        void await_suspend(std::coroutine_handle<> handle) noexcept;
        
        
        void await_resume() const noexcept {}

    private:
        std::chrono::milliseconds m_Duration;
        std::chrono::steady_clock::time_point m_StartTime;
        
        static CoroutineScheduler* GetCurrentScheduler();
    };

    /**
     * @brief Yield control to scheduler, allowing other coroutines to run
     */
    class YieldAwaitable
    {
    public:
        explicit YieldAwaitable(CoroutinePriority newPriority = CoroutinePriority::Normal)
            : m_NewPriority(newPriority) {}

        bool await_ready() const noexcept { return false; }
        
        void await_suspend(std::coroutine_handle<> handle) noexcept;
        
        void await_resume() const noexcept {}

    private:
        CoroutinePriority m_NewPriority;
        
        static CoroutineScheduler* GetCurrentScheduler();
    };

    /**
     * @brief Await completion of another task
     */
    template<typename T>
    class TaskAwaitable
    {
    public:
        explicit TaskAwaitable(Task<T>&& task) : m_Task(std::move(task)) {}

        bool await_ready() const noexcept 
        { 
            return m_Task.IsCompleted(); 
        }
        
        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            m_Task.SetContinuation(handle);
        }
        
        auto await_resume()
        {
            if constexpr (std::is_void_v<T>)
            {
                m_Task.GetResult();
                return;
            }
            else
            {
                return m_Task.GetResult();
            }
        }

    private:
        Task<T> m_Task;
    };

    /**
     * @brief Await multiple tasks concurrently
     */
    template<typename... Tasks>
    class WhenAllAwaitable
    {
    public:
        explicit WhenAllAwaitable(Tasks&&... tasks) 
            : m_Tasks(std::forward<Tasks>(tasks)...) {}

        bool await_ready() const noexcept 
        {
            return std::apply([](const auto&... tasks) {
                return (tasks.IsCompleted() && ...);
            }, m_Tasks);
        }
        
        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            m_WaitingHandle = handle;
            m_CompletedCount = 0;
            m_TotalCount = sizeof...(Tasks);
            
            std::apply([this](auto&... tasks) {
                (tasks.SetContinuation([this]() {
                    if (++m_CompletedCount == m_TotalCount && m_WaitingHandle)
                    {
                        auto handle = m_WaitingHandle;
                        m_WaitingHandle = nullptr;
                        handle.resume();
                    }
                }), ...);
            }, m_Tasks);
        }
        
        auto await_resume()
        {
            return std::apply([](auto&... tasks) {
                if constexpr ((std::is_void_v<typename std::decay_t<Tasks>::value_type> && ...))
                {
                    (tasks.GetResult(), ...);
                    return;
                }
                else
                {
                    return std::make_tuple(tasks.GetResult()...);
                }
            }, m_Tasks);
        }

    private:
        std::tuple<Tasks...> m_Tasks;
        std::coroutine_handle<> m_WaitingHandle;
        std::atomic<size_t> m_CompletedCount{0};
        size_t m_TotalCount = 0;
    };

    /**
     * @brief Switch to a specific thread for execution
     */
    class SwitchToThreadAwaitable
    {
    public:
        explicit SwitchToThreadAwaitable(std::thread::id targetThreadId)
            : m_TargetThreadId(targetThreadId) {}

        bool await_ready() const noexcept 
        { 
            return std::this_thread::get_id() == m_TargetThreadId; 
        }
        
        void await_suspend(std::coroutine_handle<> handle) noexcept;
        void await_resume() const noexcept {}

    private:
        std::thread::id m_TargetThreadId;
    };

    // ============================================================================
    // CONVENIENCE FUNCTIONS
    // ============================================================================

    /**
     * @brief Sleep for specified duration
     */
    inline SleepAwaitable Sleep(std::chrono::milliseconds duration)
    {
        return SleepAwaitable(duration);
    }

    /**
     * @brief Yield control to scheduler
     */
    inline YieldAwaitable YieldExecution(CoroutinePriority priority = CoroutinePriority::Normal)
    {
        return YieldAwaitable(priority);
    }
    
    // Provide Yield alias only if not conflicting with Windows macro
    #ifndef Yield
    inline YieldAwaitable Yield(CoroutinePriority priority = CoroutinePriority::Normal)
    {
        return YieldAwaitable(priority);
    }
    #endif

    /**
     * @brief Await multiple tasks
     */
    template<typename... Tasks>
    inline WhenAllAwaitable<Tasks...> WhenAll(Tasks&&... tasks)
    {
        return WhenAllAwaitable<Tasks...>(std::forward<Tasks>(tasks)...);
    }

    /**
     * @brief Switch execution to specific thread
     */
    inline SwitchToThreadAwaitable SwitchToThread(std::thread::id threadId)
    {
        return SwitchToThreadAwaitable(threadId);
    }

    // ============================================================================
    // COROUTINE EXCEPTION HANDLING
    // ============================================================================

    class CoroutineException : public std::exception
    {
    public:
        explicit CoroutineException(const std::string& message) 
            : m_Message(message) {}
        
        explicit CoroutineException(const Result<void>& result)
        {
            if (!result.IsSuccess())
            {
                m_Message = result.GetErrorMessage();
                m_ErrorCode = result.GetErrorCode();
            }
        }

        const char* what() const noexcept override { return m_Message.c_str(); }
        ErrorCode GetErrorCode() const noexcept { return m_ErrorCode; }

    private:
        std::string m_Message;
        ErrorCode m_ErrorCode = ErrorCode::Unknown;
    };

    // ============================================================================
    // COROUTINE DEBUGGING SUPPORT
    // ============================================================================

#ifdef VX_DEBUG
    struct CoroutineDebugInfo
    {
        std::string Name;
        std::string SourceLocation;
        std::chrono::steady_clock::time_point CreationTime;
        std::chrono::steady_clock::time_point LastResumeTime;
        CoroutinePriority Priority;
        size_t ResumeCount = 0;
        bool IsCompleted = false;
    };

    class CoroutineDebugger
    {
    public:
        static void RegisterCoroutine(void* handle, const CoroutineDebugInfo& info);
        static void UnregisterCoroutine(void* handle);
        static void UpdateCoroutine(void* handle, const CoroutineDebugInfo& info);
        static std::vector<CoroutineDebugInfo> GetAllCoroutines();
        static CoroutineDebugInfo* GetCoroutineInfo(void* handle);
    };

#define VX_COROUTINE_DEBUG_NAME(name) \
    if constexpr (VX_DEBUG) { \
        Vortex::Async::CoroutineDebugger::UpdateCoroutine( \
            co_await.address(), \
            {name, __FILE__ ":" VX_STRINGIFY(__LINE__), \
             std::chrono::steady_clock::now(), \
             std::chrono::steady_clock::now(), \
             Vortex::Async::CoroutinePriority::Normal, 0, false} \
        ); \
    }
#else
#define VX_COROUTINE_DEBUG_NAME(name)
#endif

} // namespace Vortex::Async
