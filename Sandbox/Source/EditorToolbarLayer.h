#pragma once

#include <Vortex.h>
#include <imgui.h>
#include <functional>

class EditorToolbarLayer : public Vortex::Layer
{
public:
    EditorToolbarLayer() : Layer("EditorToolbar", Vortex::LayerType::UI, Vortex::LayerPriority::High) {}
    ~EditorToolbarLayer() override = default;

    void OnAttach() override {}
    void OnDetach() override {}
    void OnUpdate() override {}
    void OnRender() override {}

    void OnImGuiRender() override;
    bool OnEvent(Vortex::Event& e) override { return false; }

private:
    void DrawToolbarButton(const char* label, bool active, const ImVec4& activeColor, std::function<void()> onClick);
};