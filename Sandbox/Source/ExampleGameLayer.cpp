#include "ExampleGameLayer.h"
#include <glad/gl.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace 
{
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
}

void ExampleGameLayer::OnAttach()
{
    VX_INFO("ExampleGameLayer attached - Game layer ready!");
    VX_INFO("Controls:");
    VX_INFO("  - SPACE/Square: Pause/Unpause (Action System)");
    VX_INFO("  - R/Triangle: Reset game (Action System)");
    VX_INFO("  - Left Click/X: Increase score (Action System)");
    VX_INFO("  - WASD/Left Stick: Movement (Polling)");
    VX_INFO("  - Mouse/Right Stick: Look around (Polling)");
    VX_INFO("  - PS5 Controller: Full support for buttons and sticks");
    
    SetupInputActions();

    // Initialize the modern shader system
    SetupShaderSystem();

    // Generate buffers and arrays
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    // Bind and set up vertex and element buffers
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices, GL_STATIC_DRAW);

    // Vertex attributes: position (3 floats) + texcoord (2 floats) + normal (3 floats) + tangent (3 floats) = stride of 11 floats
    const int stride = 11 * sizeof(float);
    
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Normal attribute (location = 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Tangent attribute (location = 3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    // Unbind for safety
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ExampleGameLayer::OnDetach()
{
    VX_INFO("ExampleGameLayer detached - Final Score: {}", m_Score);

    // Cleanup shader resources
    m_TriangleShader.reset();
    m_ShaderManager.reset();
    
    // Cleanup GL resources
    if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
    if (m_VBO) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
    if (m_EBO) { glDeleteBuffers(1, &m_EBO); m_EBO = 0; }
}

void ExampleGameLayer::OnUpdate()
{
    // === Input Polling Demonstration ===
    
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
    // Use the modern shader system for rendering
    if (m_TriangleShader && m_TriangleShader->IsValid() && m_VAO != 0)
    {
        // Bind our shader
        m_TriangleShader->Bind();
        
        // Set matrix uniforms for advanced shader
        glm::mat4 viewProjection = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat3 normalMatrix = glm::mat3(1.0f);
        
        // Set uniforms for animation
        float timeValue = m_IsPaused ? 0.0f : m_GameTime;
        
        if (m_UsingAdvancedShader)
        {
            // === Matrix uniforms (required by advanced shader) ===
            m_TriangleShader->SetUniform("u_ViewProjection", viewProjection);
            m_TriangleShader->SetUniform("u_View", view);
            m_TriangleShader->SetUniform("u_Model", model);
            m_TriangleShader->SetUniform("u_NormalMatrix", normalMatrix);
            
            // === Animation and time ===
            m_TriangleShader->SetUniform("u_Time", timeValue);
            
            // === Camera position ===
            glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
            m_TriangleShader->SetUniform("u_CameraPos", cameraPos);
            
            // === Material properties ===
            glm::vec3 albedo(0.7f, 0.3f, 0.1f); // Reddish-orange material
            m_TriangleShader->SetUniform("u_Albedo", albedo);
            m_TriangleShader->SetUniform("u_Metallic", 0.2f);
            m_TriangleShader->SetUniform("u_Roughness", 0.4f);
            m_TriangleShader->SetUniform("u_AO", 1.0f);
            m_TriangleShader->SetUniform("u_Alpha", 1.0f);
            
            // Emission for some glow effect
            float emissionStrength = (sin(timeValue * 2.0f) * 0.5f + 0.5f) * 0.1f;
            glm::vec3 emission(emissionStrength, emissionStrength * 0.5f, 0.0f);
            m_TriangleShader->SetUniform("u_Emission", emission);
            
            // === Lighting ===
            glm::vec3 lightPos(2.0f, 2.0f, 2.0f);
            glm::vec3 lightColor(1.0f, 1.0f, 0.9f); // Warm white
            m_TriangleShader->SetUniform("u_LightPosition", lightPos);
            m_TriangleShader->SetUniform("u_LightColor", lightColor);
            m_TriangleShader->SetUniform("u_LightIntensity", 10.0f);
            
            // === Transform uniforms for vertex animation ===
            glm::vec3 translation(0.0f, 0.0f, 0.0f);
            glm::vec3 rotation(0.0f, timeValue * 0.2f, 0.0f); // Slow Y rotation
            glm::vec3 scale(1.0f, 1.0f, 1.0f);
            m_TriangleShader->SetUniform("u_Translation", translation);
            m_TriangleShader->SetUniform("u_Rotation", rotation);
            m_TriangleShader->SetUniform("u_Scale", scale);
            
            // === Wind animation parameters ===
            float windStrength = (sin(timeValue * 1.5f) * 0.5f + 0.5f) * 0.05f;
            m_TriangleShader->SetUniform("u_WindStrength", windStrength);

            // === Rim lighting effect ===
            m_TriangleShader->SetUniform("u_RimPower", 2.0f);
            glm::vec3 rimColor(0.2f, 0.4f, 1.0f); // Blue rim
            m_TriangleShader->SetUniform("u_RimColor", rimColor);
            m_TriangleShader->SetUniform("u_FresnelStrength", 0.3f);
            
            // === Texture flags (all disabled for now since we don't have textures) ===
            m_TriangleShader->SetUniform("u_UseAlbedoTexture", 0);
            m_TriangleShader->SetUniform("u_UseNormalTexture", 0);
            m_TriangleShader->SetUniform("u_UseMetallicRoughnessTexture", 0);
            m_TriangleShader->SetUniform("u_UseEmissionTexture", 0);
            m_TriangleShader->SetUniform("u_UseAOTexture", 0);
        }
        else
        {
            // Fallback for basic shaders (if we ever implement them)
            m_TriangleShader->SetUniform("u_ViewProjection", viewProjection);
            m_TriangleShader->SetUniform("u_Transform", model);
            m_TriangleShader->SetUniform("u_Time", timeValue);
            glm::vec3 triangleColor(1.0f, 0.5f, 0.2f);
            m_TriangleShader->SetUniform("u_Color", triangleColor);
            m_TriangleShader->SetUniform("u_Alpha", 1.0f);
        }
        
        // Bind vertex array and draw
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        // Unbind shader
        m_TriangleShader->Unbind();
    }

    // Optional periodic log
    static int renderCallCount = 0;
    renderCallCount++;
    if (renderCallCount % 120 == 0)
    {
        //VX_INFO("[Render] Modern shader system triangle render (frame #{}) - VAO={}", renderCallCount, m_VAO);
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

void ExampleGameLayer::SetupShaderSystem()
{
    VX_INFO("[ShaderSystem] Setting up modern shader system with advanced reflection...");
    
    try
    {
        // Create shader manager
        m_ShaderManager = std::make_shared<ShaderManager>();
        
        // Try to load advanced shaders first, fallback to simple triangle shaders
        SetupAdvancedShaders();
    }
    catch (const std::exception& e)
    {
        VX_ERROR("[ShaderSystem] Exception during shader setup: {}", e.what());
        m_TriangleShader.reset();
        m_ShaderManager.reset();
    }
}

void ExampleGameLayer::SetupAdvancedShaders()
{
    VX_INFO("[ShaderSystem] Attempting to load advanced PBR shaders...");
    
    // Load and compile advanced shaders from files
    std::string vertShaderPath = "Assets/Shaders/AdvancedTriangle.vert";
    std::string fragShaderPath = "Assets/Shaders/AdvancedTriangle.frag";
    
    // Create shader compilation request
    ShaderCompileOptions options;
    options.OptimizationLevel = ShaderOptimizationLevel::None;
    options.GenerateDebugInfo = true;
    options.TargetProfile = "opengl";
    
    // Load shader sources
    std::string vertSource, fragSource;
    if (LoadShaderFromFile(vertShaderPath, vertSource) && LoadShaderFromFile(fragShaderPath, fragSource))
    {
        // Compile shaders to SPIR-V with caching enabled
        ShaderCompiler compiler;
        
        // Enable shader caching for faster subsequent loads
        compiler.SetCachingEnabled(true, "cache/shaders/");
        
        auto vsResult = compiler.CompileFromSource(vertSource, ShaderStage::Vertex, options, "AdvancedTriangle.vert");
        auto fsResult = compiler.CompileFromSource(fragSource, ShaderStage::Fragment, options, "AdvancedTriangle.frag");
        
        if(!vsResult.IsSuccess())
        {
            VX_ERROR("[ShaderSystem] Vertex shader compilation error: {}", vsResult.GetErrorMessage());
			return;
		}
        if (!fsResult.IsSuccess())
        {
            VX_ERROR("[ShaderSystem] Fragment shader compilation error: {}", fsResult.GetErrorMessage());
            return;
        }

        // Create GPU shader
        m_TriangleShader = GPUShader::Create("AdvancedTriangleShader");

        // Prepare shader stages
        std::unordered_map<ShaderStage, std::vector<uint32_t>> shaderStages;
        shaderStages[ShaderStage::Vertex] = vsResult.GetValue().SpirV;
        shaderStages[ShaderStage::Fragment] = fsResult.GetValue().SpirV;

        // Get reflection data from compiled shaders
        ShaderReflectionData reflection = vsResult.GetValue().Reflection;

        // Merge fragment shader reflection (in a real implementation, we'd have a proper merge function)
        const auto& fragReflection = fsResult.GetValue().Reflection;
        reflection.Uniforms.insert(reflection.Uniforms.end(), fragReflection.Uniforms.begin(), fragReflection.Uniforms.end());
        reflection.Resources.insert(reflection.Resources.end(), fragReflection.Resources.begin(), fragReflection.Resources.end());

        // Create the GPU shader with reflection
        auto createResult = m_TriangleShader->Create(shaderStages, reflection);
        if (createResult.IsSuccess())
        {
            VX_INFO("[ShaderSystem] Successfully created advanced PBR shader!");
            VX_INFO("[ShaderSystem] Advanced shader debug info:\n{}", m_TriangleShader->GetDebugInfo());
            m_UsingAdvancedShader = true;
            return;
        }
        else
        {
            VX_ERROR("[ShaderSystem] Failed to create advanced GPU shader: {}", static_cast<int>(createResult.GetErrorCode()));
            m_TriangleShader.reset();
        }
    }
}

bool ExampleGameLayer::LoadShaderFromFile(const std::string& path, std::string& source)
{
    namespace fs = std::filesystem;
    fs::path cwd = fs::current_path();
    fs::path exeDir;
    
    #if defined(_WIN32)
    {
        char exePathA[MAX_PATH] = {0};
        DWORD len = GetModuleFileNameA(NULL, exePathA, MAX_PATH);
        if (len != 0) exeDir = fs::path(exePathA).parent_path();
    }
    #elif defined(__linux__)
    {
        char exePath[4096];
        ssize_t l = readlink("/proc/self/exe", exePath, sizeof(exePath)-1);
        if (l > 0) { exePath[l] = '\0'; exeDir = fs::path(exePath).parent_path(); }
    }
    #endif
    
    auto try_read = [&](const fs::path& p) -> bool 
    {
        std::ifstream f(p.string());
        if (f.is_open())
        {
            std::stringstream buffer;
            buffer << f.rdbuf();
            source = buffer.str();
            f.close();
            VX_INFO("[ShaderSystem] Loaded shader from: {}", p.string());
            return true;
        }
        return false;
    };
    
    // Try multiple candidate paths
    bool loaded = false;
    loaded = loaded || try_read(fs::path(path));
    loaded = loaded || try_read(cwd / path);
    if (!exeDir.empty()) loaded = loaded || try_read(exeDir / path);
    
    if (!loaded)
    {
        VX_WARN("[ShaderSystem] Could not load shader file: {}", path);
    }
    
    return loaded;
}