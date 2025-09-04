#include "vxpch.h"

// Fix Windows Yield macro conflict
#ifdef Yield
#undef Yield
#endif

#include "Coroutine.h"
#include "CoroutineScheduler.h"
#include "Core/Debug/Log.h"

namespace Vortex
{
    // ============================================================================
    // AWAITABLE IMPLEMENTATIONS
    // ============================================================================

    void SleepAwaitable::await_suspend(std::coroutine_handle<> handle) noexcept
    {
        // Schedule resumption after duration
        auto scheduler = GetCurrentScheduler();
        if (scheduler)
        {
            scheduler->ScheduleAfter(handle, m_Duration);
        }
        else
        {
            // Fallback: resume immediately if no scheduler
            handle.resume();
        }
    }

    void YieldAwaitable::await_suspend(std::coroutine_handle<> handle) noexcept
    {
        auto scheduler = GetCurrentScheduler();
        if (scheduler)
        {
            scheduler->Reschedule(handle, m_NewPriority);
        }
        else
        {
            handle.resume();
        }
    }

    CoroutineScheduler* SleepAwaitable::GetCurrentScheduler()
    {
        return CoroutineScheduler::GetGlobal();
    }

    CoroutineScheduler* YieldAwaitable::GetCurrentScheduler()
    {
        return CoroutineScheduler::GetGlobal();
    }

    void SwitchToThreadAwaitable::await_suspend(std::coroutine_handle<> handle) noexcept
    {
        auto scheduler = CoroutineScheduler::GetGlobal();
        if (scheduler)
        {
            scheduler->RequestThreadSwitch(handle, m_TargetThreadId);
        }
        else
        {
            // No scheduler available, resume immediately on current thread
            VX_CORE_WARN("No coroutine scheduler available for thread switch");
            handle.resume();
        }
    }

    // ============================================================================
    // COROUTINE DEBUGGER IMPLEMENTATION
    // ============================================================================

#ifdef VX_DEBUG
    namespace
    {
        static std::unordered_map<void*, CoroutineDebugInfo> s_CoroutineDebugMap;
        static std::mutex s_DebugMapMutex;
    }

    void CoroutineDebugger::RegisterCoroutine(void* handle, const CoroutineDebugInfo& info)
    {
        std::lock_guard<std::mutex> lock(s_DebugMapMutex);
        s_CoroutineDebugMap[handle] = info;
        
        VX_CORE_TRACE("Coroutine registered: {} at {}", info.Name, info.SourceLocation);
    }

    void CoroutineDebugger::UnregisterCoroutine(void* handle)
    {
        std::lock_guard<std::mutex> lock(s_DebugMapMutex);
        auto it = s_CoroutineDebugMap.find(handle);
        if (it != s_CoroutineDebugMap.end())
        {
            VX_CORE_TRACE("Coroutine unregistered: {}", it->second.Name);
            s_CoroutineDebugMap.erase(it);
        }
    }

    void CoroutineDebugger::UpdateCoroutine(void* handle, const CoroutineDebugInfo& info)
    {
        std::lock_guard<std::mutex> lock(s_DebugMapMutex);
        auto it = s_CoroutineDebugMap.find(handle);
        if (it != s_CoroutineDebugMap.end())
        {
            it->second = info;
            it->second.LastResumeTime = std::chrono::steady_clock::now();
            it->second.ResumeCount++;
        }
        else
        {
            // Register if not found
            s_CoroutineDebugMap[handle] = info;
        }
    }

    std::vector<CoroutineDebugInfo> CoroutineDebugger::GetAllCoroutines()
    {
        std::lock_guard<std::mutex> lock(s_DebugMapMutex);
        std::vector<CoroutineDebugInfo> coroutines;
        coroutines.reserve(s_CoroutineDebugMap.size());
        
        for (const auto& [handle, info] : s_CoroutineDebugMap)
        {
            coroutines.push_back(info);
        }
        
        return coroutines;
    }

    CoroutineDebugInfo* CoroutineDebugger::GetCoroutineInfo(void* handle)
    {
        std::lock_guard<std::mutex> lock(s_DebugMapMutex);
        auto it = s_CoroutineDebugMap.find(handle);
        return (it != s_CoroutineDebugMap.end()) ? &it->second : nullptr;
    }
#endif

} // namespace Vortex::Async
