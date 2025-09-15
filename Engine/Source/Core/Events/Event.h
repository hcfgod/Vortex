#pragma once

#include <string>
#include <functional>

namespace Vortex
{
    /**
     * @brief Event categories for filtering and debugging
     */
    enum class EventCategory : uint32_t
    {
        None           = 0,
        Application    = 1 << 0,  // Application lifecycle events
        Input          = 1 << 1,  // Input events (keyboard, mouse, etc.)
        Keyboard       = 1 << 2,  // Keyboard-specific events
        Mouse          = 1 << 3,  // Mouse-specific events
        MouseButton    = 1 << 4,  // Mouse button events
        Gamepad        = 1 << 5,  // Gamepad/controller events
        Window         = 1 << 6,  // Window events (resize, close, etc.)
        Renderer       = 1 << 7,  // Rendering events
        Audio          = 1 << 8,  // Audio events
        Network        = 1 << 9,  // Network events
        Game           = 1 << 10, // Game-specific events
        Debug          = 1 << 11, // Debug/profiling events
    };

    // Bitwise operators for EventCategory to enable flag combinations
    inline EventCategory operator|(EventCategory lhs, EventCategory rhs)
    {
        return static_cast<EventCategory>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }

    inline EventCategory operator&(EventCategory lhs, EventCategory rhs)
    {
        return static_cast<EventCategory>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
    }

    inline EventCategory operator^(EventCategory lhs, EventCategory rhs)
    {
        return static_cast<EventCategory>(static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs));
    }

    inline EventCategory operator~(EventCategory category)
    {
        return static_cast<EventCategory>(~static_cast<uint32_t>(category));
    }

    inline EventCategory& operator|=(EventCategory& lhs, EventCategory rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    inline EventCategory& operator&=(EventCategory& lhs, EventCategory rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    inline EventCategory& operator^=(EventCategory& lhs, EventCategory rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }

    /**
     * @brief Event types for runtime type identification
     */
    enum class EventType : uint32_t
    {
        None = 0,
        
        // Application events
        ApplicationStarted,
        ApplicationShutdown,
        ApplicationTick,
        ApplicationUpdate,
        ApplicationRender,
        
        // Engine events
        EngineUpdate,
        EngineRender,
        
        // Window events
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMoved,
        WindowMinimized,
        WindowMaximized,
        WindowRestored,
        
        // Keyboard events
        KeyPressed,
        KeyReleased,
        KeyTyped,
        
        // Mouse events
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled,
        
        // Gamepad events
        GamepadConnected,
        GamepadDisconnected,
        GamepadButtonPressed,
        GamepadButtonReleased,
        GamepadAxis,
        
        // Audio events
        AudioDeviceAdded,
        AudioDeviceRemoved,
        
        // Custom events start here
        CustomEventStart = 1000
    };

    /**
     * @brief Base event class that all events inherit from
     */
    class Event
    {
    public:
        virtual ~Event() = default;

        /**
         * @brief Get the event type
         * @return EventType enum value
         */
        virtual EventType GetEventType() const = 0;
        
        /**
         * @brief Get the event category flags
         * @return Bitmask of EventCategory flags
         */
        virtual uint32_t GetCategoryFlags() const = 0;
        
        /**
         * @brief Get the event name for debugging
         * @return Event name as string
         */
        virtual const char* GetName() const = 0;
        
        /**
         * @brief Convert event to string for debugging
         * @return String representation of the event
         */
        virtual std::string ToString() const { return GetName(); }
        
        /**
         * @brief Check if this event belongs to a specific category
         * @param category Category to check
         * @return True if event belongs to category, false otherwise
         */
        bool IsInCategory(EventCategory category) const
        {
            return GetCategoryFlags() & static_cast<uint32_t>(category);
        }
        
        /**
         * @brief Check if this event has been handled
         * @return True if handled, false otherwise
         */
        bool IsHandled() const { return m_Handled; }
        
        /**
         * @brief Mark this event as handled
         */
        void SetHandled(bool handled = true) { m_Handled = handled; }

    protected:
        bool m_Handled = false;
    };

    /**
     * @brief Event dispatcher delegate - can hold functions, lambdas, or member functions
     */
    template<typename EventType>
    using EventHandler = std::function<bool(const EventType&)>;

    /**
     * @brief Macro to implement GetEventType() for event classes
     */
    #define EVENT_CLASS_TYPE(type) \
        static EventType GetStaticType() { return EventType::type; } \
        virtual EventType GetEventType() const override { return GetStaticType(); } \
        virtual const char* GetName() const override { return #type; }

    /**
     * @brief Macro to implement GetCategoryFlags() for event classes
     */
    #define EVENT_CLASS_CATEGORY(category) \
        virtual uint32_t GetCategoryFlags() const override { return static_cast<uint32_t>(category); }

    /**
     * @brief Output stream operator for events (for debugging)
     */
    inline std::ostream& operator<<(std::ostream& os, const Event& e)
    {
        return os << e.ToString();
    }
}
