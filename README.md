# Vortex Engine

A modern, cross‑platform C++20 game engine focused on clean architecture, strong abstractions, and developer ergonomics. Vortex provides a robust foundation with an event-driven core, a layered application model, a command-queued renderer, and an action-based input system.

- Repository: https://github.com/hcfgod/Vortex

## 🚀 Features

### Core
- System Manager with priority-based lifecycle (initialize, update, render, shutdown)
- Event System with immediate and queued dispatch, subscriptions, and scoped lifetime
- LayerStack for structured app composition (Game, UI, Debug, Overlay) with correct update/render/event order
- Application facade managing LayerStack and routing events
- Engine singleton coordinating the main loop and dispatching EngineUpdate/EngineRender events
- Time utilities (delta time, FPS, frame count)
- Logging (spdlog) with compile-time level control
- Configuration system with layered overrides (defaults, engine, user, runtime)

### Rendering
- RendererAPI abstraction (API-agnostic)
- OpenGL 4.6 backend implementation
- Thread-safe Render Command Queue (Clear, Viewport, Draw, Bind, State, Debug)
- GraphicsContext abstraction and state tracking

### Platform
- SDL3 integration for windowing, input, and platform events
- Windows and Linux support (macOS planned)
- Unified desktop entry points

### Tooling
- Premake5 workspace and projects (VS2022, gmake2)
- Precompiled headers for fast builds (Engine: vxpch.h)
- Vendored dependencies: spdlog, GLAD, GLM, nlohmann/json, SDL3 (prebuilt expected)

## 📋 Table of Contents
- Architecture Overview
- Project Structure
- Build and Run
- Core Concepts
  - Application and LayerStack
  - Event System
  - Input System
  - Rendering System
  - Configuration and Logging
- Examples
- Contributing
- License
- Roadmap
- Support

## 🏗️ Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│                      LayerStack                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────┐ │
│  │   Game      │ │     UI      │ │   Debug     │ │ Overlay │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────┘ │
├─────────────────────────────────────────────────────────────┤
│                        Engine Core                           │
│  Event • Input • Time • Render • Config • Logging            │
├─────────────────────────────────────────────────────────────┤
│                      System Manager                          │
├─────────────────────────────────────────────────────────────┤
│                       Platform Layer                         │
│               SDL3 • Window • GraphicsContext                │
└─────────────────────────────────────────────────────────────┘
```

Design principles: separation of concerns, explicit ownership, event-driven flow, API abstraction, platform isolation.

## 🗂️ Project Structure

- premake5.lua              Workspace (Vortex) and common configuration
- Engine/
  - premake5.lua           Static library project
  - Source/…               Engine source (core systems, events, input, renderer)
  - Vendor/…               Third-party libraries (spdlog, GLAD, GLM, json, SDL3)
- Sandbox/
  - premake5.lua           Example app project
  - Source/…               Example game layer and app bootstrap
- Config/…                 Runtime configs copied next to binaries (post-build)
- Build/…                  Generated outputs (by configuration/system/arch)

Workspace defaults:
- Workspace: Vortex
- Start project: Sandbox
- Output dir: Build/<Config>-<System>-<Arch>/<Project>

## 🔨 Build and Run

Prerequisites
- Windows: Visual Studio 2022 (Desktop development with C++)
- Linux: gcc/clang toolchain, OpenGL drivers, X11 deps (see Engine/Sandbox premake for link libs)
- Premake 5.0+ in PATH
- SDL3 prebuilt binaries present under Engine/Vendor/SDL3/install (Debug/Release lib dirs as referenced by premake)

Generate projects
- Windows (VS2022):
  - premake5 vs2022
  - Open Vortex.sln and build the solution (Debug/Release/Dist)
- Linux (Makefiles):
  - premake5 gmake2
  - make config=Debug -j$(nproc)

Run the Sandbox
- Windows: Build/Debug-windows-x86_64/Sandbox/Sandbox.exe
- Linux:   Build/Debug-linux-x86_64/Sandbox/Sandbox

If you hit linker errors for SDL on your platform, verify Engine/Vendor/SDL3/install/<Config>/lib contains the appropriate libraries and that include paths match Engine premake configuration.

## 🧠 Core Concepts

### Application and LayerStack
- Application owns a LayerStack
- Lifecycle integration:
  - Initialize: create/push core/game/UI layers
  - Update: LayerStack::OnUpdate(deltaTime)
  - Render: LayerStack::OnRender (submit commands)
  - Events: route to LayerStack::OnEvent (back-to-front; early exit if handled)
- Helper methods to push/pop layers for ease of use
- Enabled states respected for update/render/event propagation

Example ordering
- Update/Render: Game → UI → Debug → Overlay (front to back)
- Events: Overlay → Debug → UI → Game (back to front)

### Event System
- Engine drives frame events:
  - EngineUpdateEvent(float deltaTime)
  - EngineRenderEvent(float deltaTime)
- Subscribe with RAII-friendly handles; unsubscribe on shutdown/detach
- Immediate vs queued dispatch for deterministic flow

Example
```cpp
// Subscribe
auto sub = VX_SUBSCRIBE_EVENT(WindowResizeEvent, [](const WindowResizeEvent& e){
    VX_INFO("Resize {}x{}", e.GetWidth(), e.GetHeight());
    return false; // allow other layers to see it
});

// Dispatch immediately
VX_DISPATCH_EVENT(WindowResizeEvent(1280, 720));

// Queue for later (processed at a defined point)
VX_QUEUE_EVENT(EngineUpdateEvent(deltaTime));
```

### Input System
- Hybrid usage: high-level actions + low-level polling
- Per-frame flags cleared after layer updates during render to ensure consistent reads in Update
- Action maps allow binding multiple devices and callbacks per action phase

Example (actions)
```cpp
auto* input = Vortex::Application::Get()->GetEngine()->GetSystemManager().GetSystem<Vortex::InputSystem>();
auto map = input->CreateActionMap("Gameplay");

auto pause = map->CreateAction("Pause", Vortex::InputActionType::Button);
pause->AddBinding(Vortex::InputBinding::KeyboardKey(Vortex::KeyCode::Escape));
pause->SetCallbacks(
    [](Vortex::InputActionPhase){ /* started */ },
    [](Vortex::InputActionPhase){ VX_INFO("Paused"); },
    [](Vortex::InputActionPhase){ /* canceled */ }
);

// Low-level polling (movement)
if (input->GetKey(Vortex::KeyCode::W)) { /* move forward */ }
```

### Rendering System
- RendererAPI interface keeps codebase API-agnostic
- OpenGL backend implements the interface
- Render Command Queue ensures ordering and decouples submission from execution
- Typical frame:
  1) Layers submit commands in OnRender (BindShader, BindVertexArray, DrawIndexed, etc.)
  2) RenderSystem clears, processes queued commands, and presents

Example (queue usage)
```cpp
auto& q = Vortex::GetRenderCommandQueue();
q.Clear(Vortex::ClearCommand::All, {0.1f, 0.1f, 0.1f, 1.0f}, 1.0f, 0);
q.SetViewport(0, 0, width, height);
q.DrawIndexed(indexCount, 1);
```

### Configuration and Logging
- Layered config (Defaults < Engine < Runtime < User) with type-safe getters
- spdlog-backed logging with compile-time filtering and colorful console output

Examples
```cpp
// Config
auto& cfg = Vortex::EngineConfig::Get();
auto title = cfg.GetWindowTitle();
auto vsync = cfg.GetVSyncMode();

// Log
VX_CORE_INFO("Starting Vortex...");
VX_WARN("This is a warning");
```

## 💡 Examples

Basic custom layer
```cpp
class GameLayer : public Vortex::Layer {
public:
    GameLayer() : Layer("GameLayer", Vortex::LayerType::Game) {}

    void OnAttach() override {
        VX_INFO("GameLayer attached");
        m_Input = Vortex::InputSystem::Get();
        auto map = m_Input->CreateActionMap("Player");
        auto fire = map->CreateAction("Fire", Vortex::InputActionType::Button);
        fire->AddBinding(Vortex::InputBinding::MouseButton(Vortex::MouseCode::Left));
        fire->SetCallbacks(nullptr, [](auto){ VX_INFO("Bang!"); }, nullptr);
    }

    void OnUpdate() override {
        if (m_Input->GetKey(Vortex::KeyCode::W)) { /* move */ }
    }

    void OnRender() override {
        auto& cmd = Vortex::GetRenderCommandQueue();
        cmd.Clear(Vortex::ClearCommand::Color, {0.2f,0.2f,0.25f,1}, 1.0f, 0);
    }

private:
    Vortex::InputSystem* m_Input{};
};
```

## 🤝 Contributing

- Follow modern C++20 style, prefer RAII and smart pointers
- Use Result<T> for error propagation where applicable
- Keep platform-specific code isolated behind abstractions
- Conventional commits are appreciated for clarity (e.g., feat:, fix:, docs:)

Developer workflow
1) Fork and create a feature branch
2) Make changes with tests or example updates when relevant
3) Open a PR describing your changes and rationale

## 📄 License

MIT License. See LICENSE for details.

## 🗺️ Roadmap (selected)
- macOS support
- Additional RendererAPI backend(s) (e.g., Vulkan)
- Asset pipeline (resource management, hot reload)
- ECS integration and scene management
- Editor tooling

## 🙋 Support
- Issues: https://github.com/hcfgod/Vortex/issues

---

Vortex Engine — a clean, pragmatic foundation for building modern games and real-time applications.
