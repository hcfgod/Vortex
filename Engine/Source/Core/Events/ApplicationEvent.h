#pragma once

#include "Event.h"

namespace Vortex
{
    /**
     * @brief Application started event
     */
    class ApplicationStartedEvent : public Event
    {
    public:
        ApplicationStartedEvent() = default;

        EVENT_CLASS_TYPE(ApplicationStarted)
        EVENT_CLASS_CATEGORY(EventCategory::Application)
    };

    /**
     * @brief Application shutdown event
     */
    class ApplicationShutdownEvent : public Event
    {
    public:
        ApplicationShutdownEvent() = default;

        EVENT_CLASS_TYPE(ApplicationShutdown)
        EVENT_CLASS_CATEGORY(EventCategory::Application)
    };

    /**
     * @brief Application tick event (fired every frame)
     */
    class ApplicationTickEvent : public Event
    {
    public:
        ApplicationTickEvent(float deltaTime)
            : m_DeltaTime(deltaTime) {}

        float GetDeltaTime() const { return m_DeltaTime; }

        std::string ToString() const override
        {
            return fmt::format("ApplicationTick: DeltaTime={}", m_DeltaTime);
        }

        EVENT_CLASS_TYPE(ApplicationTick)
        EVENT_CLASS_CATEGORY(EventCategory::Application)

    private:
        float m_DeltaTime;
    };

    /**
     * @brief Application update event
     */
    class ApplicationUpdateEvent : public Event
    {
    public:
        ApplicationUpdateEvent(float deltaTime)
            : m_DeltaTime(deltaTime) {}

        float GetDeltaTime() const { return m_DeltaTime; }

        std::string ToString() const override
        {
            return fmt::format("ApplicationUpdate: DeltaTime={}", m_DeltaTime);
        }

        EVENT_CLASS_TYPE(ApplicationUpdate)
        EVENT_CLASS_CATEGORY(EventCategory::Application)

    private:
        float m_DeltaTime;
    };

    /**
     * @brief Application render event
     */
    class ApplicationRenderEvent : public Event
    {
    public:
        ApplicationRenderEvent(float deltaTime)
            : m_DeltaTime(deltaTime) {}

        float GetDeltaTime() const { return m_DeltaTime; }

        std::string ToString() const override
        {
            return fmt::format("ApplicationRender: DeltaTime={}", m_DeltaTime);
        }

        EVENT_CLASS_TYPE(ApplicationRender)
        EVENT_CLASS_CATEGORY(EventCategory::Application)

    private:
        float m_DeltaTime;
    };
}
