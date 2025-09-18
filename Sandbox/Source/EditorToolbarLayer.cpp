#include "EditorToolbarLayer.h"

using namespace Vortex;

void EditorToolbarLayer::DrawToolbarButton(const char* label, bool active, const ImVec4& activeColor, std::function<void()> onClick)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(activeColor.x, activeColor.y, activeColor.z, activeColor.w * 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(activeColor.x, activeColor.y, activeColor.z, activeColor.w * 0.7f));
    }

    if (ImGui::Button(label))
        onClick();

    if (active)
        ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

void EditorToolbarLayer::OnImGuiRender()
{
    auto* eng = Vortex::GetEngine();
    if (!eng) return;

    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("###EditorToolbar", nullptr, flags);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

    // Determine states
    auto mode = eng->GetRunMode();
    bool isEdit = (mode == Engine::RunMode::Edit);
    bool isPIE  = (mode == Engine::RunMode::PlayInEditor);

    // Play button
    DrawToolbarButton("Play", isPIE, ImVec4(0.20f, 0.70f, 0.20f, 1.0f), [eng]() {
        eng->SetRunMode(Engine::RunMode::PlayInEditor);
    });
    ImGui::SameLine();

    // Pause toggles an application-level pause flag via event or a simple static for now
    static bool paused = false;
    DrawToolbarButton(paused ? "Resume" : "Pause", paused, ImVec4(0.85f, 0.65f, 0.10f, 1.0f), [eng]() {
        // You can wire this to a dedicated Pause system/event. For now, just flip a static flag.
        paused = !paused;
    });
    ImGui::SameLine();

    // Stop returns to Edit mode
    DrawToolbarButton("Stop", isEdit, ImVec4(0.70f, 0.20f, 0.20f, 1.0f), [eng]() {
        eng->SetRunMode(Engine::RunMode::Edit);
    });

    ImGui::PopStyleVar();

    ImGui::End();
}