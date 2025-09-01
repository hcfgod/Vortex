#include "vxpch.h"
#include "EventSystem.h"
#include "Core/Debug/Log.h"

namespace Vortex
{
    // Static instance pointer
    EventSystem* EventSystem::s_Instance = nullptr;

    Result<void> EventSystem::Initialize()
    {
        // Set the global instance
        VX_CORE_ASSERT(!s_Instance, "EventSystem already initialized!");
        s_Instance = this;
        
        VX_CORE_INFO("EventSystem: Initialized successfully");
        return Result<void>();
    }

    Result<void> EventSystem::Update()
    {
        // Process queued events each frame
        // Limit the number of events processed per frame to prevent frame rate issues
        constexpr size_t MAX_EVENTS_PER_FRAME = 100;
        
        m_ProcessedEventsThisFrame = m_Dispatcher.ProcessQueuedEvents(MAX_EVENTS_PER_FRAME);
        
        // Events processed silently each frame
        
        return Result<void>();
    }

    Result<void> EventSystem::Render()
    {
        // No rendering needed for event system
        return Result<void>();
    }

    Result<void> EventSystem::Shutdown()
    {
        VX_CORE_INFO("EventSystem: Shutting down...");
        
        // Clear all subscriptions and queued events
        m_Dispatcher.Clear();
        
        // Clear the global instance
        s_Instance = nullptr;
        
        VX_CORE_INFO("EventSystem: Shutdown complete");
        return Result<void>();
    }
}
