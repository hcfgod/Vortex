#include "ExampleGameLayer.h"

void ExampleGameLayer::OnAttach()
{
    VX_INFO("ExampleGameLayer attached - Game layer ready!");
    VX_INFO("Controls:");
    VX_INFO("  - SPACE: Pause/Unpause (Action System)");
    VX_INFO("  - R: Reset game (Action System)");
    VX_INFO("  - Left Click: Increase score (Action System)");
    VX_INFO("  - WASD: Movement (Polling)");
    VX_INFO("  - Mouse: Look around (Polling)");
    
    SetupInputActions();
}

void ExampleGameLayer::OnDetach()
{
    VX_INFO("ExampleGameLayer detached - Final Score: {}", m_Score);
}

void ExampleGameLayer::OnUpdate()
{
    // === Input Polling Demonstration ===
    
    // WASD movement (polling)
    static float playerX = 0.0f, playerY = 0.0f;
    const float moveSpeed = 100.0f; // units per second
    float deltaTime = Time::GetDeltaTime();
    
    if (Input::GetKey(KeyCode::W)) playerY += moveSpeed * deltaTime;
    if (Input::GetKey(KeyCode::S)) playerY -= moveSpeed * deltaTime;
    if (Input::GetKey(KeyCode::A)) playerX -= moveSpeed * deltaTime;
    if (Input::GetKey(KeyCode::D)) playerX += moveSpeed * deltaTime;
    
    // Mouse look (polling)
    static float mouseX = 0.0f, mouseY = 0.0f;
    Input::GetMousePosition(mouseX, mouseY);
    
    float mouseDX, mouseDY;
    Input::GetMouseDelta(mouseDX, mouseDY);
    
    // Mouse scroll (polling)
    float scrollX, scrollY;
    Input::GetMouseScroll(scrollX, scrollY);
    if (scrollY != 0.0f)
    {
        VX_INFO("[Input] Mouse scroll: {:.2f}", scrollY);
    }
    
    // Log movement periodically
    static float lastMoveLogTime = 0.0f;
    if (Time::GetTime() - lastMoveLogTime >= 3.0f)
    {
        VX_INFO("[Input] Player pos: ({:.1f}, {:.1f}), Mouse: ({:.0f}, {:.0f})", 
               playerX, playerY, mouseX, mouseY);
        lastMoveLogTime = Time::GetTime();
    }
    
    // === Game Logic ===
    
    if (!m_IsPaused)
    {
        // Update game time using the Time system
        m_GameTime += deltaTime;
        
        // Simple animation
        m_AnimationTime += deltaTime;
        m_RotationAngle += 45.0f * deltaTime; // 45 degrees per second
        
        if (m_RotationAngle > 360.0f)
            m_RotationAngle -= 360.0f;
    }
    
    // Log stats every 5 seconds
    static float lastLogTime = 0.0f;
    if (m_GameTime - lastLogTime >= 5.0f)
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
    // We no longer handle input events directly here.
    // Input is now handled via the InputSystem (polling + actions)
    return false; // Don't consume any events
}

void ExampleGameLayer::SetupInputActions()
{
    // Get the Application and Engine to access InputSystem
    auto* app = Application::Get();
    if (!app || !app->GetEngine())
    {
        VX_WARN("Application or Engine not available for ExampleGameLayer");
        return;
    }
    
	auto* inputSystem = app->GetEngine()->GetSystemManager().GetSystem<InputSystem>();
    if (!inputSystem)
    {
        VX_WARN("InputSystem not available for ExampleGameLayer");
        return;
    }
    
    // Create a gameplay action map
    m_GameplayActions = inputSystem->CreateActionMap("ExampleGameplay");

    // === Pause Action ===
    auto* pauseAction = m_GameplayActions->CreateAction("Pause", InputActionType::Button);
    pauseAction->AddBinding(InputBinding::KeyboardKey(KeyCode::Space, "Keyboard/Space"));
    pauseAction->SetCallbacks(
        nullptr, // Started
        [this](InputActionPhase phase) { OnPauseAction(phase); }, // Performed
        nullptr  // Canceled
    );
    
    // === Reset Action ===
    auto* resetAction = m_GameplayActions->CreateAction("Reset", InputActionType::Button);
    resetAction->AddBinding(InputBinding::KeyboardKey(KeyCode::R, "Keyboard/R"));
    resetAction->SetCallbacks(
        nullptr, // Started
        [this](InputActionPhase phase) { OnResetAction(phase); }, // Performed
        nullptr  // Canceled
    );
    
    // === Fire Action ===
    auto* fireAction = m_GameplayActions->CreateAction("Fire", InputActionType::Button);
    fireAction->AddBinding(InputBinding::MouseButton(MouseCode::ButtonLeft, "Mouse/LeftButton"));
    fireAction->SetCallbacks(
        nullptr, // Started
        [this](InputActionPhase phase) { OnFireAction(phase); }, // Performed
        nullptr  // Canceled
    );
    
    VX_INFO("[Input] Input actions configured for ExampleGameLayer");
}

void ExampleGameLayer::OnPauseAction(InputActionPhase phase)
{
    m_IsPaused = !m_IsPaused;
    VX_INFO("[Action] Game {} (via Input Action)", m_IsPaused ? "PAUSED" : "RESUMED");
}

void ExampleGameLayer::OnResetAction(InputActionPhase phase)
{
    // Reset game
    m_GameTime = 0.0f;
    m_Score = 0;
    m_AnimationTime = 0.0f;
    m_RotationAngle = 0.0f;
    m_IsPaused = false;
    VX_INFO("[Action] Game RESET (via Input Action)");
}

void ExampleGameLayer::OnFireAction(InputActionPhase phase)
{
    if (!m_IsPaused)
    {
        m_Score += 10;
        VX_INFO("[Action] Score increased! New score: {} (via Input Action)", m_Score);
    }
    else
    {
        VX_INFO("[Action] Can't score while paused! (via Input Action)");
    }
}
