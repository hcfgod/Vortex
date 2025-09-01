#pragma once

#include "Layer.h"
#include "Core/Events/Event.h"
#include "Engine/Time/Time.h"

namespace Vortex
{
    /**
     * @brief Manages a stack of layers with proper ordering and lifecycle
     * 
     * The LayerStack is responsible for:
     * - Managing layer lifetimes and ownership
     * - Maintaining proper layer ordering (Game -> UI -> Debug -> Overlay)
     * - Dispatching updates, renders, and events to layers
     * - Providing efficient iteration and lookup capabilities
     * - Handling layer insertion/removal with automatic sorting
     * 
     * Layer Processing Order:
     * - Updates/Renders: Game -> UI -> Debug -> Overlay (front to back)
     * - Events: Overlay -> Debug -> UI -> Game (back to front, early termination)
     * 
     * This design allows overlays to intercept events first, while game layers
     * render first to be properly occluded by UI and debug overlays.
     */
    class LayerStack
    {
    public:
        // Type aliases for cleaner code
        using LayerContainer = std::vector<std::unique_ptr<Layer>>;
        using iterator = LayerContainer::iterator;
        using const_iterator = LayerContainer::const_iterator;
        using reverse_iterator = LayerContainer::reverse_iterator;
        using const_reverse_iterator = LayerContainer::const_reverse_iterator;

        LayerStack();
        ~LayerStack();

        // Non-copyable but movable
        LayerStack(const LayerStack&) = delete;
        LayerStack& operator=(const LayerStack&) = delete;
        LayerStack(LayerStack&&) = default;
        LayerStack& operator=(LayerStack&&) = default;

        // ============================================================================
        // Layer Management
        // ============================================================================

        /**
         * @brief Add a layer to the stack
         * @param layer Unique pointer to the layer (transfers ownership)
         * @return Raw pointer to the layer for convenience (still owned by LayerStack)
         * 
         * The layer will be automatically sorted into the correct position based on
         * its type and priority. OnAttach() will be called immediately.
         */
        Layer* PushLayer(std::unique_ptr<Layer> layer);

        /**
         * @brief Create and add a layer to the stack
         * @tparam T Layer type to create (must inherit from Layer)
         * @tparam Args Constructor argument types
         * @param args Constructor arguments
         * @return Raw pointer to the created layer
         */
        template<typename T, typename... Args>
        T* PushLayer(Args&&... args)
        {
            static_assert(std::is_base_of_v<Layer, T>, "T must inherit from Layer");
            auto layer = std::make_unique<T>(std::forward<Args>(args)...);
            T* ptr = layer.get();
            PushLayer(std::move(layer));
            return ptr;
        }

        /**
         * @brief Remove a layer from the stack
         * @param layer Pointer to the layer to remove
         * @return true if the layer was found and removed
         * 
         * OnDetach() will be called before the layer is destroyed.
         */
        bool PopLayer(Layer* layer);

        /**
         * @brief Remove a layer by name
         * @param name Name of the layer to remove
         * @return true if a layer with that name was found and removed
         */
        bool PopLayer(const std::string& name);

        /**
         * @brief Remove all layers of a specific type
         * @param type Layer type to remove
         * @return Number of layers removed
         */
        size_t PopLayersByType(LayerType type);

        /**
         * @brief Clear all layers from the stack
         * OnDetach() will be called for all layers before they are destroyed.
         */
        void Clear();

        // ============================================================================
        // Layer Access and Queries
        // ============================================================================

        /**
         * @brief Find a layer by name
         * @param name Name of the layer to find
         * @return Pointer to the layer, or nullptr if not found
         */
        Layer* FindLayer(const std::string& name);
        const Layer* FindLayer(const std::string& name) const;

        /**
         * @brief Find a layer by type (returns first match)
         * @tparam T Layer type to find
         * @return Pointer to the layer, or nullptr if not found
         */
        template<typename T>
        T* FindLayer()
        {
            static_assert(std::is_base_of_v<Layer, T>, "T must inherit from Layer");
            auto it = std::find_if(m_Layers.begin(), m_Layers.end(),
                                  [](const auto& layer) { return dynamic_cast<T*>(layer.get()) != nullptr; });
            return (it != m_Layers.end()) ? static_cast<T*>(it->get()) : nullptr;
        }

        /**
         * @brief Find all layers of a specific type
         * @param type Layer type to find
         * @return Vector of pointers to matching layers
         */
        std::vector<Layer*> FindLayersByType(LayerType type);

        /**
         * @brief Get all layers (const access)
         */
        const LayerContainer& GetLayers() const { return m_Layers; }

        /**
         * @brief Get the number of layers in the stack
         */
        size_t GetLayerCount() const { return m_Layers.size(); }

        /**
         * @brief Check if the stack is empty
         */
        bool IsEmpty() const { return m_Layers.empty(); }

        // ============================================================================
        // Layer Processing
        // ============================================================================

        /**
         * @brief Update all enabled layers
         * 
         * Layers are updated in order (Game -> UI -> Debug -> Overlay)
         */
        void OnUpdate();

        /**
         * @brief Render all enabled layers
         * 
         * Layers are rendered in order (Game -> UI -> Debug -> Overlay)
         */
        void OnRender();

        /**
         * @brief Render ImGui for all enabled layers
         * 
         * Layers are rendered in order (Game -> UI -> Debug -> Overlay)
         */
        void OnImGuiRender();

        /**
         * @brief Handle an event
         * @param event The event to process
         * @return true if any layer handled the event
         * 
         * Events are processed in reverse order (Overlay -> Debug -> UI -> Game)
         * If a layer returns true or has BlockEvents enabled, processing stops.
         */
        bool OnEvent(Event& event);

        // ============================================================================
        // Iteration Support
        // ============================================================================

        // Forward iteration (Game -> UI -> Debug -> Overlay)
        iterator begin() { return m_Layers.begin(); }
        iterator end() { return m_Layers.end(); }
        const_iterator begin() const { return m_Layers.begin(); }
        const_iterator end() const { return m_Layers.end(); }
        const_iterator cbegin() const { return m_Layers.cbegin(); }
        const_iterator cend() const { return m_Layers.cend(); }

        // Reverse iteration (Overlay -> Debug -> UI -> Game)
        reverse_iterator rbegin() { return m_Layers.rbegin(); }
        reverse_iterator rend() { return m_Layers.rend(); }
        const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
        const_reverse_iterator rend() const { return m_Layers.rend(); }
        const_reverse_iterator crbegin() const { return m_Layers.crbegin(); }
        const_reverse_iterator crend() const { return m_Layers.crend(); }

        // ============================================================================
        // Debug and Statistics
        // ============================================================================

        /**
         * @brief Get detailed information about all layers
         */
        std::string GetDebugInfo() const;

        /**
         * @brief Get statistics about the layer stack
         */
        struct LayerStackStats
        {
            size_t TotalLayers = 0;
            size_t EnabledLayers = 0;
            size_t GameLayers = 0;
            size_t UILayers = 0;
            size_t DebugLayers = 0;
            size_t OverlayLayers = 0;
            size_t BlockingLayers = 0;
        };
        
        LayerStackStats GetStats() const;

        /**
         * @brief Validate the internal state of the layer stack
         * @return true if everything is valid, false otherwise
         * 
         * This is mainly for debugging and unit tests.
         */
        bool ValidateIntegrity() const;

    private:
        /**
         * @brief Sort layers by their order value
         * Called automatically after adding layers
         */
        void SortLayers();

        /**
         * @brief Find the insertion point for a layer with the given order
         */
        LayerContainer::iterator FindInsertionPoint(int32_t order);

    private:
        LayerContainer m_Layers;
        bool m_LayersNeedSorting = false;

        // Statistics for optimization
        mutable bool m_StatsCached = false;
        mutable LayerStackStats m_CachedStats;
    };
}
