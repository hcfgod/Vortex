#include "vxpch.h"
#include "LayerStack.h"
#include "Core/Debug/Log.h"

namespace Vortex
{
    LayerStack::LayerStack()
    {
        VX_CORE_TRACE("LayerStack created");
    }

    LayerStack::~LayerStack()
    {
        Clear();
        VX_CORE_TRACE("LayerStack destroyed");
    }

    // ============================================================================
    // Layer Management
    // ============================================================================

    Layer* LayerStack::PushLayer(std::unique_ptr<Layer> layer)
    {
        if (!layer)
        {
            VX_CORE_ERROR("Attempted to push null layer to LayerStack");
            return nullptr;
        }

        // Ensure unique names
        std::string uniqueName = CreateUniqueLayerName(layer->GetName(), m_Layers);
        if (uniqueName != layer->GetName())
        {
            VX_CORE_WARN("Layer name '{}' already exists, renamed to '{}'", 
                        layer->GetName(), uniqueName);
            layer->SetName(uniqueName);
        }

        Layer* layerPtr = layer.get();
        
        // Find the correct insertion point to maintain sorting
        auto insertPos = FindInsertionPoint(layer->GetOrder());
        m_Layers.insert(insertPos, std::move(layer));

        // Attach the layer
        try
        {
            layerPtr->OnAttach();
            VX_CORE_INFO("Layer '{}' attached to LayerStack", layerPtr->GetName());
        }
        catch (const std::exception& e)
        {
            VX_CORE_ERROR("Exception in Layer::OnAttach() for '{}': {}", 
                         layerPtr->GetName(), e.what());
        }

        // Invalidate cached statistics
        m_StatsCached = false;

        return layerPtr;
    }

    bool LayerStack::PopLayer(Layer* layer)
    {
        if (!layer)
        {
            return false;
        }

        auto it = std::find_if(m_Layers.begin(), m_Layers.end(),
                              [layer](const auto& ptr) { return ptr.get() == layer; });

        if (it == m_Layers.end())
        {
            VX_CORE_WARN("Attempted to remove layer '{}' that is not in the stack", 
                        layer->GetName());
            return false;
        }

        // Detach the layer before removing
        try
        {
            layer->OnDetach();
            VX_CORE_INFO("Layer '{}' detached from LayerStack", layer->GetName());
        }
        catch (const std::exception& e)
        {
            VX_CORE_ERROR("Exception in Layer::OnDetach() for '{}': {}", 
                         layer->GetName(), e.what());
        }

        m_Layers.erase(it);
        m_StatsCached = false;
        return true;
    }

    bool LayerStack::PopLayer(const std::string& name)
    {
        Layer* layer = FindLayer(name);
        return layer ? PopLayer(layer) : false;
    }

    size_t LayerStack::PopLayersByType(LayerType type)
    {
        size_t removedCount = 0;
        
        // Remove layers in reverse order to maintain iterator validity
        for (auto it = m_Layers.rbegin(); it != m_Layers.rend();)
        {
            if ((*it)->GetType() == type)
            {
                try
                {
                    (*it)->OnDetach();
                    VX_CORE_INFO("Layer '{}' detached from LayerStack", (*it)->GetName());
                }
                catch (const std::exception& e)
                {
                    VX_CORE_ERROR("Exception in Layer::OnDetach() for '{}': {}", 
                                 (*it)->GetName(), e.what());
                }

                // Convert reverse iterator to forward iterator for erase
                auto forwardIt = std::next(it).base();
                m_Layers.erase(forwardIt);
                ++removedCount;
                
                // Don't increment reverse iterator since we erased
            }
            else
            {
                ++it;
            }
        }

        if (removedCount > 0)
        {
            VX_CORE_INFO("Removed {} layers of type {}", removedCount, LayerTypeToString(type));
            m_StatsCached = false;
        }

        return removedCount;
    }

    void LayerStack::Clear()
    {
        // Detach all layers in reverse order (overlays first)
        for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
        {
            try
            {
                (*it)->OnDetach();
                VX_CORE_TRACE("Layer '{}' detached during clear", (*it)->GetName());
            }
            catch (const std::exception& e)
            {
                VX_CORE_ERROR("Exception in Layer::OnDetach() for '{}' during clear: {}", 
                             (*it)->GetName(), e.what());
            }
        }

        size_t layerCount = m_Layers.size();
        m_Layers.clear();
        m_StatsCached = false;

        if (layerCount > 0)
        {
            VX_CORE_INFO("Cleared {} layers from LayerStack", layerCount);
        }
    }

    // ============================================================================
    // Layer Access and Queries
    // ============================================================================

    Layer* LayerStack::FindLayer(const std::string& name)
    {
        auto it = std::find_if(m_Layers.begin(), m_Layers.end(),
                              [&name](const auto& layer) { return layer->GetName() == name; });
        return (it != m_Layers.end()) ? it->get() : nullptr;
    }

    const Layer* LayerStack::FindLayer(const std::string& name) const
    {
        auto it = std::find_if(m_Layers.begin(), m_Layers.end(),
                              [&name](const auto& layer) { return layer->GetName() == name; });
        return (it != m_Layers.end()) ? it->get() : nullptr;
    }

    std::vector<Layer*> LayerStack::FindLayersByType(LayerType type)
    {
        std::vector<Layer*> result;
        
        for (const auto& layer : m_Layers)
        {
            if (layer->GetType() == type)
            {
                result.push_back(layer.get());
            }
        }

        return result;
    }

    // ============================================================================
    // Layer Processing
    // ============================================================================

    void LayerStack::OnUpdate()
    {
        // Update layers in order (Game -> UI -> Debug -> Overlay)
        for (const auto& layer : m_Layers)
        {
            if (layer->IsEnabled())
            {
                try
                {
                    layer->OnUpdate();
                }
                catch (const std::exception& e)
                {
                    VX_CORE_ERROR("Exception in Layer::OnUpdate() for '{}': {}", 
                                 layer->GetName(), e.what());
                }
            }
        }
    }

    void LayerStack::OnRender()
    {
        // Render layers in order (Game -> UI -> Debug -> Overlay)
        for (const auto& layer : m_Layers)
        {
            if (layer->IsEnabled())
            {
                try
                {
                    layer->OnRender();
                }
                catch (const std::exception& e)
                {
                    VX_CORE_ERROR("Exception in Layer::OnRender() for '{}': {}", 
                                 layer->GetName(), e.what());
                }
            }
        }
    }

    void LayerStack::OnImGuiRender()
    {
        // Render ImGui for layers in order (Game -> UI -> Debug -> Overlay)
        for (const auto& layer : m_Layers)
        {
            if (layer->IsEnabled())
            {
                try
                {
                    layer->OnImGuiRender();
                }
                catch (const std::exception& e)
                {
                    VX_CORE_ERROR("Exception in Layer::OnImGuiRender() for '{}': {}", 
                                 layer->GetName(), e.what());
                }
            }
        }
    }

    bool LayerStack::OnEvent(Event& event)
    {
        // Process events in reverse order (Overlay -> Debug -> UI -> Game)
        for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
        {
            const auto& layer = *it;
            
            if (!layer->IsEnabled())
            {
                continue;
            }

            bool eventHandled = false;
            try
            {
                eventHandled = layer->OnEvent(event);
            }
            catch (const std::exception& e)
            {
                VX_CORE_ERROR("Exception in Layer::OnEvent() for '{}': {}", 
                             layer->GetName(), e.what());
            }

            // Stop propagation if the layer handled the event or blocks events
            if (eventHandled || layer->ShouldBlockEvents())
            {
                if (eventHandled)
                {
                    VX_CORE_TRACE("Event '{}' handled by layer '{}'", 
                                 event.GetName(), layer->GetName());
                }
                return true;
            }
        }

        return false;
    }

    // ============================================================================
    // Debug and Statistics
    // ============================================================================

    std::string LayerStack::GetDebugInfo() const
    {
        std::stringstream ss;
        ss << "LayerStack (" << m_Layers.size() << " layers):\n";

        for (size_t i = 0; i < m_Layers.size(); ++i)
        {
            ss << "  [" << i << "] " << m_Layers[i]->GetDebugInfo() << "\n";
        }

        auto stats = GetStats();
        ss << "Statistics:\n";
        ss << "  Total: " << stats.TotalLayers << ", Enabled: " << stats.EnabledLayers << "\n";
        ss << "  Game: " << stats.GameLayers << ", UI: " << stats.UILayers;
        ss << ", Debug: " << stats.DebugLayers << ", Overlay: " << stats.OverlayLayers << "\n";
        ss << "  Blocking Events: " << stats.BlockingLayers;

        return ss.str();
    }

    LayerStack::LayerStackStats LayerStack::GetStats() const
    {
        if (m_StatsCached)
        {
            return m_CachedStats;
        }

        LayerStackStats stats;
        stats.TotalLayers = m_Layers.size();

        for (const auto& layer : m_Layers)
        {
            if (layer->IsEnabled())
            {
                ++stats.EnabledLayers;
            }

            if (layer->ShouldBlockEvents())
            {
                ++stats.BlockingLayers;
            }

            switch (layer->GetType())
            {
                case LayerType::Game:       ++stats.GameLayers; break;
                case LayerType::UI:         ++stats.UILayers; break;
                case LayerType::Debug:      ++stats.DebugLayers; break;
                case LayerType::Overlay:    ++stats.OverlayLayers; break;
            }
        }

        m_CachedStats = stats;
        m_StatsCached = true;
        return stats;
    }

    bool LayerStack::ValidateIntegrity() const
    {
        // Check that layers are properly sorted by order
        for (size_t i = 1; i < m_Layers.size(); ++i)
        {
            if (m_Layers[i-1]->GetOrder() > m_Layers[i]->GetOrder())
            {
                VX_CORE_ERROR("LayerStack integrity violation: layers not properly sorted at indices {} and {}", 
                             i-1, i);
                return false;
            }
        }

        // Check for duplicate names
        std::unordered_set<std::string> names;
        for (const auto& layer : m_Layers)
        {
            if (!names.insert(layer->GetName()).second)
            {
                VX_CORE_ERROR("LayerStack integrity violation: duplicate layer name '{}'", 
                             layer->GetName());
                return false;
            }
        }

        return true;
    }

    // ============================================================================
    // Private Methods
    // ============================================================================

    void LayerStack::SortLayers()
    {
        std::sort(m_Layers.begin(), m_Layers.end(),
                 [](const auto& a, const auto& b) {
                     return a->GetOrder() < b->GetOrder();
                 });
        
        VX_CORE_TRACE("LayerStack sorted {} layers", m_Layers.size());
    }

    LayerStack::LayerContainer::iterator LayerStack::FindInsertionPoint(int32_t order)
    {
        // Find the first layer with a higher order value
        return std::lower_bound(m_Layers.begin(), m_Layers.end(), order,
                               [](const auto& layer, int32_t value) {
                                   return layer->GetOrder() < value;
                               });
    }
}