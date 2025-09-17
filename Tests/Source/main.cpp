#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <Vortex.h>
#include <Core/Common/UUID.h>

// Initialize Vortex logging early for tests to avoid null logger dereferences
namespace {
struct VortexTestInit {
    VortexTestInit() {
        Vortex::LogConfig cfg;
        cfg.EnableAsync = false;
        cfg.EnableConsole = false;
        cfg.EnableFileLogging = false;
        cfg.LogLevel = spdlog::level::off;
        Vortex::Log::Init(cfg);
    }
    ~VortexTestInit() {
        Vortex::Log::Shutdown();
    }
} s_VortexTestInit;
}

TEST_CASE("Sanity: 1 + 1 == 2") {
    CHECK(1 + 1 == 2);
}

// Comprehensive test suite for Vortex Engine
// This file includes all test modules for the engine components

// Core System Tests
// - SystemManager: System registration, initialization, shutdown, priority ordering
// - Engine: Core engine functionality, system registration, lifecycle management
// - Error Handling: Result types, error codes, exception handling, logging

// Core Component Tests  
// - FileSystem: File operations, path handling, directory management
// - Configuration: Configuration loading, saving, validation, JSON serialization
// - Layer Stack: Layer ordering, event propagation, priority management
// - Event Dispatcher: Event subscription, dispatch, queuing
// - UUID: UUID generation, string conversion, uniqueness

// Engine System Tests
// - InputSystem: Input handling, key states, mouse events, action mapping
// - AssetSystem: Asset loading, caching, management, dependency handling
// - RenderSystem: Rendering pipeline, command execution, graphics context
// - Window: Window creation, events, properties, state management
// - Application: Application lifecycle, event handling, layer management

// Async System Tests
// - Coroutine Tasks: Async task execution, coroutine scheduling
// - Time System: Time management, delta time, frame counting

// Input System Tests
// - Input Codes: Key and mouse code validation, input state tracking