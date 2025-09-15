#include <doctest/doctest.h>
#include <Core/Layer/LayerStack.h>

using namespace Vortex;

namespace {
    class TestLayer : public Layer {
    public:
        TestLayer(const std::string& name, LayerType type, LayerPriority prio)
            : Layer(name, type, prio) {}
    };
}

TEST_CASE("LayerStack ordering: Game -> UI -> Debug -> Overlay, with priority")
{
    LayerStack stack;

    auto* gLow = stack.PushLayer<TestLayer>("G_Low", LayerType::Game, LayerPriority::Low);
    auto* gHi  = stack.PushLayer<TestLayer>("G_High", LayerType::Game, LayerPriority::High);
    auto* uiN  = stack.PushLayer<TestLayer>("UI_Normal", LayerType::UI, LayerPriority::Normal);
    auto* dbg  = stack.PushLayer<TestLayer>("Dbg", LayerType::Debug, LayerPriority::Normal);
    auto* ov   = stack.PushLayer<TestLayer>("Overlay", LayerType::Overlay, LayerPriority::Normal);

    REQUIRE(stack.ValidateIntegrity());

    const auto& layers = stack.GetLayers();
    REQUIRE(layers.size() == 5);

    // Game first, sorted by priority within type
    CHECK(layers[0].get() == gLow);
    CHECK(layers[1].get() == gHi);
    // Then UI, Debug, Overlay
    CHECK(layers[2].get() == uiN);
    CHECK(layers[3].get() == dbg);
    CHECK(layers[4].get() == ov);
}

TEST_CASE("LayerStack event propagation: reverse order and block events")
{
    struct Evt : public Event { 
        EVENT_CLASS_TYPE(CustomEventStart)
        EVENT_CLASS_CATEGORY(EventCategory::Debug)
    };

    class Blocker : public Layer {
    public:
        Blocker(const std::string& n, LayerType t, bool block, bool handle)
            : Layer(n, t, LayerPriority::Normal), m_Block(block), m_Handle(handle) { SetBlockEvents(block); }
        bool OnEvent(Event&) override { return m_Handle; }
    private: bool m_Block, m_Handle;
    };

    LayerStack stack;
    auto* game = stack.PushLayer<Blocker>("Game", LayerType::Game, false, false);
    auto* ui   = stack.PushLayer<Blocker>("UI", LayerType::UI, false, false);
    auto* dbg  = stack.PushLayer<Blocker>("Dbg", LayerType::Debug, false, false);
    auto* ov   = stack.PushLayer<Blocker>("Overlay", LayerType::Overlay, true, false); // blocks

    Evt e;
    CHECK(stack.OnEvent(e) == true); // blocked by overlay

    // Now let debug handle instead of block
    ov->SetBlockEvents(false);
    class Handler : public Layer { public: Handler():Layer("H",LayerType::Overlay,LayerPriority::Normal){} bool OnEvent(Event&) override { return true; } };
    auto* h = stack.PushLayer<Handler>();
    Evt e2;
    CHECK(stack.OnEvent(e2) == true); // handled by topmost overlay handler
}


