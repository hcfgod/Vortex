#include "ExampleGameLayer.h"
#include "EditorLayer.h"
#include <imgui.h>
#include "Engine/Renderer/Renderer2D.h"

// Triangle vertices with position, texture coordinates, normal, and tangent
const float kVertices[] =
{
    // Position (x, y, z)    // TexCoord (u, v)  // Normal (x, y, z)     // Tangent (x, y, z)
    -0.8f, -0.8f, 0.0f,     0.0f, 0.0f,         0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, // Bottom-left
     0.8f, -0.8f, 0.0f,     1.0f, 0.0f,         0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, // Bottom-right
     0.0f,  0.8f, 0.0f,     0.5f, 1.0f,         0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f  // Top-center
};

const unsigned int kIndices[] =
{
    0, 1, 2 // Single triangle
};

void ExampleGameLayer::OnAttach()
{
    VX_INFO("ExampleGameLayer attached - Game layer ready!");
    VX_INFO("ExampleGameLayer attached - Game layer ready!");
    VX_INFO("Controls:");
    VX_INFO("  - SPACE/Square: Pause/Unpause (Action System)");
    VX_INFO("  - R/Triangle: Reset game (Action System)");
    VX_INFO("  - Left Click/X: Increase score (Action System)");
    VX_INFO("  - WASD/Left Stick: Movement (Polling)");
    VX_INFO("  - Mouse/Right Stick: Look around (Polling)");
    VX_INFO("  - PS5 Controller: Full support for buttons and sticks");
    
	m_AssetSystem = SysShared<AssetSystem>();
    if(!m_AssetSystem)
    {
        VX_ERROR("AssetSystem not available!");
        return;
	}
    m_AssetSystem->SetWorkingDirectory(std::filesystem::current_path());

    // Initialize Renderer2D after assets root is set so it can find its shaders
    Vortex::Renderer2D::Initialize();
    
    SetupInputActions();
    
    // Initialize the modern shader system
    //SetupShaderSystem();

    // Load a texture asset using generic loader (name-based)
    m_TextureHandle = m_AssetSystem->LoadAsset<TextureAsset>("Checker");

    // === Gameplay Camera Setup ===
    SetupGameplayCamera();
}

void ExampleGameLayer::OnDetach()
{
    VX_INFO("ExampleGameLayer detached - Final Score: {}", m_Score);
    Vortex::Renderer2D::Shutdown();
}

void ExampleGameLayer::OnUpdate()
{
    // WASD + Gamepad Left Stick movement (polling)
    static float playerX = 0.0f, playerY = 0.0f;
    const float moveSpeed = 100.0f; // units per second
    float deltaTime = Time::GetDeltaTime();
    
    // Keyboard input
    if (Input::GetKey(KeyCode::W)) playerY += moveSpeed * deltaTime;
    if (Input::GetKey(KeyCode::S)) playerY -= moveSpeed * deltaTime;
    if (Input::GetKey(KeyCode::A)) playerX -= moveSpeed * deltaTime;
    if (Input::GetKey(KeyCode::D)) playerX += moveSpeed * deltaTime;
    
    // Gamepad input (use first connected gamepad)
    int gamepadIndex = Input::GetFirstConnectedGamepadIndex();
    if (gamepadIndex >= 0)
    {
        // SDL Gamepad Axes: 0=LeftX, 1=LeftY, 2=RightX, 3=RightY
        float leftStickX = Input::GetGamepadAxis(gamepadIndex, 0);  // Left stick horizontal
        float leftStickY = Input::GetGamepadAxis(gamepadIndex, 1);  // Left stick vertical
        
        // Apply deadzone (PS5 typically needs ~0.1f deadzone)
        const float deadzone = 0.15f;
        if (std::abs(leftStickX) > deadzone)
            playerX += leftStickX * moveSpeed * deltaTime;
        if (std::abs(leftStickY) > deadzone)
            playerY -= leftStickY * moveSpeed * deltaTime; // Invert Y for typical game movement
    }
    
    // Mouse + Gamepad Right Stick look (polling)
    static float mouseX = 0.0f, mouseY = 0.0f;
    static float lookX = 0.0f, lookY = 0.0f;
    Input::GetMousePosition(mouseX, mouseY);
    
    float mouseDX, mouseDY;
    Input::GetMouseDelta(mouseDX, mouseDY);
    
    // Gamepad right stick for look
    if (gamepadIndex >= 0)
    {
        float rightStickX = Input::GetGamepadAxis(gamepadIndex, 2);  // Right stick horizontal
        float rightStickY = Input::GetGamepadAxis(gamepadIndex, 3);  // Right stick vertical
        
        const float deadzone = 0.15f;
        const float lookSensitivity = 90.0f; // degrees per second at full stick
        
        if (std::abs(rightStickX) > deadzone)
            lookX += rightStickX * lookSensitivity * deltaTime;
        if (std::abs(rightStickY) > deadzone)
            lookY += rightStickY * lookSensitivity * deltaTime;
        
        // Clamp look values
        while (lookX > 360.0f) lookX -= 360.0f;
        while (lookX < 0.0f) lookX += 360.0f;
        lookY = std::clamp(lookY, -90.0f, 90.0f);
    }
    
    // Mouse scroll (polling)
    float scrollX, scrollY;
    Input::GetMouseScroll(scrollX, scrollY);
    if (scrollY != 0.0f)
    {
        VX_INFO("[Input] Mouse scroll: {:.2f}", scrollY);
    }
    
    // === Gamepad Status and Additional Controls ===
    static bool lastGamepadConnected = false;
    bool gamepadConnected = (gamepadIndex >= 0);
    
    // Log gamepad connection changes
    if (gamepadConnected != lastGamepadConnected)
    {
        if (gamepadConnected)
            VX_INFO("[Input] 🎮 PS5 Controller connected! Ready for testing.");
        else
            VX_INFO("[Input] 🎮 PS5 Controller disconnected.");
        lastGamepadConnected = gamepadConnected;
    }
    
    // Test additional gamepad inputs (triggers, D-pad)
    if (gamepadConnected)
    {
        // Test triggers (SDL axes 4=LeftTrigger, 5=RightTrigger)
        float leftTrigger = Input::GetGamepadAxis(gamepadIndex, 4);
        float rightTrigger = Input::GetGamepadAxis(gamepadIndex, 5);
        
        if (leftTrigger > 0.1f || rightTrigger > 0.1f)
        {
            VX_INFO("[Input] 🎮 Triggers: L2={:.2f}, R2={:.2f}", leftTrigger, rightTrigger);
        }
        
        // Test D-pad buttons
        if (Input::GetGamepadButtonDown(gamepadIndex, 11)) VX_INFO("[Input] 🎮 D-Pad UP pressed");
        if (Input::GetGamepadButtonDown(gamepadIndex, 12)) VX_INFO("[Input] 🎮 D-Pad DOWN pressed");
        if (Input::GetGamepadButtonDown(gamepadIndex, 13)) VX_INFO("[Input] 🎮 D-Pad LEFT pressed");
        if (Input::GetGamepadButtonDown(gamepadIndex, 14)) VX_INFO("[Input] 🎮 D-Pad RIGHT pressed");
        
        // Test shoulder buttons
        if (Input::GetGamepadButtonDown(gamepadIndex, 9)) VX_INFO("[Input] 🎮 L1 pressed");
        if (Input::GetGamepadButtonDown(gamepadIndex, 10)) VX_INFO("[Input] 🎮 R1 pressed");
        
        // Test menu buttons
        if (Input::GetGamepadButtonDown(gamepadIndex, 4)) VX_INFO("[Input] 🎮 Share button pressed");
        if (Input::GetGamepadButtonDown(gamepadIndex, 6)) VX_INFO("[Input] 🎮 Options button pressed");
        if (Input::GetGamepadButtonDown(gamepadIndex, 5)) VX_INFO("[Input] 🎮 PS button pressed");
        
        // Test stick clicks
        if (Input::GetGamepadButtonDown(gamepadIndex, 7)) VX_INFO("[Input] 🎮 Left stick clicked (L3)");
        if (Input::GetGamepadButtonDown(gamepadIndex, 8)) VX_INFO("[Input] 🎮 Right stick clicked (R3)");
    }
    
    // Log movement periodically
    static float lastMoveLogTime = 0.0f;
    if (Time::GetTime() - lastMoveLogTime >= 3.0f)
    {
        if (gamepadConnected)
        {
            float lx = Input::GetGamepadAxis(gamepadIndex, 0);
            float ly = Input::GetGamepadAxis(gamepadIndex, 1);
            float rx = Input::GetGamepadAxis(gamepadIndex, 2);
            float ry = Input::GetGamepadAxis(gamepadIndex, 3);
            
            VX_INFO("[Input] Player: ({:.1f},{:.1f}) | L-Stick: ({:.2f},{:.2f}) | R-Stick: ({:.2f},{:.2f})", 
                   playerX, playerY, lx, ly, rx, ry);
        }
        else
        {
            VX_INFO("[Input] Player pos: ({:.1f}, {:.1f}), Mouse: ({:.0f}, {:.0f})", 
                   playerX, playerY, mouseX, mouseY);
        }
        lastMoveLogTime = Time::GetTime();
    }
    
    // === Camera Controls Demo (Only in Play Mode) === // TODO: when we add scripting suppport (E.G. NativeScripting, etc) we wont have to check for play mode manually
    auto* app = Application::Get();
    bool isInPlayMode = app && app->GetEngine() && 
                       (app->GetEngine()->GetRunMode() == Engine::RunMode::PlayInEditor || 
                        app->GetEngine()->GetRunMode() == Engine::RunMode::Production);

    // In play mode, the application auto-enables relative mouse on right mouse down over the viewport.
    // No per-layer code required.
    if (m_MainCamera && !m_IsPaused && isInPlayMode)
    {
        // Simple camera orbit around the origin
        static float orbitAngle = 0.0f;
        orbitAngle += 30.0f * deltaTime; // 30 degrees per second
        
        if (orbitAngle > 360.0f)
            orbitAngle -= 360.0f;
            
        // Orbit camera around the triangle
        float radius = 5.0f;
        glm::vec3 orbitPos(
            std::cos(glm::radians(orbitAngle)) * radius,
            0.0f,
            std::sin(glm::radians(orbitAngle)) * radius
        );
        
        m_MainCamera->SetPosition(orbitPos);
        
        // Look at the center
        m_MainCamera->LookAt(glm::vec3(0.0f, 0.0f, 0.0f));
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
    // === Camera System Integration ===
    auto* cameraSystem = Sys<CameraSystem>();
    std::shared_ptr<Camera> activeCamera = cameraSystem ? cameraSystem->GetActiveCamera() : nullptr;
    
	// Default matrices if no active camera
    glm::mat4 viewProjection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat3 normalMatrix = glm::mat3(1.0f);
    glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);

    if (activeCamera)
    {
        // Use camera matrices from the camera system
        viewProjection = activeCamera->GetViewProjectionMatrix();
        view = activeCamera->GetViewMatrix();
        
        // Get camera position from perspective camera
        if (auto* perspCam = dynamic_cast<PerspectiveCamera*>(activeCamera.get()))
        {
            cameraPos = perspCam->GetPosition();
        }
    }
    else
    {
        VX_CORE_WARN("[CameraSystem] No active camera found, using identity matrices");
    }

    Renderer2D::BeginScene(*activeCamera);

	Renderer2D::DrawQuad(glm::vec2(-5.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	Renderer2D::DrawQuad(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	Renderer2D::DrawQuad(glm::vec2(5.0f, 0.0f), glm::vec2(1.0f, 1.0f), m_TextureHandle, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    Renderer2D::EndScene();

    // Unbind shader through ShaderManager
    GetShaderManager().UnbindShader();
}

// This is a way of handling events if needed, but there are better ways via Input Actions and the event system, so we leave it empty here.
bool ExampleGameLayer::OnEvent(Event& event)
{
    return false;
}

void ExampleGameLayer::OnImGuiRender()
{
    if (ImGui::Begin("Example Panel"))
    {
        ImGui::Text("Hello from ExampleGameLayer!");
        ImGui::Separator();
        ImGui::Text("Time: %.2fs", m_GameTime);
        ImGui::Text("Score: %d", m_Score);
        ImGui::Checkbox("Paused", &m_IsPaused);
        if (ImGui::Button("Reset"))
        {
            OnResetAction(InputActionPhase::Performed);
        }
        
        ImGui::Separator();
        ImGui::Text("=== Camera System Demo ===");
        
        auto* cameraSystem = Sys<CameraSystem>();
        if (cameraSystem)
        {
            ImGui::Text("CameraSystem: Available");
            ImGui::Text("Registered Cameras: %zu", cameraSystem->GetCameras().size());
            
            auto activeCamera = cameraSystem->GetActiveCamera();
            if (activeCamera)
            {
                ImGui::Text("Active Camera: Available");
                
                // Check camera type and display appropriate info
                if (auto* perspCam = dynamic_cast<PerspectiveCamera*>(activeCamera.get()))
                {
                    ImGui::Text("Type: Perspective Camera (Gameplay)");
                    glm::vec3 pos = perspCam->GetPosition();
                    ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
                    
                    glm::vec3 rot = perspCam->GetRotation();
                    ImGui::Text("Rotation: (%.1f, %.1f, %.1f)", rot.x, rot.y, rot.z);
                }
                else if (auto* editorCam = dynamic_cast<EditorCamera*>(activeCamera.get()))
                {
                    ImGui::Text("Type: Editor Camera (Editor Mode)");
                    glm::vec3 pos = editorCam->GetPosition();
                    ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
                    
                    ImGui::Text("Projection: %s", 
                               editorCam->GetProjectionType() == EditorCamera::ProjectionType::Perspective ? 
                               "Perspective" : "Orthographic");
                    
                    if (editorCam->GetProjectionType() == EditorCamera::ProjectionType::Orthographic)
                    {
                        ImGui::Text("Ortho Size: %.1f", editorCam->GetOrthoSize());
                    }
                }
            }
            else
            {
                ImGui::Text("Active Camera: None");
            }
            
            // Show engine mode
            auto* app = Application::Get();
            if (app && app->GetEngine())
            {
                auto mode = app->GetEngine()->GetRunMode();
                const char* modeStr = (mode == Engine::RunMode::Edit) ? "Edit" : 
                                     (mode == Engine::RunMode::PlayInEditor) ? "Play In Editor" : "Production";
                ImGui::Text("Engine Mode: %s", modeStr);
            }
        }
        else
        {
            ImGui::Text("CameraSystem: Not Available");
        }
    }
    ImGui::End();
}

void ExampleGameLayer::SetupShaderSystem()
{
    VX_INFO("[ShaderSystem] Setting up modern shader system via AssetSystem (async)...");

    try
    {
        // Request async load with progress callback for logging
        m_ShaderHandle = m_AssetSystem->LoadAsset<ShaderAsset>(
            "AdvancedTriangle",
            [this](float progress)
            {
                VX_CORE_INFO("[AssetSystem] Shader loading progress: {:.1f}%", progress * 100.0f);
                std::string title = "Vortex Sandbox - Loading Shaders: " + std::to_string(static_cast<int>(progress * 100)) + "%";
                if (progress >= 1.0f)
                {
                    title = "Vortex Sandbox - Shaders Loaded!";
                    VX_CORE_INFO("[AssetSystem] Shader loading completed!");
                }
            }
        );
    }
    catch (const std::exception& e)
    {
        VX_ERROR("[ShaderSystem] Exception during shader setup: {}", e.what());
    }
}

void ExampleGameLayer::SetupInputActions()
{
    m_InputSystem = SysShared<InputSystem>();
    if (!m_InputSystem)
    {
        VX_WARN("InputSystem not available for ExampleGameLayer");
        return;
    }
    
    // Create a gameplay action map
    m_GameplayActions = m_InputSystem->CreateActionMap("ExampleGameplay");

    // === Pause Action ===
    auto* pauseAction = m_GameplayActions->CreateAction("Pause", InputActionType::Button);
    pauseAction->AddBinding(InputBinding::KeyboardKey(KeyCode::Space, "Keyboard/Space"));
    pauseAction->AddBinding(InputBinding::GamepadButton(-1, 0, "Gamepad/Square")); // PS5 Square button (SDL button 0, any gamepad)
    pauseAction->SetCallbacks(
        nullptr, // Started
        [this](InputActionPhase phase) { OnPauseAction(phase); }, // Performed
        nullptr  // Canceled
    );
    
    // === Reset Action ===
    auto* resetAction = m_GameplayActions->CreateAction("Reset", InputActionType::Button);
    resetAction->AddBinding(InputBinding::KeyboardKey(KeyCode::R, "Keyboard/R"));
    resetAction->AddBinding(InputBinding::GamepadButton(-1, 3, "Gamepad/Triangle")); // PS5 Triangle button (SDL button 3, any gamepad)
    resetAction->SetCallbacks(
        nullptr, // Started
        [this](InputActionPhase phase) { OnResetAction(phase); }, // Performed
        nullptr  // Canceled
    );
    
    // === Fire Action ===
    auto* fireAction = m_GameplayActions->CreateAction("Fire", InputActionType::Button);
    fireAction->AddBinding(InputBinding::MouseButton(MouseCode::ButtonLeft, "Mouse/LeftButton"));
    fireAction->AddBinding(InputBinding::GamepadButton(-1, 1, "Gamepad/X")); // PS5 X button (SDL button 1, any gamepad)
    fireAction->SetCallbacks(
        nullptr, // Started
        [this](InputActionPhase phase) { OnFireAction(phase); }, // Performed
        nullptr  // Canceled
    );

    // === Build Assets Action ===
    auto* buildAssetsAction = m_GameplayActions->CreateAction("BuildAssets", InputActionType::Button);
    buildAssetsAction->AddBinding(InputBinding::KeyboardKey(KeyCode::B, "Keyboard/B"));
    buildAssetsAction->AddBinding(InputBinding::GamepadButton(-1, 6, "Gamepad/Options")); // PS5 Options
    buildAssetsAction->SetCallbacks(
        nullptr,
        [this](InputActionPhase phase) { OnBuildAssetsAction(phase); },
        nullptr
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

void ExampleGameLayer::OnBuildAssetsAction(InputActionPhase phase)
{
    if (!m_AssetSystem)
    {
        VX_WARN("[Action] AssetSystem not available");
        return;
    }

    VX_INFO("[Action] Building asset pack via AssetSystem...");
    AssetSystem::BuildAssetsOptions opts;
    opts.PrecompileShaders = true;
    opts.IncludeShaderSources = false;
    auto res = m_AssetSystem->BuildAssetsPack(opts);
    if (res.IsError())
    {
        VX_ERROR("[Action] BuildAssetsPack failed: {}", res.GetErrorMessage());
    }
    else
    {
        VX_INFO("[Action] Asset pack written to {}", res.GetValue().string());
    }
}

void ExampleGameLayer::SetupGameplayCamera()
{
    VX_INFO("[CameraSystem] Setting up gameplay camera...");

    // Get the CameraSystem through the system manager
    auto* cameraSystem = Sys<CameraSystem>();
    if (!cameraSystem)
    {
        VX_ERROR("[CameraSystem] CameraSystem not available!");
        return;
    }

    // Create a perspective camera for 3D rendering
    m_MainCamera = std::make_shared<PerspectiveCamera>(
        45.0f,    // FOV
        16.0f/9.0f, // Aspect ratio
        0.1f,     // Near clip
        1000.0f   // Far clip
    );
    
    // Set initial camera position
    m_MainCamera->SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    m_MainCamera->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));

    // Register the camera with the system
    cameraSystem->Register(m_MainCamera);
    
    // Register as the main gameplay camera
    cameraSystem->SetMainCamera(m_MainCamera);

    // Set it as the active camera for gameplay
    cameraSystem->SetActiveCamera(m_MainCamera);

    VX_INFO("[CameraSystem] Gameplay camera created and registered successfully");
    VX_INFO("[CameraSystem] Camera position: ({:.1f}, {:.1f}, {:.1f})", 
           m_MainCamera->GetPosition().x, m_MainCamera->GetPosition().y, m_MainCamera->GetPosition().z);
}