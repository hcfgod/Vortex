#pragma once

#include "Event.h"

namespace Vortex
{
    /**
     * @brief Engine update event - fired every frame during engine update
     */
    class EngineUpdateEvent : public Event
    {
    public:
        EngineUpdateEvent(float deltaTime)
            : m_DeltaTime(deltaTime) {}

        float GetDeltaTime() const { return m_DeltaTime; }

        std::string ToString() const override
        {
            return fmt::format("EngineUpdate: DeltaTime={}", m_DeltaTime);
        }

        EVENT_CLASS_TYPE(EngineUpdate)
        EVENT_CLASS_CATEGORY(EventCategory::Application)

    private:
        float m_DeltaTime;
    };

    /**
     * @brief Engine render event - fired every frame during engine render
     */
    class EngineRenderEvent : public Event
    {
    public:
        EngineRenderEvent(float deltaTime)
            : m_DeltaTime(deltaTime) {}

        float GetDeltaTime() const { return m_DeltaTime; }

        std::string ToString() const override
        {
            return fmt::format("EngineRender: DeltaTime={}", m_DeltaTime);
        }

        EVENT_CLASS_TYPE(EngineRender)
        EVENT_CLASS_CATEGORY(EventCategory::Application)

    private:
        float m_DeltaTime;
    };
}
