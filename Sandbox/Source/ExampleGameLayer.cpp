#include "ExampleGameLayer.h"

void ExampleGameLayer::OnAttach()
{
    VX_INFO("ExampleGameLayer attached - Game layer ready!");
    VX_INFO("Controls:");
    VX_INFO("  - SPACE: Pause/Unpause");
    VX_INFO("  - R: Reset game");
    VX_INFO("  - Left Click: Increase score");
}

void ExampleGameLayer::OnDetach()
{
    VX_INFO("ExampleGameLayer detached - Final Score: {}", m_Score);
}

void ExampleGameLayer::OnUpdate()
{
    if (!m_IsPaused)
    {
        // Update game time using the Time system
        float deltaTime = Time::GetDeltaTime();
        m_GameTime += deltaTime;
        
        // Simple animation
        m_AnimationTime += deltaTime;
        m_RotationAngle += 45.0f * deltaTime; // 45 degrees per second
        
        if (m_RotationAngle > 360.0f)
            m_RotationAngle -= 360.0f;
    }
    
    // Log stats every 5 seconds
    static float lastLogTime = 0.0f;
    if (m_GameTime - lastLogTime >= 6.5f)
    {
        VX_INFO("[Game] Time: {:.1f}s, Score: {}, FPS: {:.1f}, Paused: {}", 
               m_GameTime, m_Score, Time::GetFPS(), m_IsPaused ? "Yes" : "No");
        lastLogTime = m_GameTime;
    }
}

void ExampleGameLayer::OnRender()
{
    // This would normally contain actual rendering code
    // For now, we'll just demonstrate that the render cycle is working
    
    // Simulate some "rendering work" with a simple calculation
    static int renderCallCount = 0;
    renderCallCount++;
    
    // Log render stats occasionally
    if (renderCallCount % 600 == 0)
    {
        VX_TRACE("[Game] Render call #{} - Rotation: {:.1f}°", renderCallCount, m_RotationAngle);
    }
}

bool ExampleGameLayer::OnEvent(Event& event)
{
    // Handle events directly using dynamic casting
    // This is simpler than the subscription-based EventDispatcher for immediate handling
    
    if (auto keyEvent = dynamic_cast<KeyPressedEvent*>(&event))
    {
        return OnKeyPressed(*keyEvent);
    }
    
    if (auto mouseEvent = dynamic_cast<MouseButtonPressedEvent*>(&event))
    {
        return OnMouseButtonPressed(*mouseEvent);
    }
    
    return false; // Don't consume events, let other layers handle them too
}

bool ExampleGameLayer::OnKeyPressed(KeyPressedEvent& e)
{
    switch (e.GetKeyCode())
    {
        case KeyCode::Space:
            m_IsPaused = !m_IsPaused;
            VX_INFO("[Game] Game {}", m_IsPaused ? "PAUSED" : "RESUMED");
            return true; // Consume this event
            
        case KeyCode::R:
            // Reset game
            m_GameTime = 0.0f;
            m_Score = 0;
            m_AnimationTime = 0.0f;
            m_RotationAngle = 0.0f;
            m_IsPaused = false;
            VX_INFO("[Game] Game RESET");
            return true; // Consume this event
    }
    
    return false;
}

bool ExampleGameLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
    if (e.GetMouseButton() == MouseCode::ButtonLeft)
    {
        if (!m_IsPaused)
        {
            m_Score += 10;
            VX_INFO("[Game] Score increased! New score: {}", m_Score);
        }
        else
        {
            VX_INFO("[Game] Can't score while paused!");
        }
        return true; // Consume this event
    }
    
    return false;
}
