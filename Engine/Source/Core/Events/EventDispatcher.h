#pragma once

#include "Event.h"
#include "Core/Debug/Log.h"
#include <shared_mutex>
#include <typeindex>

namespace Vortex
{
    /**
     * @brief Unique identifier for event subscriptions
     */
    using SubscriptionID = uint64_t;

    /**
     * @brief High-performance event dispatcher with type-safe subscriptions
     * 
     * This dispatcher uses type erasure and template specialization for maximum performance.
     * Features:
     * - Type-safe event subscription/dispatch
     * - Thread-safe operations with minimal locking
     * - Immediate and queued event dispatch modes
     * - Automatic subscription cleanup
     * - High-performance with minimal allocations
     */
    class EventDispatcher
    {
    public:
        EventDispatcher();
        ~EventDispatcher();

        // Non-copyable, but movable
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;
        EventDispatcher(EventDispatcher&&) = default;
        EventDispatcher& operator=(EventDispatcher&&) = default;

        /**
         * @brief Subscribe to events of a specific type
         * @tparam T Event type to subscribe to
         * @param handler Function/lambda to call when event is dispatched
         * @return SubscriptionID for later unsubscription
         */
        template<typename T>
        SubscriptionID Subscribe(EventHandler<T> handler)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must be derived from Event");
            
            std::lock_guard<std::shared_mutex> lock(m_Mutex);
            
            SubscriptionID id = GenerateSubscriptionID();
            auto& subscribers = GetSubscribers<T>();
            
            subscribers.emplace_back(id, std::move(handler));
            
            // Track subscription type for unsubscription by ID
            m_SubscriptionTypes.emplace(id, std::type_index(typeid(T)));
            
            // Subscription added silently
            return id;
        }

        /**
         * @brief Subscribe to events with a member function
         * @tparam T Event type to subscribe to
         * @tparam ClassType Class containing the member function
         * @param instance Pointer to instance of the class
         * @param memberFunc Member function pointer
         * @return SubscriptionID for later unsubscription
         */
        template<typename T, typename ClassType>
        SubscriptionID Subscribe(ClassType* instance, bool (ClassType::*memberFunc)(const T&))
        {
            static_assert(std::is_base_of_v<Event, T>, "T must be derived from Event");
            
            auto handler = [instance, memberFunc](const T& event) -> bool {
                return (instance->*memberFunc)(event);
            };
            
            return Subscribe<T>(handler);
        }

        /**
         * @brief Unsubscribe from events using subscription ID
         * @param subscriptionId ID returned from Subscribe()
         * @return True if subscription was found and removed, false otherwise
         */
        bool Unsubscribe(SubscriptionID subscriptionId);

        /**
         * @brief Unsubscribe all handlers for a specific event type
         * @tparam T Event type to unsubscribe from
         * @return Number of subscriptions removed
         */
        template<typename T>
        size_t UnsubscribeAll()
        {
            static_assert(std::is_base_of_v<Event, T>, "T must be derived from Event");
            
            std::lock_guard<std::shared_mutex> lock(m_Mutex);
            
            auto& subscribers = GetSubscribers<T>();
            size_t count = subscribers.size();
            subscribers.clear();
            
            // Unsubscribed all handlers silently
            return count;
        }

        /**
         * @brief Dispatch an event immediately (synchronous)
         * @tparam T Event type (auto-deduced)
         * @param event Event to dispatch
         * @return True if event was handled by any subscriber, false otherwise
         */
        template<typename T>
        bool DispatchImmediate(const T& event)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must be derived from Event");
            
            std::shared_lock<std::shared_mutex> lock(m_Mutex);
            
            auto& subscribers = GetSubscribers<T>();
            bool handled = false;
            
            for (auto& [id, handler] : subscribers)
            {
                try
                {
                    if (handler(event))
                    {
                        handled = true;
                    }
                }
                catch (const std::exception& e)
                {
                    VX_CORE_ERROR("Exception in event handler {}: {}", id, e.what());
                }
            }
            
            return handled;
        }

        /**
         * @brief Queue an event for later dispatch (asynchronous)
         * @tparam T Event type (auto-deduced)
         * @param event Event to queue (will be copied/moved)
         */
        template<typename T>
        void QueueEvent(T&& event)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must be derived from Event");
            
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_EventQueue.emplace_back(std::make_unique<T>(std::forward<T>(event)));
        }

        /**
         * @brief Process all queued events
         * @param maxEvents Maximum number of events to process (0 = all)
         * @return Number of events processed
         */
        size_t ProcessQueuedEvents(size_t maxEvents = 0);

        /**
         * @brief Clear all queued events without processing them
         * @return Number of events cleared
         */
        size_t ClearEventQueue();

        /**
         * @brief Get the number of subscribers for a specific event type
         * @tparam T Event type
         * @return Number of active subscribers
         */
        template<typename T>
        size_t GetSubscriberCount() const
        {
            static_assert(std::is_base_of_v<Event, T>, "T must be derived from Event");
            
            std::shared_lock<std::shared_mutex> lock(m_Mutex);
            
            const auto& subscribers = GetSubscribers<T>();
            return subscribers.size();
        }

        /**
         * @brief Get the total number of queued events
         * @return Number of events in queue
         */
        size_t GetQueuedEventCount() const
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            return m_EventQueue.size();
        }

        /**
         * @brief Clear all subscriptions and queued events
         */
        void Clear();

    private:
        /**
         * @brief Base class for type-erased subscriber storage
         */
        struct SubscriberStorageBase
        {
            virtual ~SubscriberStorageBase() = default;
            virtual bool RemoveSubscription(SubscriptionID id) = 0;
        };

        /**
         * @brief Type-erased subscription holder
         */
        struct SubscriptionBase
        {
            SubscriptionID id;
            virtual ~SubscriptionBase() = default;
            virtual bool CanUnsubscribe(SubscriptionID targetId) const = 0;
        };

        /**
         * @brief Typed subscription holder
         */
        template<typename T>
        struct TypedSubscription : public SubscriptionBase
        {
            EventHandler<T> handler;
            
            TypedSubscription(SubscriptionID subscriptionId, EventHandler<T> eventHandler)
                : handler(std::move(eventHandler))
            {
                id = subscriptionId;
            }
            
            bool CanUnsubscribe(SubscriptionID targetId) const override
            {
                return id == targetId;
            }
        };

        /**
         * @brief Container for subscribers of a specific event type
         */
        template<typename T>
        using SubscriberList = std::vector<std::pair<SubscriptionID, EventHandler<T>>>;

        /**
         * @brief Typed storage wrapper that inherits from base for type erasure
         */
        template<typename T>
        struct SubscriberStorage : public SubscriberStorageBase
        {
            SubscriberList<T> subscribers;
            
            SubscriberStorage() = default;
            virtual ~SubscriberStorage() = default;
            
            bool RemoveSubscription(SubscriptionID id) override
            {
                auto it = std::find_if(subscribers.begin(), subscribers.end(),
                    [id](const auto& pair) { return pair.first == id; });
                
                if (it != subscribers.end())
                {
                    subscribers.erase(it);
                    return true;
                }
                return false;
            }
        };

        /**
         * @brief Get or create the subscriber list for event type T
         */
        template<typename T>
        SubscriberList<T>& GetSubscribers()
        {
            // Use type_index as key for type-safe storage
            std::type_index typeIndex(typeid(T));
            
            auto it = m_Subscribers.find(typeIndex);
            if (it == m_Subscribers.end())
            {
                // Create new subscriber storage for this type
                auto storage = std::make_unique<SubscriberStorage<T>>();
                auto* ptr = &storage->subscribers;
                m_Subscribers.emplace(typeIndex, std::move(storage));
                return *ptr;
            }
            
            // Cast back to the correct type
            auto* storage = static_cast<SubscriberStorage<T>*>(it->second.get());
            return storage->subscribers;
        }

        /**
         * @brief Get the subscriber list for event type T (const version)
         */
        template<typename T>
        const SubscriberList<T>& GetSubscribers() const
        {
            std::type_index typeIndex(typeid(T));
            
            auto it = m_Subscribers.find(typeIndex);
            if (it == m_Subscribers.end())
            {
                static const SubscriberList<T> empty;
                return empty;
            }
            
            auto* storage = static_cast<const SubscriberStorage<T>*>(it->second.get());
            return storage->subscribers;
        }

        /**
         * @brief Generate a unique subscription ID
         */
        SubscriptionID GenerateSubscriptionID()
        {
            return ++m_NextSubscriptionID;
        }

        // Thread synchronization
        mutable std::shared_mutex m_Mutex;
        mutable std::mutex m_QueueMutex;
        
        // Subscriber storage (type-erased)
        std::unordered_map<std::type_index, std::unique_ptr<SubscriberStorageBase>> m_Subscribers;
        
        // Subscription tracking for unsubscription by ID
        std::unordered_map<SubscriptionID, std::type_index> m_SubscriptionTypes;
        
        // Event queue for asynchronous dispatch
        std::vector<std::unique_ptr<Event>> m_EventQueue;
        
        // Subscription ID generation
        std::atomic<SubscriptionID> m_NextSubscriptionID{0};
    };
}