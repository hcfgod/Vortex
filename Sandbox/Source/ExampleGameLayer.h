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
    virtual bool OnEvent(Event& event) override;

private:
    void SetupShaderSystem();
    void SetupInputActions();
    
    // Action callbacks
    void OnPauseAction(InputActionPhase phase);
    void OnResetAction(InputActionPhase phase);
    void OnFireAction(InputActionPhase phase);
    


private:
    // Game state
    float m_GameTime = 0.0f;
    int m_Score = 0;
    bool m_IsPaused = false;
    
    // Simple animation
    float m_AnimationTime = 0.0f;
    float m_RotationAngle = 0.0f;

	// Cache engine systems for optimal access
	std::shared_ptr<AssetSystem> m_AssetSystem;
	std::shared_ptr<InputSystem> m_InputSystem;
    
    // Input action map
    std::shared_ptr<InputActionMap> m_GameplayActions;

    // Asset-system managed handles
    AssetHandle<ShaderAsset> m_ShaderHandle;
    AssetHandle<TextureAsset> m_TextureHandle;

    // Loading indicator state
    bool m_ShaderLoading = false;
    float m_ShaderProgress = 0.0f;

    // Resources for demo triangle (VAO/VBO still needed for vertex data)
    unsigned int m_VBO = 0;
    unsigned int m_VAO = 0;
    unsigned int m_EBO = 0;

    // High-level rendering objects
    std::shared_ptr<VertexArray> m_VertexArray;
	std::shared_ptr<VertexBuffer> m_VertexBuffer;
	std::shared_ptr<IndexBuffer> m_IndexBuffer;
};