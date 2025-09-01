#pragma once

#include <Vortex.h>

using namespace Vortex;

/**
 * @brief Example game layer that demonstrates the new Input system
 * 
 * This layer shows how to:
 * - Use the Input polling API (Input::GetKey, Input::GetMouseButton, etc.)
 * - Set up InputActions with callbacks
 * - Update game logic
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
    // Input setup
    void SetupInputActions();
    
    // Action callbacks
    void OnPauseAction(InputActionPhase phase);
    void OnResetAction(InputActionPhase phase);
    void OnFireAction(InputActionPhase phase);

private:
    // Game state
    float m_GameTime = 0.0f;
    int m_Score = 0;
    bool m_IsPaused = false;
    
    // Simple animation
    float m_AnimationTime = 0.0f;
    float m_RotationAngle = 0.0f;
    
    // Input action map
    std::shared_ptr<InputActionMap> m_GameplayActions;
};
