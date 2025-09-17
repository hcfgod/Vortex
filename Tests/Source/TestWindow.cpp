#include <doctest/doctest.h>
#include <Core/Window.h>
#include <Core/Events/WindowEvent.h>
#include <Core/Debug/ErrorCodes.h>
#include <spdlog/fmt/ostr.h>

#ifdef VX_USE_SDL
#include <SDL3/SDL.h>
#endif

using namespace Vortex;

namespace {
    class MockWindow : public Window {
    public:
        MockWindow(const WindowProperties& props) : m_Properties(props), m_Valid(true) {}
        
        bool IsValid() const override {
            return m_Valid;
        }
        
        uint32_t GetWindowID() const override {
            return 1;
        }
        
        void SetTitle(const std::string& title) override {
            m_Properties.Title = title;
        }
        
        std::string GetTitle() const override {
            return m_Properties.Title;
        }
        
        void SetSize(int width, int height) override {
            m_Properties.Width = width;
            m_Properties.Height = height;
        }
        
        void GetSize(int& width, int& height) const override {
            width = m_Properties.Width;
            height = m_Properties.Height;
        }
        
        int GetWidth() const override {
            return m_Properties.Width;
        }
        
        int GetHeight() const override {
            return m_Properties.Height;
        }
        
        void SetPosition(int x, int y) override {
            m_X = x;
            m_Y = y;
        }
        
        void GetPosition(int& x, int& y) const override {
            x = m_X;
            y = m_Y;
        }
        
        void SetFullscreen(bool fullscreen) override {
            m_Properties.Fullscreen = fullscreen;
        }
        
        bool IsFullscreen() const override {
            return m_Properties.Fullscreen;
        }
        
        void Show() override {
            m_Visible = true;
        }
        
        void Hide() override {
            m_Visible = false;
        }
        
        bool IsVisible() const override {
            return m_Visible;
        }
        
        void Minimize() override {
            m_Minimized = true;
        }
        
        void Maximize() override {
            m_Maximized = true;
        }
        
        void Restore() override {
            m_Minimized = false;
            m_Maximized = false;
        }
        
        bool IsMinimized() const {
            return m_Minimized;
        }
        
        bool IsMaximized() const {
            return m_Maximized;
        }
        
        void Center() override {
            m_X = 100;
            m_Y = 100;
        }
        
        const WindowProperties& GetProperties() const override {
            return m_Properties;
        }
        
        bool ProcessEvent(const SDL_Event& event) override {
            return false;
        }
        
        void* GetNativeHandle() const override {
            return nullptr;
        }
        
        // Test tracking
        bool m_Valid = true;
        bool m_Visible = true;
        bool m_Minimized = false;
        bool m_Maximized = false;
        int m_X = 100;
        int m_Y = 100;
        WindowProperties m_Properties;
    };
}

TEST_CASE("Window creation and initialization") {
    WindowProperties props;
    props.Title = "Test Window";
    props.Width = 800;
    props.Height = 600;
    
    auto window = std::make_unique<MockWindow>(props);
    CHECK(window != nullptr);
    CHECK(window->IsValid());
    CHECK(window->GetTitle() == "Test Window");
    CHECK(window->GetWidth() == 800);
    CHECK(window->GetHeight() == 600);
}

TEST_CASE("Window properties") {
    WindowProperties props;
    props.Title = "Test Window";
    props.Width = 1024;
    props.Height = 768;
    
    auto window = std::make_unique<MockWindow>(props);
    REQUIRE(window != nullptr);
    
    // Test title
    CHECK(window->GetTitle() == "Test Window");
    
    window->SetTitle("New Title");
    CHECK(window->GetTitle() == "New Title");
    
    // Test size
    CHECK(window->GetWidth() == 1024);
    CHECK(window->GetHeight() == 768);
    
    window->SetSize(800, 600);
    CHECK(window->GetWidth() == 800);
    CHECK(window->GetHeight() == 600);
    
    // Test position
    int x, y;
    window->GetPosition(x, y);
    CHECK(x == 100);
    CHECK(y == 100);
    
    window->SetPosition(200, 300);
    window->GetPosition(x, y);
    CHECK(x == 200);
    CHECK(y == 300);
}

TEST_CASE("Window state management") {
    WindowProperties props;
    props.Title = "Test Window";
    
    auto window = std::make_unique<MockWindow>(props);
    REQUIRE(window != nullptr);
    
    // Test fullscreen
    CHECK(!window->IsFullscreen());
    
    window->SetFullscreen(true);
    CHECK(window->IsFullscreen());
    
    // Test visibility
    CHECK(window->IsVisible());
    
    window->Hide();
    CHECK(!window->IsVisible());
    
    window->Show();
    CHECK(window->IsVisible());
}

TEST_CASE("Window focus and minimization") {
    WindowProperties props;
    props.Title = "Test Window";
    
    auto window = std::make_unique<MockWindow>(props);
    REQUIRE(window != nullptr);
    
    // Test minimization
    CHECK(!window->IsMinimized());
    
    window->Minimize();
    CHECK(window->IsMinimized());
    
    // Test maximization
    CHECK(!window->IsMaximized());
    
    window->Maximize();
    CHECK(window->IsMaximized());
    
    // Test restore
    window->Restore();
    CHECK(!window->IsMinimized());
    CHECK(!window->IsMaximized());
}

TEST_CASE("Window center") {
    WindowProperties props;
    props.Title = "Test Window";
    
    auto window = std::make_unique<MockWindow>(props);
    REQUIRE(window != nullptr);
    
    // Test center
    window->Center();
    int x, y;
    window->GetPosition(x, y);
    CHECK(x == 100);
    CHECK(y == 100);
}

TEST_CASE("Window properties access") {
    WindowProperties props;
    props.Title = "Test Window";
    props.Width = 800;
    props.Height = 600;
    props.Fullscreen = true;
    props.Resizable = false;
    
    auto window = std::make_unique<MockWindow>(props);
    REQUIRE(window != nullptr);
    
    const auto& properties = window->GetProperties();
    CHECK(properties.Title == "Test Window");
    CHECK(properties.Width == 800);
    CHECK(properties.Height == 600);
    CHECK(properties.Fullscreen == true);
    CHECK(properties.Resizable == false);
}

TEST_CASE("Window native handle") {
    WindowProperties props;
    props.Title = "Test Window";
    
    auto window = std::make_unique<MockWindow>(props);
    REQUIRE(window != nullptr);
    
    // Test native window handle
    void* nativeHandle = window->GetNativeHandle();
    // Note: MockWindow returns nullptr, but real implementations would return actual handles
    CHECK(nativeHandle == nullptr);
}

TEST_CASE("Window multiple instances") {
    WindowProperties props1;
    props1.Title = "Window 1";
    props1.Width = 400;
    props1.Height = 300;
    
    WindowProperties props2;
    props2.Title = "Window 2";
    props2.Width = 600;
    props2.Height = 400;
    
    auto window1 = std::make_unique<MockWindow>(props1);
    auto window2 = std::make_unique<MockWindow>(props2);
    
    CHECK(window1 != nullptr);
    CHECK(window2 != nullptr);
    CHECK(window1 != window2);
    
    CHECK(window1->GetTitle() == "Window 1");
    CHECK(window2->GetTitle() == "Window 2");
    CHECK(window1->GetWidth() == 400);
    CHECK(window2->GetWidth() == 600);
}

TEST_CASE("Window event processing") {
    WindowProperties props;
    props.Title = "Test Window";
    
    auto window = std::make_unique<MockWindow>(props);
    REQUIRE(window != nullptr);
    
    // Test event processing
    SDL_Event event = {};
    bool result = window->ProcessEvent(event);
    CHECK(!result); // MockWindow always returns false
}

TEST_CASE("Window ID") {
    WindowProperties props;
    props.Title = "Test Window";
    
    auto window = std::make_unique<MockWindow>(props);
    REQUIRE(window != nullptr);
    
    // Test window ID
    uint32_t id = window->GetWindowID();
    CHECK(id == 1); // MockWindow returns 1
}
