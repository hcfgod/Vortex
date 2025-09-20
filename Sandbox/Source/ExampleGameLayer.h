#pragma once

#include <Vortex.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace Vortex;

class ExampleGameLayer : public Layer
{
public:
    ExampleGameLayer() : Layer("ExampleGame", LayerType::Game, LayerPriority::Normal) {}

    virtual ~ExampleGameLayer() = default;

    // Layer interface
    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnImGuiRender() override;
    virtual bool OnEvent(Event& event) override;

private:
    void SetupShaderSystem();
    void SetupInputActions();
    void SetupGameplayCamera();
    
    // Action callbacks
    void OnPauseAction(InputActionPhase phase);
    void OnResetAction(InputActionPhase phase);
    void OnFireAction(InputActionPhase phase);
    void OnBuildAssetsAction(InputActionPhase phase);
    
private:
    // Game state
    float m_GameTime = 0.0f;
    int m_Score = 0;
    bool m_IsPaused = false;
    
    // Simple animation
    float m_AnimationTime = 0.0f;
    float m_RotationAngle = 0.0f;
    
    // Grid testing for batching
    struct GridTestConfig {
        int gridWidth = 20;
        int gridHeight = 20;
        float quadSize = 1.0f;
        float spacing = 1.2f;
        bool useTextures = true;
        bool useRotation = false;
        bool use3DPositioning = false;
        bool useRandomColors = true;
        float animationSpeed = 1.0f;
        glm::vec3 gridOffset = glm::vec3(0.0f, 0.0f, 0.0f);
    } m_GridConfig;
    
    // Renderer2D lifetime stats (accumulate across frames)
    uint64_t m_LifetimeDrawCalls = 0;
    uint64_t m_LifetimeQuadCount = 0;

    // Renderer2D options
    bool m_PixelSnapEnabled = true;

    void RenderBatchingTestGrid();

	// Cache engine systems for optimal access
	std::shared_ptr<AssetSystem> m_AssetSystem;
	std::shared_ptr<InputSystem> m_InputSystem;
    
    // Input action map
    std::shared_ptr<InputActionMap> m_GameplayActions;

    // Asset-system managed handles
    AssetHandle<ShaderAsset> m_ShaderHandle;
    AssetHandle<TextureAsset> m_TextureHandle;

    // Camera system demo
    std::shared_ptr<PerspectiveCamera> m_MainCamera;
};