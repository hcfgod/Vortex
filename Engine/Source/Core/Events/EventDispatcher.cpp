#include "vxpch.h"
#include "EventDispatcher.h"

namespace Vortex
{
    EventDispatcher::EventDispatcher()
    {
        // EventDispatcher initialized silently
    }

    EventDispatcher::~EventDispatcher()
    {
        Clear();
        // EventDispatcher destroyed silently
    }

    bool EventDispatcher::Unsubscribe(SubscriptionID subscriptionId)
    {
        std::lock_guard<std::shared_mutex> lock(m_Mutex);
        
        // Find the event type for this subscription ID
        auto typeIt = m_SubscriptionTypes.find(subscriptionId);
        if (typeIt == m_SubscriptionTypes.end())
        {
            VX_CORE_WARN("Subscription ID {} not found", subscriptionId);
            return false;
        }
        
        std::type_index eventType = typeIt->second;
        
        // Find the storage for this event type
        auto storageIt = m_Subscribers.find(eventType);
        if (storageIt == m_Subscribers.end())
        {
            VX_CORE_ERROR("Storage for event type not found for subscription {}", subscriptionId);
            m_SubscriptionTypes.erase(typeIt);
            return false;
        }
        
        // Remove the subscription from the storage
        bool removed = storageIt->second->RemoveSubscription(subscriptionId);
        
        if (removed)
        {
            // Remove from tracking map
            m_SubscriptionTypes.erase(typeIt);
            // Unsubscription completed silently
        }
        else
        {
            VX_CORE_WARN("Failed to remove subscription with ID: {}", subscriptionId);
        }
        
        return removed;
    }

    size_t EventDispatcher::ProcessQueuedEvents(size_t maxEvents)
    {
        // Move events from queue to processing list to minimize lock time
        std::vector<std::unique_ptr<Event>> eventsToProcess;
        
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            
            if (m_EventQueue.empty())
            {
                return 0;
            }
            
            size_t eventCount = maxEvents == 0 ? m_EventQueue.size() : std::min(maxEvents, m_EventQueue.size());
            
            eventsToProcess.reserve(eventCount);
            
            auto endIt = m_EventQueue.begin() + eventCount;
            std::move(m_EventQueue.begin(), endIt, std::back_inserter(eventsToProcess));
            m_EventQueue.erase(m_EventQueue.begin(), endIt);
        }
        
        // Process events outside of the lock
        size_t processedCount = 0;
        
        for (auto& eventPtr : eventsToProcess)
        {
            if (eventPtr)
            {
                // We need to dispatch based on the actual event type
                // This requires dynamic dispatch or virtual methods
                // For now, we'll implement a basic dispatch mechanism
                
                // The challenge here is that we have a base Event* but need to dispatch
                // to type-specific handlers. This requires either:
                // 1. Virtual dispatch in the Event base class
                // 2. A type registry/visitor pattern
                // 3. Dynamic casting to known event types
                
                // Let's add a virtual Dispatch method to Event for this purpose
                // For now, we'll log that we received the event
                
                // Process queued event silently
                
                // TODO: Implement proper dynamic dispatch
                // eventPtr->DispatchTo(*this);
                
                processedCount++;
            }
        }
        
        // Processed queued events silently
        return processedCount;
    }

    size_t EventDispatcher::ClearEventQueue()
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        
        size_t clearedCount = m_EventQueue.size();
        m_EventQueue.clear();
        
        // Cleared queued events silently
        return clearedCount;
    }

    void EventDispatcher::Clear()
    {
        {
            std::lock_guard<std::shared_mutex> lock(m_Mutex);
            m_Subscribers.clear();
            m_SubscriptionTypes.clear();
        }
        
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_EventQueue.clear();
        }
        
        m_NextSubscriptionID = 0;
        
        // EventDispatcher cleared silently
    }
}