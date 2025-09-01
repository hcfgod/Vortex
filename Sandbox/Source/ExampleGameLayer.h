#pragma once

#include <Vortex.h>

using namespace Vortex;

/**
 * @brief Example game layer that demonstrates basic game functionality
 * 
 * This layer shows how to:
 * - Handle game-specific events
 * - Update game logic
 * - Render game content
 * - Work with the Time system
 */
class ExampleGameLayer : public Layer
{
public:
    ExampleGameLayer() 
        : Layer("ExampleGame", LayerType::Game, LayerPriority::Normal) {}

    virtual ~ExampleGameLayer() = default;

    // Layer interface
    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual bool OnEvent(Event& event) override;

private:
    // Event handlers
    bool OnKeyPressed(KeyPressedEvent& e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

private:
    // Game state
    float m_GameTime = 0.0f;
    int m_Score = 0;
    bool m_IsPaused = false;
    
    // Simple animation
    float m_AnimationTime = 0.0f;
    float m_RotationAngle = 0.0f;
};