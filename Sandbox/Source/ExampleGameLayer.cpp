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
    // Triangle vertices with position and texture coordinates
    const float kVertices[] =
    {
        // Position (x, y, z)    // TexCoord (u, v)
        -0.8f, -0.8f, 0.0f,     0.0f, 0.0f, // Bottom-left
         0.8f, -0.8f, 0.0f,     1.0f, 0.0f, // Bottom-right
         0.0f,  0.8f, 0.0f,     0.5f, 1.0f  // Top-center
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

    // Vertex attributes: position (3 floats) + texcoord (2 floats) = stride of 5 floats
    const int stride = 5 * sizeof(float);
    
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

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
        
        // Set matrix uniforms (identity matrices for now - simple 2D rendering)
        glm::mat4 viewProjection = glm::mat4(1.0f);
        glm::mat4 transform = glm::mat4(1.0f);
        
        m_TriangleShader->SetUniform("u_ViewProjection", viewProjection);
        m_TriangleShader->SetUniform("u_Transform", transform);
        
        // Set uniforms for animation
        float timeValue = m_IsPaused ? 0.0f : m_GameTime;
        m_TriangleShader->SetUniform("u_Time", timeValue);
        
        // Set color uniform
        glm::vec3 triangleColor(1.0f, 0.5f, 0.2f); // Orange
        m_TriangleShader->SetUniform("u_Color", triangleColor);
        
        // Set alpha uniform
        m_TriangleShader->SetUniform("u_Alpha", 1.0f);
        
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
    VX_INFO("[ShaderSystem] Setting up modern shader system...");
    
    try
    {
        // Create shader manager
        m_ShaderManager = std::make_shared<Vortex::Shader::ShaderManager>();
        
        // Load and compile shaders from files
        // Look for shaders in the deployed Assets/Shaders directory next to the executable
        std::string vertShaderPath = "Assets/Shaders/Triangle.vert";
        std::string fragShaderPath = "Assets/Shaders/Triangle.frag";
        
        // Create shader compilation request
        Vortex::Shader::ShaderCompileOptions options;
        options.OptimizationLevel = Vortex::Shader::ShaderOptimizationLevel::None;
        options.GenerateDebugInfo = true;
        options.TargetProfile = "opengl"; // Target OpenGL for now
        
        // Check if shader files exist (fallback to embedded GLSL if not)
        VX_INFO("[ShaderSystem] Loading shaders from {} and {}", vertShaderPath, fragShaderPath);

        // Log CWD and executable directory to help diagnose asset path issues
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
        VX_INFO("[ShaderSystem] CWD: {}", cwd.string());
        if (!exeDir.empty()) VX_INFO("[ShaderSystem] EXE dir: {}", exeDir.string());
        
        // For now, let's use the ShaderCompiler directly to create SPIR-V, then pass to our GPU shader
        Vortex::Shader::ShaderCompiler compiler;
        
        // Load vertex shader source from file or use fallback
        std::string vertSource;
        std::string fragSource;
        
        // Try multiple candidate locations for vertex shader: relative, CWD, and EXE dir
        {
            std::string loadedPath;
            auto try_read = [&](const fs::path& p) -> bool 
            {
                std::ifstream f(p.string());
                if (f.is_open())
                {
                    std::stringstream buffer; buffer << f.rdbuf();
                    vertSource = buffer.str();
                    f.close();
                    loadedPath = p.string();
                    return true;
                }

                return false;
            };

            bool loaded = false;
            loaded = loaded || try_read(fs::path(vertShaderPath));
            loaded = loaded || try_read(cwd / vertShaderPath);
            if (!exeDir.empty()) loaded = loaded || try_read(exeDir / vertShaderPath);

            if (loaded) {
                VX_INFO("[ShaderSystem] Loaded vertex shader from file: {}", loadedPath);
            } else {
                // Fallback to simple embedded shader
                vertSource = R"(
                #version 450 core
                layout(location = 0) in vec3 a_Position;
                layout(location = 1) in vec2 a_TexCoord;
                layout(location = 0) out vec2 v_TexCoord;
                layout(location = 0) uniform float u_Time = 0.0;
                layout(location = 1) uniform mat4 u_ViewProjection = mat4(1.0);
                layout(location = 2) uniform mat4 u_Transform = mat4(1.0);
                void main() {
                    v_TexCoord = a_TexCoord;
                    vec4 worldPos = u_Transform * vec4(a_Position, 1.0);
                    float angle = u_Time * 0.5;
                    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
                    worldPos.xy = rotation * worldPos.xy;
                    gl_Position = u_ViewProjection * worldPos;
                }
                )";
                VX_WARN("[ShaderSystem] Using fallback vertex shader (file not found: {})", vertShaderPath);
            }
        }
        
        // Try multiple candidate locations for fragment shader: relative, CWD, and EXE dir
        {
            std::string loadedPath;
            auto try_read = [&](const fs::path& p) -> bool 
            {
                std::ifstream f(p.string());

                if (f.is_open()) 
                {
                    std::stringstream buffer; buffer << f.rdbuf();
                    fragSource = buffer.str();
                    f.close();
                    loadedPath = p.string();
                    return true;
                }

                return false;
            };

            bool loaded = false;
            loaded = loaded || try_read(fs::path(fragShaderPath));
            loaded = loaded || try_read(cwd / fragShaderPath);
            if (!exeDir.empty()) loaded = loaded || try_read(exeDir / fragShaderPath);

            if (loaded) {
                VX_INFO("[ShaderSystem] Loaded fragment shader from file: {}", loadedPath);
            } else {
                // Fallback to simple embedded shader
                fragSource = R"(
                #version 450 core
                layout(location = 0) in vec2 v_TexCoord;
                layout(location = 0) out vec4 FragColor;
                layout(location = 3) uniform vec3 u_Color = vec3(1.0, 0.5, 0.2);
                layout(location = 0) uniform float u_Time = 0.0;
                layout(location = 4) uniform float u_Alpha = 1.0;
                void main() {
                    vec2 center = vec2(0.5, 0.5);
                    float dist = distance(v_TexCoord, center);
                    float pulse = sin(u_Time * 2.0) * 0.5 + 0.5;
                    vec3 gradient = mix(u_Color, u_Color * 1.5, v_TexCoord.x);
                    gradient = mix(gradient, gradient * 0.7, dist);
                    gradient = mix(gradient * 0.8, gradient, pulse);
                    FragColor = vec4(gradient, u_Alpha);
                }
                )";
                VX_WARN("[ShaderSystem] Using fallback fragment shader (file not found: {})", fragShaderPath);
            }
        }
        
        // Compile shaders to SPIR-V
        auto vsResult = compiler.CompileFromSource(vertSource, Vortex::Shader::ShaderStage::Vertex, options, "Triangle.vert");
        auto fsResult = compiler.CompileFromSource(fragSource, Vortex::Shader::ShaderStage::Fragment, options, "Triangle.frag");
        
        if (!vsResult.IsSuccess()) 
        {
            VX_ERROR("[ShaderSystem] Vertex shader compilation failed: {}", vsResult.GetErrorMessage());
            return;
        }
        
        if (!fsResult.IsSuccess()) 
        {
            VX_ERROR("[ShaderSystem] Fragment shader compilation failed: {}", fsResult.GetErrorMessage());
            return;
        }
        
        // Create GPU shader
        m_TriangleShader = GPUShader::Create("TriangleShader");
        
        // Prepare shader stages
        std::unordered_map<Vortex::Shader::ShaderStage, std::vector<uint32_t>> shaderStages;
        shaderStages[Vortex::Shader::ShaderStage::Vertex] = vsResult.GetValue().SpirV;
        shaderStages[Vortex::Shader::ShaderStage::Fragment] = fsResult.GetValue().SpirV;
        
        // Create empty reflection data for now
        Vortex::Shader::ShaderReflectionData reflection{};
        
        // Create the GPU shader
        auto createResult = m_TriangleShader->Create(shaderStages, reflection);
        if (!createResult.IsSuccess())
        {
            VX_ERROR("[ShaderSystem] Failed to create GPU shader: {}", static_cast<int>(createResult.GetErrorCode()));
            m_TriangleShader.reset();
            return;
        }
        
        VX_INFO("[ShaderSystem] Successfully created triangle shader!");
        VX_INFO("[ShaderSystem] Shader debug info:\n{}", m_TriangleShader->GetDebugInfo());
    }
    catch (const std::exception& e)
    {
        VX_ERROR("[ShaderSystem] Exception during shader setup: {}", e.what());
        m_TriangleShader.reset();
        m_ShaderManager.reset();
    }
}
