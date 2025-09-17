#include "vxpch.h"
#include "AssetPack.h"
#include "Core/Debug/Log.h"
#include <fstream>
#include <algorithm>

namespace Vortex
{
    // Simple binary pack format (little endian):
    // [4] magic 'VXPK' (0x56,0x58,0x50,0x4B)
    // [4] version (uint32_t) = 1
    // [8] entryCount (uint64_t)
    // For each entry:
    //   [2] pathLen (uint16_t)
    //   [pathLen] path bytes (utf8, normalized forward slash)
    //   [8] offset (uint64_t)
    //   [8] size (uint64_t)
    //   [4] crc32 (uint32_t) (reserved, zero for now)
    //   [1] compression (uint8_t) (0 = none)
    // Then contiguous payloads for each entry in the same order.

    static constexpr uint32_t kVXPKMagic = 0x4B505856; // 'VXPK' little-endian
    static constexpr uint32_t kVXPKVersion = 1;

    static void WriteU16(std::ofstream& o, uint16_t v) { o.write(reinterpret_cast<const char*>(&v), sizeof(v)); }
    static void WriteU32(std::ofstream& o, uint32_t v) { o.write(reinterpret_cast<const char*>(&v), sizeof(v)); }
    static void WriteU64(std::ofstream& o, uint64_t v) { o.write(reinterpret_cast<const char*>(&v), sizeof(v)); }
    static bool  ReadU16(std::ifstream& i, uint16_t& v) { return static_cast<bool>(i.read(reinterpret_cast<char*>(&v), sizeof(v))); }
    static bool  ReadU32(std::ifstream& i, uint32_t& v) { return static_cast<bool>(i.read(reinterpret_cast<char*>(&v), sizeof(v))); }
    static bool  ReadU64(std::ifstream& i, uint64_t& v) { return static_cast<bool>(i.read(reinterpret_cast<char*>(&v), sizeof(v))); }

    std::string AssetPackReader::NormalizePathKey(const std::string& path)
    {
        std::string key = path;
        std::replace(key.begin(), key.end(), '\\', '/');
        // trim leading './' or '/'
        while (!key.empty() && (key[0] == '/' || (key.size() >= 2 && key[0] == '.' && key[1] == '/')))
        {
            if (key[0] == '/') key.erase(key.begin());
            else key.erase(key.begin(), key.begin() + 2);
        }
        return key;
    }

    bool AssetPackReader::Load(const std::filesystem::path& packFilePath)
    {
        m_IsLoaded = false;
        m_FilePath = packFilePath;
        m_Entries.clear();

        std::ifstream in(packFilePath, std::ios::binary);
        if (!in.is_open())
        {
            VX_CORE_WARN("AssetPackReader: Failed to open pack '{}""'", packFilePath.string());
            return false;
        }

        uint32_t magic = 0, version = 0;
        uint64_t count = 0;
        if (!ReadU32(in, magic) || magic != kVXPKMagic || !ReadU32(in, version) || version != kVXPKVersion || !ReadU64(in, count))
        {
            VX_CORE_ERROR("AssetPackReader: Invalid pack header for '{}""'", packFilePath.string());
            return false;
        }

        for (uint64_t idx = 0; idx < count; ++idx)
        {
            uint16_t pathLen = 0;
            if (!ReadU16(in, pathLen)) return false;
            std::string path(pathLen, '\0');
            if (!in.read(path.data(), pathLen)) return false;

            AssetPackEntry e{};
            if (!ReadU64(in, e.Offset)) return false;
            if (!ReadU64(in, e.Size)) return false;
            if (!ReadU32(in, e.Crc32)) return false;
            char comp = 0;
            if (!in.read(&comp, 1)) return false;
            e.Compression = static_cast<uint8_t>(comp);
            e.Path = NormalizePathKey(path);
            m_Entries.emplace(e.Path, e);
        }

        m_IsLoaded = true;
        return true;
    }

    bool AssetPackReader::Exists(const std::string& relativePath) const
    {
        if (!m_IsLoaded) return false;
        std::string key = NormalizePathKey(relativePath);
        return m_Entries.find(key) != m_Entries.end();
    }

    bool AssetPackReader::Read(const std::string& relativePath, std::vector<uint8_t>& outBytes) const
    {
        outBytes.clear();
        if (!m_IsLoaded) return false;
        std::string key = NormalizePathKey(relativePath);
        auto it = m_Entries.find(key);
        if (it == m_Entries.end()) return false;

        std::ifstream in(m_FilePath, std::ios::binary);
        if (!in.is_open()) return false;

        const AssetPackEntry& e = it->second;
        outBytes.resize(static_cast<size_t>(e.Size));
        in.seekg(static_cast<std::streamoff>(e.Offset), std::ios::beg);
        if (!in.read(reinterpret_cast<char*>(outBytes.data()), static_cast<std::streamsize>(e.Size)))
            return false;
        return true;
    }

    std::string AssetPackReader::FindFirstByFilename(const std::string& filename) const
    {
        if (!m_IsLoaded) return {};
        auto toLower = [](std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); });
            return s;
        };
        std::string target = toLower(NormalizePathKey(filename));
        std::string targetFile = target;
        // strip directories if passed a path
        {
            auto pos = target.find_last_of('/');
            if (pos != std::string::npos)
                targetFile = target.substr(pos + 1);
        }
        for (const auto& kv : m_Entries)
        {
            const std::string& key = kv.first;
            auto pos = key.find_last_of('/');
            std::string base = (pos == std::string::npos) ? key : key.substr(pos + 1);
            if (toLower(base) == targetFile)
                return key;
        }
        return {};
    }

    // ----------------- Writer -----------------

    std::string AssetPackWriter::NormalizePathKey(const std::string& path)
    {
        std::string key = path;
        std::replace(key.begin(), key.end(), '\\', '/');
        while (!key.empty() && (key[0] == '/' || (key.size() >= 2 && key[0] == '.' && key[1] == '/')))
        {
            if (key[0] == '/') key.erase(key.begin());
            else key.erase(key.begin(), key.begin() + 2);
        }
        return key;
    }

    void AssetPackWriter::AddEntry(const std::string& relativePath, const std::vector<uint8_t>& bytes, uint8_t compression)
    {
        PendingEntry e;
        e.Path = NormalizePathKey(relativePath);
        e.Data = bytes; // copy
        e.Compression = compression;
        m_Pending.emplace_back(std::move(e));
    }

    bool AssetPackWriter::AddFile(const std::string& relativePath, const std::filesystem::path& diskPath, uint8_t compression)
    {
        std::ifstream in(diskPath, std::ios::binary);
        if (!in.is_open())
            return false;
        in.seekg(0, std::ios::end);
        std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);
        if (size <= 0)
            return false;
        std::vector<uint8_t> bytes(static_cast<size_t>(size));
        if (!in.read(reinterpret_cast<char*>(bytes.data()), size))
            return false;
        AddEntry(relativePath, bytes, compression);
        return true;
    }

    bool AssetPackWriter::WriteToFile(const std::filesystem::path& outPackPath) const
    {
        std::ofstream out(outPackPath, std::ios::binary);
        if (!out.is_open())
            return false;

        // Header
        WriteU32(out, kVXPKMagic);
        WriteU32(out, kVXPKVersion);
        WriteU64(out, static_cast<uint64_t>(m_Pending.size()));

        // We'll fill directory with correct offsets after we know header+dir size.
        // For simplicity, write directory with placeholder offsets and then rewrite.
        struct DirRec { uint64_t OffsetPos; uint64_t SizePos; };
        std::vector<DirRec> dirRecs;
        dirRecs.reserve(m_Pending.size());

        // Write directory
        for (const auto& e : m_Pending)
        {
            uint16_t pathLen = static_cast<uint16_t>(std::min<size_t>(e.Path.size(), 0xFFFF));
            WriteU16(out, pathLen);
            out.write(e.Path.data(), pathLen);
            DirRec rec{};
            rec.OffsetPos = static_cast<uint64_t>(out.tellp());
            WriteU64(out, 0ull); // offset placeholder
            rec.SizePos = static_cast<uint64_t>(out.tellp());
            WriteU64(out, static_cast<uint64_t>(e.Data.size())); // size
            WriteU32(out, 0u); // crc32 placeholder (0 for now)
            char comp = static_cast<char>(e.Compression);
            out.write(&comp, 1);
            dirRecs.emplace_back(rec);
        }

        // Write payloads and record offsets
        std::vector<uint64_t> offsets(m_Pending.size(), 0ull);
        for (size_t i = 0; i < m_Pending.size(); ++i)
        {
            offsets[i] = static_cast<uint64_t>(out.tellp());
            const auto& data = m_Pending[i].Data;
            if (!data.empty())
            {
                out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
            }
        }

        // Patch the offsets in directory
        for (size_t i = 0; i < dirRecs.size(); ++i)
        {
            out.seekp(static_cast<std::streamoff>(dirRecs[i].OffsetPos), std::ios::beg);
            WriteU64(out, offsets[i]);
        }

        return true;
    }
}


