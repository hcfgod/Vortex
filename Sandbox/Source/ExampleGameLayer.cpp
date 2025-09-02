#include "ExampleGameLayer.h"
#include <glad/gl.h>

namespace {
// Example Vertex Shader
const char* kVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)";

// Example Fragment Shader - bright red for visibility
const char* kFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f); // Bright red
}
)";

// Make triangle bigger and more centered
const float kVertices[] =
{
    -0.8f, -0.8f, 0.0f, // Bottom-left
     0.8f, -0.8f, 0.0f, // Bottom-right
     0.0f,  0.8f, 0.0f  // Top-center
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
    VX_INFO("  - SPACE: Pause/Unpause (Action System)");
    VX_INFO("  - R: Reset game (Action System)");
    VX_INFO("  - Left Click: Increase score (Action System)");
    VX_INFO("  - WASD: Movement (Polling)");
    VX_INFO("  - Mouse: Look around (Polling)");
    
    SetupInputActions();

    // Simple OpenGL setup for rendering a triangle
    // Compile Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &kVertexShaderSource, NULL);
    glCompileShader(vertexShader);
    {
        GLint status = 0; glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint len = 0; glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0');
            if (len > 0) glGetShaderInfoLog(vertexShader, len, &len, log.data());
            VX_ERROR("Vertex shader compile failed: {}", log);
        }
    }

    // Compile Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &kFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    {
        GLint status = 0; glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint len = 0; glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0');
            if (len > 0) glGetShaderInfoLog(fragmentShader, len, &len, log.data());
            VX_ERROR("Fragment shader compile failed: {}", log);
        }
    }

    // Link Shaders into a Program
    m_ShaderProgram = glCreateProgram();
    glAttachShader(m_ShaderProgram, vertexShader);
    glAttachShader(m_ShaderProgram, fragmentShader);
    glLinkProgram(m_ShaderProgram);
    {
        GLint status = 0; glGetProgramiv(m_ShaderProgram, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint len = 0; glGetProgramiv(m_ShaderProgram, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0');
            if (len > 0) glGetProgramInfoLog(m_ShaderProgram, len, &len, log.data());
            VX_ERROR("Shader program link failed: {}", log);
        }
    }

    // Cleanup shaders as they're linked now
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

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

    // Vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind for safety
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ExampleGameLayer::OnDetach()
{
    VX_INFO("ExampleGameLayer detached - Final Score: {}", m_Score);

    // Cleanup GL resources
    if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
    if (m_VBO) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
    if (m_EBO) { glDeleteBuffers(1, &m_EBO); m_EBO = 0; }
    if (m_ShaderProgram) { glDeleteProgram(m_ShaderProgram); m_ShaderProgram = 0; }
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
    // Submit via the render command queue; with main loop ordering fixed,
    // these will execute this frame inside RenderSystem::Render()
    if (m_ShaderProgram != 0 && m_VAO != 0)
    {
        GetRenderCommandQueue().BindShader(m_ShaderProgram);
        GetRenderCommandQueue().BindVertexArray(m_VAO);
        GetRenderCommandQueue().DrawIndexed(3);
    }

    // Optional periodic log
    static int renderCallCount = 0;
    renderCallCount++;
    if (renderCallCount % 120 == 0)
    {
        //VX_INFO("[Render] Submitted triangle draw (frame #{}) - VAO={}, Shader={}", renderCallCount, m_VAO, m_ShaderProgram);
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
