#include "vxpch.h"
#include "Layer.h"
#include "Core/Debug/Log.h"

namespace Vortex
{
    Layer::Layer(const std::string& name, LayerType type, LayerPriority priority)
        : m_Name(name), m_Type(type), m_Priority(priority)
    {
        VX_CORE_TRACE("Layer '{}' created (Type: {}, Priority: {})", 
                     m_Name, LayerTypeToString(m_Type), LayerPriorityToString(m_Priority));
    }

    int32_t Layer::GetOrder() const
    {
        // Combine layer type and priority into a single sortable value
        // Type is the primary sort key (multiplied by large value)
        // Priority is the secondary sort key
        return (static_cast<int32_t>(m_Type) * 100000) + static_cast<int32_t>(m_Priority);
    }

    std::string Layer::GetDebugInfo() const
    {
        return fmt::format("Layer '{}' - Type: {}, Priority: {}, Order: {}, Enabled: {}, BlockEvents: {}",
                          m_Name, 
                          LayerTypeToString(m_Type), 
                          LayerPriorityToString(m_Priority),
                          GetOrder(),
                          m_Enabled ? "Yes" : "No",
                          m_BlockEvents ? "Yes" : "No");
    }

    // ============================================================================
    // Utility Functions
    // ============================================================================

    const char* LayerTypeToString(LayerType type)
    {
        switch (type)
        {
            case LayerType::Game:       return "Game";
            case LayerType::UI:         return "UI";
            case LayerType::Debug:      return "Debug";
            case LayerType::Overlay:    return "Overlay";
            default:                    return "Unknown";
        }
    }

    const char* LayerPriorityToString(LayerPriority priority)
    {
        switch (priority)
        {
            case LayerPriority::Lowest:     return "Lowest";
            case LayerPriority::VeryLow:    return "VeryLow";
            case LayerPriority::Low:        return "Low";
            case LayerPriority::Normal:     return "Normal";
            case LayerPriority::High:       return "High";
            case LayerPriority::VeryHigh:   return "VeryHigh";
            case LayerPriority::Highest:    return "Highest";
            default:                        return "Custom";
        }
    }

    std::string CreateUniqueLayerName(const std::string& baseName, 
                                     const std::vector<std::unique_ptr<Layer>>& existingLayers)
    {
        // Check if the base name is already unique
        auto nameExists = [&](const std::string& name) {
            return std::any_of(existingLayers.begin(), existingLayers.end(),
                              [&name](const auto& layer) { 
                                  return layer && layer->GetName() == name; 
                              });
        };

        if (!nameExists(baseName))
        {
            return baseName;
        }

        // Find the first available name with a numeric suffix
        for (int i = 1; i < 10000; ++i)  // Reasonable upper limit
        {
            std::string candidate = fmt::format("{}_{}", baseName, i);
            if (!nameExists(candidate))
            {
                return candidate;
            }
        }

        // Fallback with timestamp if we somehow exhaust numeric suffixes
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return fmt::format("{}_{}", baseName, timestamp);
    }
}