#pragma once

#include "Core/Events/Event.h"
#include "Engine/Time/Time.h"

namespace Vortex
{
    /**
     * @brief Type of layer for categorization and ordering
     */
    enum class LayerType : uint8_t
    {
        Game        = 0,    // Game logic layers (rendered first, processed last for events)
        UI          = 1,    // UI layers (rendered after game)
        Debug       = 2,    // Debug layers (rendered after UI)
        Overlay     = 3     // Overlay layers (rendered last, processed first for events)
    };

    /**
     * @brief Priority levels for layers within the same type
     * Lower values are processed first for updates/rendering
     * Higher values are processed first for events (reverse order)
     */
    enum class LayerPriority : int32_t
    {
        Lowest      = -1000,
        VeryLow     = -500,
        Low         = -100,
        Normal      = 0,
        High        = 100,
        VeryHigh    = 500,
        Highest     = 1000
    };

    /**
     * @brief Base class for all engine layers
     * 
     * Layers are the fundamental building blocks of the engine's architecture.
     * They provide a way to organize different systems (rendering, input, UI, etc.)
     * and control their execution order and event handling priority.
     * 
     * Features:
     * - Hierarchical event processing with overlays taking priority
     * - Separate update and render phases
     * - Attach/Detach lifecycle management
     * - Enable/Disable state control
     * - Debug name and profiling support
     */
    class Layer
    {
    public:
        /**
         * @brief Construct a new Layer
         * @param name Debug name for the layer
         * @param type Type of layer (affects rendering and event order)
         * @param priority Priority within the layer type
         */
        explicit Layer(const std::string& name = "Layer", 
                      LayerType type = LayerType::Game,
                      LayerPriority priority = LayerPriority::Normal);

        virtual ~Layer() = default;

        // Non-copyable but movable
        Layer(const Layer&) = delete;
        Layer& operator=(const Layer&) = delete;
        Layer(Layer&&) = default;
        Layer& operator=(Layer&&) = default;

        // ============================================================================
        // Lifecycle Methods
        // ============================================================================

        /**
         * @brief Called when the layer is added to the layer stack
         * Use this for initialization that requires the layer to be part of the stack
         */
        virtual void OnAttach() {}

        /**
         * @brief Called when the layer is removed from the layer stack
         * Use this for cleanup that requires the layer to still be part of the stack
         */
        virtual void OnDetach() {}

        /**
         * @brief Called every frame for logic updates
         */
        virtual void OnUpdate() {}

        /**
         * @brief Called every frame for rendering
         * Note: Layers are rendered in order (Game -> UI -> Debug -> Overlay)
         */
        virtual void OnRender() {}

        /**
         * @brief Called for ImGui rendering (if ImGui is enabled)
         * Use this for debug UI, tools, overlays, etc.
         */
        virtual void OnImGuiRender() {}

        // ============================================================================
        // Event Handling
        // ============================================================================

        /**
         * @brief Handle an event
         * @param event The event to handle
         * @return true if the event was handled and should not propagate further
         * 
         * Events are processed in reverse layer order (Overlay -> Debug -> UI -> Game)
         * If a layer returns true, the event stops propagating to lower layers.
         */
        virtual bool OnEvent(Event& event) { return false; }

        // ============================================================================
        // State Management
        // ============================================================================

        /**
         * @brief Enable or disable the layer
         * Disabled layers don't receive updates, renders, or events
         */
        void SetEnabled(bool enabled) { m_Enabled = enabled; }
        bool IsEnabled() const { return m_Enabled; }

        /**
         * @brief Check if the layer should block events from reaching lower layers
         * This is useful for modal dialogs, fullscreen menus, etc.
         */
        void SetBlockEvents(bool block) { m_BlockEvents = block; }
        bool ShouldBlockEvents() const { return m_BlockEvents; }

        // ============================================================================
        // Properties
        // ============================================================================

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        LayerType GetType() const { return m_Type; }
        LayerPriority GetPriority() const { return m_Priority; }

        /**
         * @brief Get the effective order value for sorting
         * Combines type and priority into a single sortable value
         */
        int32_t GetOrder() const;

        // ============================================================================
        // Debug and Profiling
        // ============================================================================

        /**
         * @brief Get debug information about the layer
         */
        virtual std::string GetDebugInfo() const;

        /**
         * @brief Called by profiler to get layer-specific stats
         */
        virtual void GetProfilingStats(std::unordered_map<std::string, float>& stats) const {}

    protected:
        std::string m_Name;
        LayerType m_Type;
        LayerPriority m_Priority;
        bool m_Enabled = true;
        bool m_BlockEvents = false;
    };

    // ============================================================================
    // Utility Functions
    // ============================================================================

    /**
     * @brief Convert LayerType to string for debugging
     */
    const char* LayerTypeToString(LayerType type);

    /**
     * @brief Convert LayerPriority to string for debugging
     */
    const char* LayerPriorityToString(LayerPriority priority);

    /**
     * @brief Create a unique layer name with suffix if needed
     */
    std::string CreateUniqueLayerName(const std::string& baseName, 
                                             const std::vector<std::unique_ptr<Layer>>& existingLayers);
}