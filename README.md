# Vortex Engine

A modern, cross-platform C++ game engine built with performance, flexibility, and extensibility in mind. Vortex provides a robust foundation for game development with a clean architecture, comprehensive systems, and professional-grade tooling.

## 🚀 Features

### Core Systems
- **System Manager**: Priority-based system lifecycle management
- **Event System**: Sophisticated event-driven architecture with queued and immediate dispatch
- **Layer System**: Hierarchical layer management for UI, game logic, and debugging
- **Configuration System**: Multi-layered configuration with hot-reloading
- **Logging System**: Professional logging with async support and rotation
- **Input System**: Comprehensive input handling with action mapping
- **Time System**: High-precision timing and frame management

### Rendering
- **Renderer API**: Abstract rendering interface supporting multiple graphics APIs
- **OpenGL Renderer**: Complete OpenGL 4.6 implementation
- **Command Queue**: Thread-safe render command system
- **Graphics Context**: Platform-agnostic graphics context management

### Platform Support
- **Cross-Platform**: Windows, Linux, macOS support
- **SDL3 Integration**: Modern SDL3 for window management and input
- **Entry Point Abstraction**: Unified entry points for desktop and mobile platforms

### Development Tools
- **Build System**: Premake5-based build configuration
- **Error Handling**: Comprehensive error codes and Result<T> pattern
- **Debugging**: Crash handling and assertion system
- **Profiling**: Built-in performance monitoring

## 📋 Table of Contents

- [Architecture Overview](#architecture-overview)
- [System Architecture](#system-architecture)
- [Core Systems](#core-systems)
- [Rendering System](#rendering-system)
- [Input System](#input-system)
- [Configuration System](#configuration-system)
- [Platform Abstraction](#platform-abstraction)
- [Build System](#build-system)
- [Getting Started](#getting-started)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Contributing](#contributing)

## 🏗️ Architecture Overview

Vortex follows a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│                      Layer Stack                            │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────┐ │
│  │   Game      │ │     UI      │ │   Debug     │ │ Overlay │ │
│  │  Layers     │ │  Layers     │ │  Layers     │ │ Layers  │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────┘ │
├─────────────────────────────────────────────────────────────┤
│                      Engine Core                            │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────┐ │
│  │   Event     │ │   Input     │ │   Time      │ │ Render  │ │
│  │  System     │ │  System     │ │  System     │ │ System  │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    System Manager                           │
├─────────────────────────────────────────────────────────────┤
│                   Platform Layer                            │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────┐ │
│  │    SDL3     │ │   OpenGL     │ │   Window    │ │ Graphics│ │
│  │  Manager    │ │  Context    │ │  Manager    │ │ Context │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Design Principles

1. **Separation of Concerns**: Each system has a single responsibility
2. **Dependency Inversion**: High-level modules don't depend on low-level modules
3. **Event-Driven Architecture**: Loose coupling through events
4. **System Priority Management**: Controlled execution order
5. **Cross-Platform Abstraction**: Platform-specific code isolated

## 🔧 System Architecture

### System Manager

The System Manager orchestrates all engine systems with priority-based execution:

```cpp
// System priorities (higher number = higher priority)
enum class SystemPriority : uint32_t
{
    Low = 0,        // Background systems
    Normal = 100,   // Standard systems
    High = 200,     // Important systems
    Critical = 300  // Core systems (Time, Events)
};
```

**Key Features:**
- Automatic priority-based sorting
- Controlled initialization/shutdown order
- Error propagation and recovery
- System lifecycle management

### Event System

Sophisticated event-driven architecture supporting both immediate and queued dispatch:

```cpp
// Subscribe to events
auto subscription = VX_SUBSCRIBE_EVENT(WindowResizeEvent, 
    [](const WindowResizeEvent& e) -> bool {
        // Handle window resize
        return false; // Don't consume event
    });

// Dispatch events
VX_DISPATCH_EVENT(WindowResizeEvent(1280, 720));

// Queue events for later dispatch
VX_QUEUE_EVENT(ApplicationUpdateEvent(deltaTime));
```

**Event Types:**
- **Application Events**: Lifecycle events (start, update, render, shutdown)
- **Window Events**: Window management (resize, focus, close)
- **Input Events**: Keyboard, mouse, and gamepad input
- **Custom Events**: User-defined events

### Layer System

Hierarchical layer management for organizing game logic:

```cpp
// Layer types and processing order
enum class LayerType : uint32_t
{
    Game = 0,       // Game logic (processed first)
    UI = 100,       // User interface
    Debug = 200,    // Debug overlays
    Overlay = 300   // Top-level overlays (processed last)
};
```

**Layer Processing:**
- **Updates/Renders**: Game → UI → Debug → Overlay (front to back)
- **Events**: Overlay → Debug → UI → Game (back to front, early termination)

## 🎯 Core Systems

### Time System

High-precision timing and frame management:

```cpp
// Get timing information
float time = Time::GetTime();           // Total time since start
float deltaTime = Time::GetDeltaTime(); // Time since last frame
uint64_t frameCount = Time::GetFrameCount();
float fps = Time::GetFPS();
```

### Configuration System

Multi-layered configuration with hot-reloading:

```cpp
// Configuration layers (higher priority overrides lower)
1. Defaults (Priority: 0)
2. Engine Config (Priority: 100)
3. User Preferences (Priority: 1000)
4. Runtime Overrides (Priority: 900)

// Access configuration
auto& config = EngineConfig::Get();
std::string title = config.GetWindowTitle();
int width = config.GetWindowWidth();
bool vsync = config.GetVSyncMode() == "Enabled";
```

**Configuration Features:**
- JSON-based configuration
- Type-safe accessors
- Hot-reloading support
- User preference persistence
- Runtime overrides

### Logging System

Professional logging with async support:

```cpp
// Log levels
VX_CORE_TRACE("Detailed trace information");
VX_CORE_DEBUG("Debug information");
VX_CORE_INFO("General information");
VX_CORE_WARN("Warning messages");
VX_CORE_ERROR("Error messages");
VX_CORE_CRITICAL("Critical errors");

// Client logging
VX_TRACE("Client trace");
VX_INFO("Client info");
VX_ERROR("Client error");
```

**Logging Features:**
- Async logging for performance
- File rotation and management
- Colored console output
- Structured logging
- Performance metrics

## 🎨 Rendering System

### Renderer API

Abstract rendering interface supporting multiple graphics APIs:

```cpp
// Renderer API interface
class RendererAPI
{
public:
    virtual Result<void> Initialize(GraphicsContext* context) = 0;
    virtual Result<void> Clear(uint32_t flags, const glm::vec4& color, 
                              float depth, int32_t stencil) = 0;
    virtual Result<void> SetViewport(uint32_t x, uint32_t y, 
                                   uint32_t width, uint32_t height) = 0;
    virtual Result<void> DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex, int32_t baseVertex,
                                   uint32_t baseInstance) = 0;
    // ... more methods
};
```

### Command Queue System

Thread-safe render command system:

```cpp
// Submit render commands
auto& queue = GetRenderCommandQueue();

// Clear screen
queue.Clear(ClearCommand::All, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));

// Set viewport
queue.SetViewport(0, 0, 1280, 720);

// Draw geometry
queue.DrawIndexed(indexCount, instanceCount);

// Process commands (call from render thread)
queue.ProcessCommands();
```

**Command Types:**
- **Clear Commands**: Screen clearing operations
- **Viewport Commands**: Viewport and scissor management
- **Draw Commands**: Geometry rendering
- **Binding Commands**: Resource binding
- **State Commands**: Render state management
- **Debug Commands**: Profiling and debugging

### OpenGL Implementation

Complete OpenGL 4.6 renderer with state tracking:

```cpp
class OpenGLRendererAPI : public RendererAPI
{
    // Implements all RendererAPI methods
    // Includes state tracking for optimization
    // Supports modern OpenGL features
};
```

**OpenGL Features:**
- Modern OpenGL 4.6 support
- State tracking for performance
- Error checking and debugging
- Resource management
- Debug groups for profiling

## 🎮 Input System

### Input Action System

Comprehensive input handling with action mapping:

```cpp
// Create input action map
auto actionMap = inputSystem->CreateActionMap("Player");

// Create actions
auto moveAction = actionMap->CreateAction("Move", InputActionType::Value);
auto jumpAction = actionMap->CreateAction("Jump", InputActionType::Button);

// Add bindings
moveAction->AddBinding(InputBinding::KeyboardKey(KeyCode::W));
moveAction->AddBinding(InputBinding::KeyboardKey(KeyCode::S));
jumpAction->AddBinding(InputBinding::KeyboardKey(KeyCode::Space));

// Set callbacks
jumpAction->SetCallbacks(
    [](InputActionPhase phase) { /* Jump started */ },
    [](InputActionPhase phase) { /* Jump performed */ },
    [](InputActionPhase phase) { /* Jump canceled */ }
);
```

### Device Support

Support for multiple input devices:

```cpp
// Keyboard queries
bool isKeyDown = inputSystem->GetKey(KeyCode::W);
bool keyPressed = inputSystem->GetKeyDown(KeyCode::Space);

// Mouse queries
bool mouseDown = inputSystem->GetMouseButton(MouseCode::Left);
float mouseX, mouseY;
inputSystem->GetMousePosition(mouseX, mouseY);

// Gamepad queries
bool gamepadConnected = inputSystem->IsGamepadConnected(0);
bool buttonPressed = inputSystem->GetGamepadButtonDown(0, 0);
float axisValue = inputSystem->GetGamepadAxis(0, 0);
```

**Input Features:**
- Multi-device support (keyboard, mouse, gamepad)
- Action mapping system
- Input rebinding support
- Frame-based state tracking
- Event-driven input processing

## 🌐 Platform Abstraction

### SDL3 Integration

Modern SDL3 for cross-platform support:

```cpp
// SDL3 Manager handles initialization
SDL3Manager::Initialize(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD);

// Window creation
auto window = std::make_unique<SDLWindow>(windowProps);

// Event processing
SDL_Event event;
while (SDL_PollEvent(&event)) {
    window->ProcessEvent(event);
}
```

### Entry Point Abstraction

Unified entry points for different platforms:

```cpp
// Desktop platforms (Windows, Linux, macOS)
int main(int argc, char** argv) {
    // Traditional main function
    auto app = CreateApplication();
    app->Run(engine);
    return 0;
}

// Mobile platforms (iOS, Android)
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    // SDL3 callback-based initialization
    return SDL_APP_CONTINUE;
}
```

## 🔨 Build System

### Premake5 Configuration

Modern build system with cross-platform support:

```lua
-- Engine project configuration
project "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    
    -- Platform-specific settings
    filter "system:windows"
        defines { "VX_PLATFORM_WINDOWS", "VX_USE_SDL" }
        
    filter "system:linux"
        defines { "VX_PLATFORM_LINUX", "VX_USE_SDL" }
        links { "SDL3", "GL", "X11" }
```

### Build Configurations

Multiple build configurations for different needs:

- **Debug**: Full debugging information, assertions enabled
- **Release**: Optimized builds with minimal debugging
- **Dist**: Distribution builds with maximum optimization

## 🚀 Getting Started

### Prerequisites

- **C++20 Compatible Compiler**: MSVC 2019+, GCC 10+, Clang 12+
- **CMake 3.20+** or **Premake5**
- **SDL3**: Cross-platform multimedia library
- **OpenGL 4.6**: Graphics API (driver support required)

### Building from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/yourusername/vortex-engine.git
   cd vortex-engine
   ```

2. **Generate build files:**
   ```bash
   # Using Premake5
   premake5 vs2022  # Visual Studio 2022
   premake5 gmake2  # GNU Make
   premake5 xcode4  # Xcode
   
   # Or using CMake
   mkdir build && cd build
   cmake ..
   ```

3. **Build the engine:**
   ```bash
   # Visual Studio
   msbuild Vortex.sln /p:Configuration=Debug
   
   # GNU Make
   make -j$(nproc)
   
   # CMake
   cmake --build . --config Debug
   ```

### Running the Sandbox

The sandbox application demonstrates engine features:

```bash
# Windows
./Build/Debug-windows-x86_64/Sandbox/Sandbox.exe

# Linux
./Build/Debug-linux-x86_64/Sandbox/Sandbox

# macOS
./Build/Debug-macos-x86_64/Sandbox/Sandbox
```

## 📚 API Reference

### Core Classes

#### Application
```cpp
class Application
{
public:
    // Layer management
    Layer* PushLayer(std::unique_ptr<Layer> layer);
    bool PopLayer(Layer* layer);
    
    // Event handling
    virtual bool OnKeyPressed(const KeyPressedEvent& event);
    virtual bool OnWindowResize(const WindowResizeEvent& event);
    
    // Lifecycle
    virtual void Initialize();
    virtual void Update();
    virtual void Render();
    virtual void Shutdown();
};
```

#### Engine
```cpp
class Engine
{
public:
    Result<void> Initialize();
    Result<void> Update();
    Result<void> Render();
    Result<void> Shutdown();
    
    SystemManager& GetSystemManager();
    bool IsRunning() const;
    void Stop();
};
```

#### SystemManager
```cpp
class SystemManager
{
public:
    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args);
    
    template<typename T>
    T* GetSystem() const;
    
    Result<void> InitializeAllSystems();
    Result<void> UpdateAllSystems();
    Result<void> RenderAllSystems();
    Result<void> ShutdownAllSystems();
};
```

### Event System

#### Event Types
```cpp
// Application events
ApplicationStartedEvent
ApplicationUpdateEvent(float deltaTime)
ApplicationRenderEvent(float deltaTime)
ApplicationShutdownEvent

// Window events
WindowCloseEvent
WindowResizeEvent(uint32_t width, uint32_t height)
WindowFocusEvent
WindowLostFocusEvent

// Input events
KeyPressedEvent(KeyCode key, bool isRepeat)
KeyReleasedEvent(KeyCode key)
KeyTypedEvent(uint32_t character)
MouseButtonPressedEvent(MouseCode button)
MouseButtonReleasedEvent(MouseCode button)
MouseMovedEvent(float x, float y)
MouseScrolledEvent(float xOffset, float yOffset)
GamepadConnectedEvent(int gamepadId)
GamepadDisconnectedEvent(int gamepadId)
GamepadButtonPressedEvent(int gamepadId, int button)
GamepadButtonReleasedEvent(int gamepadId, int button)
GamepadAxisEvent(int gamepadId, int axis, float value)
```

#### Event Macros
```cpp
// Subscribe to events
VX_SUBSCRIBE_EVENT(eventType, handler)
VX_SUBSCRIBE_EVENT_METHOD(eventType, instance, method)

// Dispatch events
VX_DISPATCH_EVENT(event)
VX_QUEUE_EVENT(event)

// Unsubscribe from events
VX_UNSUBSCRIBE_EVENT(subscriptionId)
```

### Rendering System

#### Render Commands
```cpp
// Clear commands
ClearCommand(uint32_t flags, const glm::vec4& color, float depth, int32_t stencil)

// Viewport commands
SetViewportCommand(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
SetScissorCommand(bool enabled, uint32_t x, uint32_t y, uint32_t width, uint32_t height)

// Draw commands
DrawIndexedCommand(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t baseInstance)
DrawArraysCommand(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t baseInstance)

// Binding commands
BindVertexBufferCommand(uint32_t binding, uint32_t buffer, uint64_t offset, uint64_t stride)
BindIndexBufferCommand(uint32_t buffer, IndexType type, uint64_t offset)
BindShaderCommand(uint32_t program)
BindTextureCommand(uint32_t slot, uint32_t texture, uint32_t sampler)

// State commands
SetDepthStateCommand(bool testEnabled, bool writeEnabled, CompareFunction func)
SetBlendStateCommand(bool enabled, BlendFactor srcFactor, BlendFactor dstFactor, BlendOperation op)
SetCullStateCommand(CullMode mode, FrontFace face)

// Debug commands
PushDebugGroupCommand(const std::string& name)
PopDebugGroupCommand()
```

#### Render Command Queue
```cpp
// Submit commands
queue.Submit(std::make_unique<ClearCommand>(flags, color, depth, stencil));
queue.Submit(std::make_unique<DrawIndexedCommand>(indexCount, instanceCount));

// Process commands
queue.ProcessCommands();

// Convenience methods
queue.Clear(flags, color, depth, stencil);
queue.SetViewport(x, y, width, height);
queue.DrawIndexed(indexCount, instanceCount);
queue.BindShader(program);
```

### Input System

#### Input Actions
```cpp
// Action types
enum class InputActionType : uint8_t
{
    Button,     // Binary input (pressed/released)
    Value,      // Continuous input (analog)
    PassThrough // Direct input passthrough
};

// Action phases
enum class InputActionPhase : uint8_t
{
    Waiting,    // No input detected
    Started,    // Input just started
    Performed,  // Input is active
    Canceled    // Input was canceled
};

// Create actions
auto action = actionMap->CreateAction("Move", InputActionType::Value);
action->AddBinding(InputBinding::KeyboardKey(KeyCode::W));
action->SetCallbacks(startedCallback, performedCallback, canceledCallback);
```

#### Input Queries
```cpp
// Keyboard
bool isKeyDown = inputSystem->GetKey(KeyCode::W);
bool keyPressed = inputSystem->GetKeyDown(KeyCode::Space);
bool keyReleased = inputSystem->GetKeyUp(KeyCode::Escape);

// Mouse
bool mouseDown = inputSystem->GetMouseButton(MouseCode::Left);
bool mousePressed = inputSystem->GetMouseButtonDown(MouseCode::Right);
float mouseX, mouseY;
inputSystem->GetMousePosition(mouseX, mouseY);
float deltaX, deltaY;
inputSystem->GetMouseDelta(deltaX, deltaY);

// Gamepad
bool gamepadConnected = inputSystem->IsGamepadConnected(0);
bool buttonPressed = inputSystem->GetGamepadButtonDown(0, 0);
float axisValue = inputSystem->GetGamepadAxis(0, 0);
```

### Configuration System

#### EngineConfig
```cpp
// Window settings
std::string title = config.GetWindowTitle();
int width = config.GetWindowWidth();
int height = config.GetWindowHeight();
bool fullscreen = config.GetWindowFullscreen();

// Renderer settings
std::string api = config.GetRendererAPI();
std::string vsync = config.GetVSyncMode();
int msaaSamples = config.GetMSAASamples();
Color clearColor = config.GetClearColor();

// Input settings
float mouseSensitivity = config.GetMouseSensitivity();
bool invertMouseY = config.GetInvertMouseY();
bool enableGamepad = config.GetEnableGamepad();

// Performance settings
bool enableProfiling = config.GetEnableProfiling();
bool enableGPUDebug = config.GetEnableGPUDebug();
```

#### Configuration Layers
```cpp
// Set values in specific layers
config.Set("Window.Title", "My Game", "UserPreferences", true, 1000);
config.Set("Renderer.VSync", "Disabled", "RuntimeOverrides", true, 900);

// Get merged values (higher priority overrides lower)
std::string title = config.GetWindowTitle(); // Returns merged value
```

## 💡 Examples

### Basic Application Setup

```cpp
class MyGame : public Vortex::Application
{
public:
    void Initialize() override
    {
        // Create game layer
        m_GameLayer = PushLayer<GameLayer>();
        
        // Subscribe to events
        m_Subscription = VX_SUBSCRIBE_EVENT(WindowResizeEvent, 
            [this](const WindowResizeEvent& e) -> bool {
                VX_INFO("Window resized to {}x{}", e.GetWidth(), e.GetHeight());
                return false;
            });
    }
    
    void Update() override
    {
        // Game logic update
    }
    
    void Render() override
    {
        // Rendering code
    }
    
    void Shutdown() override
    {
        // Cleanup
        VX_UNSUBSCRIBE_EVENT(m_Subscription);
    }
    
private:
    GameLayer* m_GameLayer = nullptr;
    Vortex::SubscriptionID m_Subscription = 0;
};

Vortex::Application* Vortex::CreateApplication()
{
    return new MyGame();
}
```

### Custom Layer Implementation

```cpp
class GameLayer : public Vortex::Layer
{
public:
    GameLayer() : Layer("GameLayer", LayerType::Game) {}
    
    void OnAttach() override
    {
        VX_INFO("GameLayer attached");
        
        // Initialize game systems
        m_InputSystem = Vortex::InputSystem::Get();
        
        // Create input actions
        auto actionMap = m_InputSystem->CreateActionMap("Player");
        m_MoveAction = actionMap->CreateAction("Move", Vortex::InputActionType::Value);
        m_JumpAction = actionMap->CreateAction("Jump", Vortex::InputActionType::Button);
        
        // Add bindings
        m_MoveAction->AddBinding(Vortex::InputBinding::KeyboardKey(Vortex::KeyCode::W));
        m_MoveAction->AddBinding(Vortex::InputBinding::KeyboardKey(Vortex::KeyCode::S));
        m_JumpAction->AddBinding(Vortex::InputBinding::KeyboardKey(Vortex::KeyCode::Space));
        
        // Set callbacks
        m_JumpAction->SetCallbacks(
            [this](Vortex::InputActionPhase phase) { OnJumpStarted(); },
            [this](Vortex::InputActionPhase phase) { OnJumpPerformed(); },
            [this](Vortex::InputActionPhase phase) { OnJumpCanceled(); }
        );
    }
    
    void OnDetach() override
    {
        VX_INFO("GameLayer detached");
    }
    
    void OnUpdate() override
    {
        // Update game logic
        float deltaTime = Vortex::Time::GetDeltaTime();
        
        // Handle input
        if (m_InputSystem->GetKey(Vortex::KeyCode::W))
        {
            // Move forward
        }
    }
    
    void OnRender() override
    {
        // Render game objects
        auto& renderQueue = Vortex::GetRenderCommandQueue();
        
        // Clear screen
        renderQueue.Clear(Vortex::ClearCommand::All, 
                         glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
        
        // Set viewport
        renderQueue.SetViewport(0, 0, 1280, 720);
        
        // Draw game objects
        // ... rendering code
    }
    
    bool OnEvent(Vortex::Event& event) override
    {
        // Handle events
        Vortex::EventDispatcher dispatcher(event);
        
        dispatcher.Dispatch<Vortex::KeyPressedEvent>(
            [this](const Vortex::KeyPressedEvent& e) -> bool {
                if (e.GetKeyCode() == Vortex::KeyCode::Escape)
                {
                    // Handle escape key
                    return true; // Consume event
                }
                return false;
            });
        
        return false;
    }
    
private:
    Vortex::InputSystem* m_InputSystem = nullptr;
    Vortex::InputAction* m_MoveAction = nullptr;
    Vortex::InputAction* m_JumpAction = nullptr;
    
    void OnJumpStarted() { VX_INFO("Jump started"); }
    void OnJumpPerformed() { VX_INFO("Jump performed"); }
    void OnJumpCanceled() { VX_INFO("Jump canceled"); }
};
```

### Custom System Implementation

```cpp
class PhysicsSystem : public Vortex::EngineSystem
{
public:
    PhysicsSystem() : EngineSystem("PhysicsSystem", Vortex::SystemPriority::Normal) {}
    
    Result<void> Initialize() override
    {
        VX_CORE_INFO("PhysicsSystem initializing...");
        
        // Initialize physics engine
        // ... initialization code
        
        MarkInitialized();
        return Result<void>();
    }
    
    Result<void> Update() override
    {
        float deltaTime = Vortex::Time::GetDeltaTime();
        
        // Update physics simulation
        // ... physics update code
        
        return Result<void>();
    }
    
    Result<void> Render() override
    {
        // Render physics debug info
        // ... debug rendering code
        
        return Result<void>();
    }
    
    Result<void> Shutdown() override
    {
        VX_CORE_INFO("PhysicsSystem shutting down...");
        
        // Cleanup physics engine
        // ... cleanup code
        
        MarkShutdown();
        return Result<void>();
    }
};

// Register in Engine::RegisterCoreSystems()
auto* physicsSystem = m_SystemManager.RegisterSystem<PhysicsSystem>();
```

### Configuration Example

```cpp
// engine.json
{
  "Engine": {
    "Version": "1.0.0",
    "EnableAsserts": true
  },
  "Window": {
    "Title": "My Game",
    "Width": 1280,
    "Height": 720,
    "Fullscreen": false,
    "Resizable": true
  },
  "Renderer": {
    "API": "OpenGL",
    "VSync": "Enabled",
    "MSAASamples": 4,
    "ClearColor": { "r": 0.1, "g": 0.1, "b": 0.1, "a": 1.0 }
  },
  "Input": {
    "MouseSensitivity": 1.0,
    "InvertMouseY": false,
    "EnableGamepad": true,
    "GamepadDeadzone": 0.1
  },
  "Performance": {
    "EnableProfiling": true,
    "EnableGPUDebug": false,
    "MaxFrameRate": 60
  }
}
```

## 🤝 Contributing

We welcome contributions to Vortex Engine! Please read our contributing guidelines:

### Development Setup

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/amazing-feature`
3. **Make your changes**: Follow the existing code style
4. **Add tests**: Ensure new features are properly tested
5. **Commit your changes**: Use conventional commit messages
6. **Push to the branch**: `git push origin feature/amazing-feature`
7. **Open a Pull Request**: Describe your changes clearly

### Code Style Guidelines

- **C++20**: Use modern C++ features appropriately
- **Naming**: Use descriptive names, follow existing conventions
- **Documentation**: Document public APIs and complex logic
- **Error Handling**: Use Result<T> pattern for error handling
- **Memory Management**: Prefer smart pointers over raw pointers
- **Thread Safety**: Consider thread safety for shared resources

### Testing

- **Unit Tests**: Write tests for new functionality
- **Integration Tests**: Test system interactions
- **Performance Tests**: Ensure performance requirements are met
- **Platform Tests**: Test on multiple platforms

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **SDL3**: Cross-platform multimedia library
- **spdlog**: Fast C++ logging library
- **GLM**: OpenGL Mathematics library
- **nlohmann/json**: JSON library for C++
- **GLAD**: OpenGL loading library

## 📞 Support

- **Documentation**: [Wiki](https://github.com/yourusername/vortex-engine/wiki)
- **Issues**: [GitHub Issues](https://github.com/yourusername/vortex-engine/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/vortex-engine/discussions)
- **Discord**: [Join our Discord](https://discord.gg/vortex-engine)

---

**Vortex Engine** - Building the future of game development, one system at a time.