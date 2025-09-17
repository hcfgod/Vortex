# Vortex Engine Test Suite

This directory contains comprehensive tests for the Vortex Engine using DocTest framework.

## Test Coverage

The test suite covers all major engine components and systems:

### Core System Tests
- **SystemManager** (`TestSystemManager.cpp`): System registration, initialization, shutdown, priority ordering
- **Engine** (`TestEngine.cpp`): Core engine functionality, system registration, lifecycle management  
- **Error Handling** (`TestErrorHandling.cpp`): Result types, error codes, exception handling, logging

### Core Component Tests
- **FileSystem** (`TestFileSystem.cpp`): File operations, path handling, directory management
- **Configuration** (`TestConfiguration.cpp`): Configuration loading, saving, validation, JSON serialization
- **Layer Stack** (`TestLayerStack.cpp`): Layer ordering, event propagation, priority management
- **Event Dispatcher** (`TestEventDispatcher.cpp`): Event subscription, dispatch, queuing
- **UUID** (`TestUUID.cpp`): UUID generation, string conversion, uniqueness

### Engine System Tests
- **InputSystem** (`TestInputSystem.cpp`): Input handling, key states, mouse events, action mapping
- **AssetSystem** (`TestAssetSystem.cpp`): Asset loading, caching, management, dependency handling
- **RenderSystem** (`TestRenderSystem.cpp`): Rendering pipeline, command execution, graphics context
- **Window** (`TestWindow.cpp`): Window creation, events, properties, state management
- **Application** (`TestApplication.cpp`): Application lifecycle, event handling, layer management

### Async System Tests
- **Coroutine Tasks** (`TestCoroutineTask.cpp`): Async task execution, coroutine scheduling
- **Time System** (`TestTime.cpp`): Time management, delta time, frame counting

### Input System Tests
- **Input Codes** (`TestInputCodes.cpp`): Key and mouse code validation, input state tracking
- **Render Command Queue** (`TestRenderCommandQueue.cpp`): Command submission, execution, queuing

## Running Tests

### Windows
```bash
# Build tests
Scripts\Windows\Build.bat

# Run tests
cd Build\Debug-windows-x86_64\Tests
Tests.exe
```

### Linux
```bash
# Build tests
Scripts\Linux\build.sh

# Run tests
cd Build/Debug-linux-x86_64/Tests
./Tests
```

## Test Structure

Each test file follows a consistent structure:

1. **Mock Classes**: Test-specific mock implementations for complex dependencies
2. **Setup/Teardown**: Proper initialization and cleanup in each test case
3. **Error Handling**: Tests for both success and failure scenarios
4. **Edge Cases**: Boundary conditions and error states
5. **Integration**: Tests that verify component interactions

## Test Categories

### Unit Tests
- Individual component functionality
- Mock dependencies for isolation
- Fast execution, no external dependencies

### Integration Tests  
- Component interaction testing
- System-level functionality
- Real dependencies where appropriate

### Performance Tests
- Memory usage validation
- Execution time verification
- Resource cleanup verification

## Adding New Tests

When adding new tests:

1. **Follow naming convention**: `Test[ComponentName].cpp`
2. **Include comprehensive coverage**: Success, failure, and edge cases
3. **Use descriptive test names**: Clear what each test verifies
4. **Mock external dependencies**: Keep tests isolated and fast
5. **Clean up resources**: Ensure proper teardown in each test
6. **Update this README**: Document new test coverage

## Test Best Practices

- **Arrange-Act-Assert**: Structure tests clearly
- **One assertion per test**: Focus on single behaviors
- **Descriptive names**: Test names should explain what they verify
- **Independent tests**: Tests should not depend on each other
- **Fast execution**: Keep tests quick to run
- **Deterministic**: Tests should produce consistent results

## Mock Objects

The test suite uses mock objects to isolate components:

- **MockSystem**: For testing SystemManager
- **MockGraphicsContext**: For testing RenderSystem
- **MockWindow**: For testing Window functionality
- **TestLayer**: For testing Layer operations
- **TestAsset**: For testing AssetSystem

## Continuous Integration

Tests are designed to run in CI environments:

- No GUI dependencies
- Deterministic execution
- Proper error reporting
- Fast execution time
- Cross-platform compatibility

## Coverage Goals

- **Unit Tests**: 90%+ code coverage for core components
- **Integration Tests**: All major system interactions
- **Error Handling**: All error paths and edge cases
- **Performance**: Critical performance characteristics

## Debugging Tests

To debug failing tests:

1. **Run individual test files**: Focus on specific components
2. **Use verbose output**: Enable detailed test reporting
3. **Check mock setup**: Verify mock objects are properly configured
4. **Validate assumptions**: Ensure test expectations match implementation
5. **Review dependencies**: Check for missing or incorrect dependencies
