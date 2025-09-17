#include "vxpch.h"
#include "Core/Configuration.h"
#include "Core/Debug/Log.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <tuple>

namespace Vortex
{
    Configuration& Configuration::Get()
    {
        static Configuration s_Instance;
        return s_Instance;
    }

    bool Configuration::AddOrReplaceLayer(const std::string& name, int priority)
    {
        std::unique_lock lock(m_Mutex);

        auto it = m_Layers.find(name);
        if (it != m_Layers.end())
        {
            // If priority changes, update index
            if (it->second.Priority != priority)
            {
                // Remove from old priority bucket
                auto pit = m_PriorityToLayerNames.find(it->second.Priority);
                if (pit != m_PriorityToLayerNames.end())
                {
                    auto& vec = pit->second;
                    vec.erase(std::remove(vec.begin(), vec.end(), name), vec.end());
                    if (vec.empty()) m_PriorityToLayerNames.erase(pit);
                }
                // Add to new bucket
                m_PriorityToLayerNames[priority].push_back(name);
                it->second.Priority = priority;
            }
            return false; // existed
        }

        Layer layer;
        layer.Name = name;
        layer.Priority = priority;
        m_Layers.emplace(name, std::move(layer));
        m_PriorityToLayerNames[priority].push_back(name);
        return true; // newly added
    }

    bool Configuration::RemoveLayer(const std::string& name)
    {
        std::unique_lock lock(m_Mutex);
        auto it = m_Layers.find(name);
        if (it == m_Layers.end())
            return false;

        // Remove from priority index
        auto pit = m_PriorityToLayerNames.find(it->second.Priority);
        if (pit != m_PriorityToLayerNames.end())
        {
            auto& vec = pit->second;
            vec.erase(std::remove(vec.begin(), vec.end(), name), vec.end());
            if (vec.empty()) m_PriorityToLayerNames.erase(pit);
        }
        // Remove tracked file (if any)
        m_TrackedFiles.erase(name);

        m_Layers.erase(it);
        return true;
    }

    void Configuration::Clear()
    {
        std::unique_lock lock(m_Mutex);
        m_Layers.clear();
        m_PriorityToLayerNames.clear();
        m_TrackedFiles.clear();
    }

    bool Configuration::LoadLayerFromFile(const std::filesystem::path& path, const std::string& layerName, int priority, std::string* outError, bool trackForReload)
    {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
        {
            if (outError) *outError = "File does not exist: " + path.string();
            return false;
        }

        std::ifstream ifs(path, std::ios::binary);
        if (!ifs)
        {
            if (outError) *outError = "Failed to open file: " + path.string();
            return false;
        }

        try
        {
            Json data;
            ifs >> data;

            // Precompute last write time outside lock
            auto lastWrite = std::filesystem::last_write_time(path, ec);

            std::unique_lock lock(m_Mutex);

            // Create or update layer without re-entering locks
            auto it = m_Layers.find(layerName);
            if (it == m_Layers.end())
            {
                Layer layer;
                layer.Name = layerName;
                layer.Priority = priority;
                layer.Data = std::move(data);
                m_Layers.emplace(layerName, std::move(layer));
                m_PriorityToLayerNames[priority].push_back(layerName);
            }
            else
            {
                if (it->second.Priority != priority)
                {
                    // Move layer to new priority bucket
                    auto pit = m_PriorityToLayerNames.find(it->second.Priority);
                    if (pit != m_PriorityToLayerNames.end())
                    {
                        auto& vec = pit->second;
                        vec.erase(std::remove(vec.begin(), vec.end(), layerName), vec.end());
                        if (vec.empty()) m_PriorityToLayerNames.erase(pit);
                    }
                    m_PriorityToLayerNames[priority].push_back(layerName);
                    it->second.Priority = priority;
                }
                it->second.Data = std::move(data);
            }

            if (trackForReload)
            {
                TrackedFile tf;
                tf.Path = path;
                tf.LastWriteTime = lastWrite;
                m_TrackedFiles[layerName] = std::move(tf);
            }
            else
            {
                // If not tracking, ensure it's not in the tracked set
                m_TrackedFiles.erase(layerName);
            }
        }
        catch (const std::exception& e)
        {
            if (outError) *outError = std::string("JSON parse error: ") + e.what();
            return false;
        }
        return true;
    }

    bool Configuration::SaveLayerToFile(const std::filesystem::path& path, const std::string& layerName, std::string* outError) const
    {
        std::shared_lock lock(m_Mutex);
        auto it = m_Layers.find(layerName);
        if (it == m_Layers.end())
        {
            if (outError) *outError = "Layer not found: " + layerName;
            return false;
        }

        std::ofstream ofs(path, std::ios::binary);
        if (!ofs)
        {
            if (outError) *outError = "Failed to open file for write: " + path.string();
            return false;
        }

        try
        {
            ofs << it->second.Data.dump(4);
        }
        catch (const std::exception& e)
        {
            if (outError) *outError = std::string("JSON serialize error: ") + e.what();
            return false;
        }
        return true;
    }

    void Configuration::MergeInto(Json& dst, const Json& src)
    {
        // If source is null or not an object, skip merging (don't overwrite)
        if (src.is_null() || !src.is_object())
        {
            return;
        }
        
        // If destination is not an object, initialize it as an empty object
        if (!dst.is_object())
        {
            dst = Json::object();
        }
        
        for (auto it = src.begin(); it != src.end(); ++it)
        {
            const auto& key = it.key();
            const auto& val = it.value();
            if (dst.contains(key))
            {
                if (dst[key].is_object() && val.is_object())
                {
                    MergeInto(dst[key], val);
                }
                else
                {
                    dst[key] = val; // override
                }
            }
            else
            {
                dst[key] = val;
            }
        }
    }

    Configuration::Json Configuration::GetMerged() const
    {
        std::shared_lock lock(m_Mutex);
        Json merged = Json::object();
        for (const auto& [priority, names] : m_PriorityToLayerNames)
        {
            for (const auto& name : names)
            {
                auto it = m_Layers.find(name);
                if (it != m_Layers.end())
                {
                    MergeInto(merged, it->second.Data);
                }
            }
        }
        return merged;
    }

    const Configuration::Json* Configuration::ResolvePath(const Json& root, const std::string& dottedPath)
    {
        if (dottedPath.empty()) return &root;
        const Json* cur = &root;
        size_t start = 0;
        while (start <= dottedPath.size())
        {
            size_t dot = dottedPath.find('.', start);
            std::string key = dot == std::string::npos ? dottedPath.substr(start) : dottedPath.substr(start, dot - start);
            if (!cur->is_object()) return nullptr;
            auto it = cur->find(key);
            if (it == cur->end()) return nullptr;
            cur = &(*it);
            if (dot == std::string::npos) break;
            start = dot + 1;
        }
        return cur;
    }

    Configuration::Json* Configuration::EnsurePath(Json& root, const std::string& dottedPath)
    {
        if (dottedPath.empty()) return &root;
        Json* cur = &root;
        size_t start = 0;
        while (start <= dottedPath.size())
        {
            size_t dot = dottedPath.find('.', start);
            std::string key = dot == std::string::npos ? dottedPath.substr(start) : dottedPath.substr(start, dot - start);
            if (!cur->is_object())
                *cur = Json::object();
            cur = &((*cur)[key]);
            if (dot == std::string::npos) break;
            start = dot + 1;
        }
        return cur;
    }

    Configuration::Json Configuration::Get(const std::string& dottedPath) const
    {
        auto merged = GetMerged();
        auto* ptr = ResolvePath(merged, dottedPath);
        return ptr ? *ptr : Json();
    }

    Configuration::Json Configuration::GetOr(const std::string& dottedPath, const Json& defaultValue) const
    {
        auto v = Get(dottedPath);
        if (v.is_null()) return defaultValue;
        return v;
    }

    bool Configuration::Set(const std::string& dottedPath, const Json& value, const std::string& layerName, bool createLayerIfMissing, int layerPriority)
    {
        std::unique_lock lock(m_Mutex);
        auto it = m_Layers.find(layerName);
        if (it == m_Layers.end())
        {
            if (!createLayerIfMissing)
                return false;
            // Create the layer in-place without calling into locking APIs
            Layer layer;
            layer.Name = layerName;
            layer.Priority = layerPriority;
            layer.Data = Json::object();
            m_Layers.emplace(layerName, std::move(layer));
            m_PriorityToLayerNames[layerPriority].push_back(layerName);
            it = m_Layers.find(layerName);
        }
        auto* target = EnsurePath(it->second.Data, dottedPath);
        if (!target) return false;
        *target = value;
        return true;
    }

    bool Configuration::Has(const std::string& dottedPath) const
    {
        auto merged = GetMerged();
        return ResolvePath(merged, dottedPath) != nullptr;
    }

    bool Configuration::ReloadChangedFiles(std::vector<std::string>* outReloadedLayers)
    {
        // Collect layers to reload and their priorities under a shared lock
        std::vector<std::tuple<std::string, std::filesystem::path, int>> toReload;
        {
            std::shared_lock lock(m_Mutex);
            for (const auto& [layer, tf] : m_TrackedFiles)
            {
                std::error_code ec;
                if (std::filesystem::exists(tf.Path, ec))
                {
                    auto cur = std::filesystem::last_write_time(tf.Path, ec);
                    if (!ec && cur > tf.LastWriteTime)
                    {
                        // Capture current priority safely under lock
                        int prio = 0;
                        auto it = m_Layers.find(layer);
                        if (it != m_Layers.end()) prio = it->second.Priority;
                        toReload.emplace_back(layer, tf.Path, prio);
                    }
                }
            }
        }

        bool any = false;
        for (const auto& item : toReload)
        {
            const auto& layerName = std::get<0>(item);
            const auto& path = std::get<1>(item);
            const int prio = std::get<2>(item);

            std::string err;
            if (LoadLayerFromFile(path, layerName, prio, &err, true))
            {
                any = true;
                if (outReloadedLayers) outReloadedLayers->push_back(layerName);
                VX_CORE_INFO("Configuration: Reloaded layer '%s' from '%s'", layerName.c_str(), path.string().c_str());
            }
            else
            {
                VX_CORE_WARN("Configuration: Failed to reload layer '%s': %s", layerName.c_str(), err.c_str());
            }
        }
        return any;
    }

    // Template specialization implementation for spdlog::level::level_enum
    template<>
    spdlog::level::level_enum Configuration::GetAs<spdlog::level::level_enum>(const std::string& dottedPath, const spdlog::level::level_enum& defaultValue) const
    {
        try
        {
            auto v = Get(dottedPath);
            if (v.is_null() || !v.is_string())
                return defaultValue;
            
            std::string levelStr = v.get<std::string>();
            // Convert to lowercase for case-insensitive matching
            std::transform(levelStr.begin(), levelStr.end(), levelStr.begin(), ::tolower);
            
            if (levelStr == "trace")         return spdlog::level::trace;
            else if (levelStr == "debug")    return spdlog::level::debug;
            else if (levelStr == "info")     return spdlog::level::info;
            else if (levelStr == "warn")     return spdlog::level::warn;
            else if (levelStr == "error")    return spdlog::level::err;
            else if (levelStr == "critical") return spdlog::level::critical;
            else if (levelStr == "off")      return spdlog::level::off;
            else                             return defaultValue;
        }
        catch (...)
        {
            return defaultValue;
        }
    }
}