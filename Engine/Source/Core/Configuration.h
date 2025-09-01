#pragma once

// If your project uses a precompiled header, include it first
#ifdef __has_include
#  if __has_include("vxpch.h")
#    include "vxpch.h"
#  endif
#endif

#include <nlohmann/json.hpp>
#include <filesystem>
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
#include <shared_mutex>
#include <algorithm>
#include <spdlog/spdlog.h>

#ifndef VX_CORE_INFO
#define VX_CORE_INFO(...) (void)0
#define VX_CORE_WARN(...) (void)0
#define VX_CORE_ERROR(...) (void)0
#endif

namespace Vortex
{
    // AAA-grade layered configuration system powered by nlohmann::json
    // - Multiple named layers with priorities (higher priority overrides lower)
    // - Thread-safe reads/writes
    // - Load/save layers from/to files
    // - Dotted-path get/set helpers
    // - Change detection for file-backed layers (manual polling)
    class Configuration
    {
    public:
        using Json = nlohmann::json;

        struct Layer
        {
            std::string Name;
            int Priority = 0; // Higher number means higher precedence
            Json Data;
        };

        static Configuration& Get();

        // Layer management
        bool AddOrReplaceLayer(const std::string& name, int priority);
        bool RemoveLayer(const std::string& name);
        void Clear();

        // File I/O for layers
        bool LoadLayerFromFile(const std::filesystem::path& path,
            const std::string& layerName,
            int priority,
            std::string* outError = nullptr,
            bool trackForReload = true);

        bool SaveLayerToFile(const std::filesystem::path& path,
            const std::string& layerName,
            std::string* outError = nullptr) const;

        // Dotted-path accessors on the MERGED view of all layers
        Json Get(const std::string& dottedPath) const;
        Json GetOr(const std::string& dottedPath, const Json& defaultValue) const;

        template<typename T>
        T GetAs(const std::string& dottedPath, const T& defaultValue) const
        {
            try
            {
                auto v = Get(dottedPath);
                if (v.is_null())
                    return defaultValue;
                return v.get<T>();
            }
            catch (...)
            {
                return defaultValue;
            }
        }

        // Set into a specific layer (default: RuntimeOverrides)
        bool Set(const std::string& dottedPath,
            const Json& value,
            const std::string& layerName = "RuntimeOverrides",
            bool createLayerIfMissing = true,
            int layerPriority = 1000);

        bool Has(const std::string& dottedPath) const;

        // Merge all layers to a single Json (higher priority overrides lower)
        Json GetMerged() const;

        // Reload any file-backed layers that changed on disk
        bool ReloadChangedFiles(std::vector<std::string>* outReloadedLayers = nullptr);

    private:
        Configuration() = default;

        static void MergeInto(Json& dst, const Json& src);
        static Json* EnsurePath(Json& root, const std::string& dottedPath);
        static const Json* ResolvePath(const Json& root, const std::string& dottedPath);

    private:
        mutable std::shared_mutex m_Mutex;

        // Name -> layer
        std::unordered_map<std::string, Layer> m_Layers;
        // Priority -> ordered layer names (for stable merging)
        std::map<int, std::vector<std::string>> m_PriorityToLayerNames;

        struct TrackedFile
        {
            std::filesystem::path Path;
            std::filesystem::file_time_type LastWriteTime{};
        };
        // LayerName -> tracked file
        std::unordered_map<std::string, TrackedFile> m_TrackedFiles;
    };
}

// Template specialization for spdlog::level::level_enum (implemented in .cpp)
template<>
spdlog::level::level_enum Vortex::Configuration::GetAs<spdlog::level::level_enum>(const std::string& dottedPath, const spdlog::level::level_enum& defaultValue) const;

