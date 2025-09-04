#pragma once

#include "Coroutine.h"
#include <coroutine>
#include <optional>
#include <variant>

namespace Vortex
{
    // ============================================================================
    // TASK PROMISE TYPES
    // ============================================================================

    template<typename T>
    class TaskPromise
    {
    public:
        using value_type = T;
        using handle_type = std::coroutine_handle<TaskPromise>;

        // Coroutine interface
        Task<T> get_return_object();
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept 
        {
            ResumeContinuation();
            return {};
        }

        // Exception handling
        void unhandled_exception() noexcept
        {
            m_Exception = std::current_exception();
        }

        // Return value handling
        template<typename U>
        void return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
        {
            if constexpr (std::is_constructible_v<T, U>)
            {
                m_Value.emplace(std::forward<U>(value));
            }
            else
            {
                static_assert(std::is_constructible_v<T, U>, "Cannot construct T from provided value");
            }
        }

        // Get the result (throws if exception occurred)
        T GetResult()
        {
            if (m_Exception)
            {
                std::rethrow_exception(m_Exception);
            }

            if (!m_Value.has_value())
            {
                throw CoroutineException("Task completed without returning a value");
            }

            return std::move(*m_Value);
        }

        // Check if task has completed
        bool IsCompleted() const noexcept
        {
            return m_Value.has_value() || m_Exception;
        }

        // Set continuation for when this task completes
        void SetContinuation(std::coroutine_handle<> continuation) noexcept
        {
            m_Continuation = continuation;
        }

        // Resume continuation if set
        void ResumeContinuation() noexcept
        {
            if (m_Continuation)
            {
                auto cont = m_Continuation;
                m_Continuation = nullptr;
                cont.resume();
            }
        }

    private:
        std::optional<T> m_Value;
        std::exception_ptr m_Exception;
        std::coroutine_handle<> m_Continuation;
    };

    // Specialization for void tasks
    template<>
    class TaskPromise<void>
    {
    public:
        using value_type = void;
        using handle_type = std::coroutine_handle<TaskPromise>;

        // Coroutine interface
        Task<void> get_return_object();
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept 
        {
            ResumeContinuation();
            return {};
        }

        // Exception handling
        void unhandled_exception() noexcept
        {
            m_Exception = std::current_exception();
        }

        // Return void handling
        void return_void() noexcept
        {
            m_Completed = true;
        }

        // Get the result (throws if exception occurred)
        void GetResult()
        {
            if (m_Exception)
            {
                std::rethrow_exception(m_Exception);
            }

            if (!m_Completed)
            {
                throw CoroutineException("Task has not completed yet");
            }
        }

        // Check if task has completed
        bool IsCompleted() const noexcept
        {
            return m_Completed || m_Exception;
        }

        // Set continuation for when this task completes
        void SetContinuation(std::coroutine_handle<> continuation) noexcept
        {
            m_Continuation = continuation;
        }

        // Resume continuation if set
        void ResumeContinuation() noexcept
        {
            if (m_Continuation)
            {
                auto cont = m_Continuation;
                m_Continuation = nullptr;
                cont.resume();
            }
        }

    private:
        bool m_Completed = false;
        std::exception_ptr m_Exception;
        std::coroutine_handle<> m_Continuation;
    };

    // ============================================================================
    // TASK CLASS IMPLEMENTATION
    // ============================================================================

    template<typename T>
    class Task
    {
    public:
        using promise_type = TaskPromise<T>;
        using handle_type = std::coroutine_handle<promise_type>;
        using value_type = T;

        // Constructor from coroutine handle
        explicit Task(handle_type handle) noexcept : m_Handle(handle) {}

        // Move constructor
        Task(Task&& other) noexcept : m_Handle(std::exchange(other.m_Handle, {})) {}

        // Move assignment
        Task& operator=(Task&& other) noexcept
        {
            if (this != &other)
            {
                if (m_Handle)
                {
                    m_Handle.destroy();
                }
                m_Handle = std::exchange(other.m_Handle, {});
            }
            return *this;
        }

        // Destructor
        ~Task()
        {
            if (m_Handle)
            {
                m_Handle.destroy();
            }
        }

        // Delete copy operations
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        // ============================================================================
        // AWAITABLE INTERFACE
        // ============================================================================

        bool await_ready() const noexcept
        {
            return !m_Handle || m_Handle.promise().IsCompleted();
        }

        void await_suspend(std::coroutine_handle<> awaiter) noexcept
        {
            if (m_Handle)
            {
                m_Handle.promise().SetContinuation(awaiter);
            }
        }

        auto await_resume()
        {
            if (!m_Handle)
            {
                throw CoroutineException("Task handle is invalid");
            }

            if constexpr (std::is_void_v<T>)
            {
                m_Handle.promise().GetResult();
                return;
            }
            else
            {
                return m_Handle.promise().GetResult();
            }
        }

        // ============================================================================
        // TASK INTERFACE
        // ============================================================================

        /**
         * @brief Check if the task has completed
         */
        bool IsCompleted() const noexcept
        {
            return !m_Handle || m_Handle.promise().IsCompleted();
        }

        /**
         * @brief Check if the task is valid
         */
        bool IsValid() const noexcept
        {
            return static_cast<bool>(m_Handle);
        }

        /**
         * @brief Get the result (blocks until completion)
         * This should generally not be used - prefer await instead
         */
        auto GetResult()
        {
            if (!m_Handle)
            {
                throw CoroutineException("Task handle is invalid");
            }

            // Busy wait until completion (not ideal, but sometimes necessary)
            while (!m_Handle.promise().IsCompleted())
            {
                std::this_thread::yield();
            }

            if constexpr (std::is_void_v<T>)
            {
                m_Handle.promise().GetResult();
                return;
            }
            else
            {
                return m_Handle.promise().GetResult();
            }
        }

        /**
         * @brief Set continuation to be called when task completes
         */
        void SetContinuation(std::coroutine_handle<> continuation) noexcept
        {
            if (m_Handle)
            {
                m_Handle.promise().SetContinuation(continuation);
            }
        }

        /**
         * @brief Set continuation to be called when task completes (callable version)
         */
        template<typename Callable>
        void SetContinuation(Callable&& callable)
        {
            static_assert(std::is_invocable_v<Callable>, "Callable must be invocable with no arguments");
            
            if (m_Handle)
            {
                // Create a simple coroutine that calls the callable
                auto wrapper = [](Callable callable) -> Task<void>
                {
                    callable();
                    co_return;
                };
                
                auto wrapperTask = wrapper(std::forward<Callable>(callable));
                m_Handle.promise().SetContinuation(wrapperTask.m_Handle);
            }
        }

        // ============================================================================
        // STATIC FACTORY METHODS
        // ============================================================================

        /**
         * @brief Create a completed task with a value
         */
        template<typename U = T>
        static Task<T> FromResult(U&& value) requires(!std::is_void_v<T>)
        {
            auto task = []<typename V>(V&& v) -> Task<T>
            {
                co_return std::forward<V>(v);
            }(std::forward<U>(value));

            // Wait for immediate completion
            while (!task.IsCompleted())
            {
                std::this_thread::yield();
            }

            return task;
        }

        /**
         * @brief Create a completed void task
         */
        static Task<void> FromResult() requires(std::is_void_v<T>)
        {
            auto task = []() -> Task<void>
            {
                co_return;
            }();

            // Wait for immediate completion
            while (!task.IsCompleted())
            {
                std::this_thread::yield();
            }

            return task;
        }

        /**
         * @brief Create a task that completes with an exception
         */
        template<typename Exception>
        static Task<T> FromException(Exception&& exception)
        {
            auto task = [](auto ex) -> Task<T>
            {
                throw std::forward<decltype(ex)>(ex);
                if constexpr (!std::is_void_v<T>)
                {
                    co_return T{}; // Never reached
                }
                else
                {
                    co_return;
                }
            }(std::forward<Exception>(exception));

            return task;
        }

    private:
        handle_type m_Handle;
    };

    // ============================================================================
    // PROMISE IMPLEMENTATIONS
    // ============================================================================

    template<typename T>
    inline Task<T> TaskPromise<T>::get_return_object()
    {
        return Task<T>(handle_type::from_promise(*this));
    }

    inline Task<void> TaskPromise<void>::get_return_object()
    {
        return Task<void>(handle_type::from_promise(*this));
    }

    // ============================================================================
    // TASK UTILITY FUNCTIONS
    // ============================================================================

    /**
     * @brief Run a task to completion synchronously
     */
    template<typename T>
    auto RunSync(Task<T>&& task)
    {
        while (!task.IsCompleted())
        {
            std::this_thread::yield();
        }

        if constexpr (std::is_void_v<T>)
        {
            task.GetResult();
            return;
        }
        else
        {
            return task.GetResult();
        }
    }

    /**
     * @brief Create a task that runs on a background thread
     */
    template<typename Callable>
    auto RunAsync(Callable&& callable) -> Task<std::invoke_result_t<Callable>>
    {
        using ReturnType = std::invoke_result_t<Callable>;
        
        auto future = std::async(std::launch::async, std::forward<Callable>(callable));
        
        if constexpr (std::is_void_v<ReturnType>)
        {
            future.get();
            co_return;
        }
        else
        {
            co_return future.get();
        }
    }

    /**
     * @brief Create a task that delays execution
     */
    inline Task<void> Delay(std::chrono::milliseconds duration)
    {
        co_await Sleep(duration);
        co_return;
    }

} // namespace Vortex::Async
