#pragma once

#include "Event.h"

namespace Vortex
{
    /**
     * @brief Window resize event
     */
    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(uint32_t width, uint32_t height)
            : m_Width(width), m_Height(height) {}

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }

        std::string ToString() const override
        {
            return fmt::format("WindowResize: {}x{}", m_Width, m_Height);
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategory::Window)

    private:
        uint32_t m_Width, m_Height;
    };

    /**
     * @brief Window close event
     */
    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() = default;

        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EventCategory::Window)
    };

    /**
     * @brief Window focus event
     */
    class WindowFocusEvent : public Event
    {
    public:
        WindowFocusEvent() = default;

        EVENT_CLASS_TYPE(WindowFocus)
        EVENT_CLASS_CATEGORY(EventCategory::Window)
    };

    /**
     * @brief Window lost focus event
     */
    class WindowLostFocusEvent : public Event
    {
    public:
        WindowLostFocusEvent() = default;

        EVENT_CLASS_TYPE(WindowLostFocus)
        EVENT_CLASS_CATEGORY(EventCategory::Window)
    };

    /**
     * @brief Window moved event
     */
    class WindowMovedEvent : public Event
    {
    public:
        WindowMovedEvent(int x, int y)
            : m_X(x), m_Y(y) {}

        int GetX() const { return m_X; }
        int GetY() const { return m_Y; }

        std::string ToString() const override
        {
            return fmt::format("WindowMoved: {}, {}", m_X, m_Y);
        }

        EVENT_CLASS_TYPE(WindowMoved)
        EVENT_CLASS_CATEGORY(EventCategory::Window)

    private:
        int m_X, m_Y;
    };

    /**
     * @brief Window minimized event
     */
    class WindowMinimizedEvent : public Event
    {
    public:
        WindowMinimizedEvent() = default;

        EVENT_CLASS_TYPE(WindowMinimized)
        EVENT_CLASS_CATEGORY(EventCategory::Window)
    };

    /**
     * @brief Window maximized event
     */
    class WindowMaximizedEvent : public Event
    {
    public:
        WindowMaximizedEvent() = default;

        EVENT_CLASS_TYPE(WindowMaximized)
        EVENT_CLASS_CATEGORY(EventCategory::Window)
    };

    /**
     * @brief Window restored event
     */
    class WindowRestoredEvent : public Event
    {
    public:
        WindowRestoredEvent() = default;

        EVENT_CLASS_TYPE(WindowRestored)
        EVENT_CLASS_CATEGORY(EventCategory::Window)
    };
}
