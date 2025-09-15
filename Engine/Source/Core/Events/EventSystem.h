#pragma once

#include "vxpch.h"
#include "EventDispatcher.h"
#include "Engine/Systems/EngineSystem.h"

namespace Vortex
{
    /**
     * @brief Global event system managing event dispatch across the entire engine
     * 
     * This system provides a centralized event management interface that integrates
     * with the engine's system architecture. It manages the global EventDispatcher
     * and provides convenient access for all engine systems and user code.
     */
    class EventSystem : public EngineSystem
    {
    public:
        EventSystem() : EngineSystem("EventSystem", SystemPriority::Critical) {}
        ~EventSystem() override = default;

        // EngineSystem interface
        Result<void> Initialize() override;
        Result<void> Update() override;
        Result<void> Render() override;
        Result<void> Shutdown() override;

        /**
         * @brief Get the global event dispatcher instance
         * @return Reference to the EventDispatcher
         */
        static EventDispatcher& GetDispatcher() { return s_Instance->m_Dispatcher; }

        /**
         * @brief Subscribe to events of a specific type (static convenience method)
         * @tparam T Event type to subscribe to
         * @param handler Function/lambda to call when event is dispatched
         * @return SubscriptionID for later unsubscription
         */
        template<typename T>
        static SubscriptionID Subscribe(EventHandler<T> handler)
        {
            VX_CORE_ASSERT(s_Instance, "EventSystem not initialized!");
            return s_Instance->m_Dispatcher.Subscribe<T>(std::move(handler));
        }

        /**
         * @brief Subscribe to events with a member function (static convenience method)
         * @tparam T Event type to subscribe to
         * @tparam ClassType Class containing the member function
         * @param instance Pointer to instance of the class
         * @param memberFunc Member function pointer
         * @return SubscriptionID for later unsubscription
         */
        template<typename T, typename ClassType>
        static SubscriptionID Subscribe(ClassType* instance, bool (ClassType::*memberFunc)(const T&))
        {
            VX_CORE_ASSERT(s_Instance, "EventSystem not initialized!");
            return s_Instance->m_Dispatcher.Subscribe<T>(instance, memberFunc);
        }

        /**
         * @brief Dispatch an event immediately (static convenience method)
         * @tparam T Event type (auto-deduced)
         * @param event Event to dispatch
         * @return True if event was handled by any subscriber, false otherwise
         */
        template<typename T>
        static bool Dispatch(const T& event)
        {
            VX_CORE_ASSERT(s_Instance, "EventSystem not initialized!");
            return s_Instance->m_Dispatcher.DispatchImmediate(event);
        }

        /**
         * @brief Queue an event for later dispatch (static convenience method)
         * @tparam T Event type (auto-deduced)
         * @param event Event to queue (will be copied/moved)
         */
        template<typename T>
        static void Queue(T&& event)
        {
            VX_CORE_ASSERT(s_Instance, "EventSystem not initialized!");
            s_Instance->m_Dispatcher.QueueEvent(std::forward<T>(event));
        }

        /**
         * @brief Unsubscribe from events (static convenience method)
         * @param subscriptionId ID returned from Subscribe()
         * @return True if subscription was found and removed, false otherwise
         */
        static bool Unsubscribe(SubscriptionID subscriptionId)
        {
            VX_CORE_ASSERT(s_Instance, "EventSystem not initialized!");
            return s_Instance->m_Dispatcher.Unsubscribe(subscriptionId);
        }

        /**
         * @brief Get statistics about the event system
         */
        struct Statistics
        {
            size_t QueuedEvents = 0;
            size_t ProcessedEventsThisFrame = 0;
            size_t TotalSubscriptions = 0;
        };

        /**
         * @brief Get current event system statistics
         * @return Statistics structure with current metrics
         */
        static Statistics GetStatistics()
        {
            if (!s_Instance) return {};
            
            Statistics stats;
            stats.QueuedEvents = s_Instance->m_Dispatcher.GetQueuedEventCount();
            stats.ProcessedEventsThisFrame = s_Instance->m_ProcessedEventsThisFrame;
            // TotalSubscriptions would require additional tracking
            return stats;
        }

        /**
         * @brief Check if EventSystem is currently initialized
         */
        static bool IsInitialized() { return s_Instance != nullptr; }

    private:
        EventDispatcher m_Dispatcher;
        size_t m_ProcessedEventsThisFrame = 0;
        
        static EventSystem* s_Instance;
        
        friend class Engine; // Allow Engine to set the instance
        static void SetInstance(EventSystem* instance) { s_Instance = instance; }
    };

    // Convenience macros for common event operations
    #define VX_SUBSCRIBE_EVENT(eventType, handler) \
        ::Vortex::EventSystem::Subscribe<eventType>(handler)

    #define VX_SUBSCRIBE_EVENT_METHOD(eventType, instance, method) \
        ::Vortex::EventSystem::Subscribe<eventType>(instance, method)

    #define VX_DISPATCH_EVENT(event) \
        ::Vortex::EventSystem::Dispatch(event)

    #define VX_QUEUE_EVENT(event) \
        ::Vortex::EventSystem::Queue(event)

    #define VX_UNSUBSCRIBE_EVENT(subscriptionId) \
        ::Vortex::EventSystem::Unsubscribe(subscriptionId)
}
