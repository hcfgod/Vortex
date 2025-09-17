#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <cstdint>

namespace Vortex
{
    struct AssetPackEntry
    {
        std::string Path;      // normalized forward-slash relative path inside pack (e.g., "Textures/Checker.png")
        uint64_t     Offset = 0; // offset to data payload
        uint64_t     Size   = 0; // payload size in bytes
        uint32_t     Crc32  = 0; // reserved for future integrity checks
        uint8_t      Compression = 0; // 0 = none (reserved)
    };

    class AssetPackReader
    {
    public:
        AssetPackReader() = default;
        ~AssetPackReader() = default;

        bool Load(const std::filesystem::path& packFilePath);
        bool IsLoaded() const { return m_IsLoaded; }

        bool Exists(const std::string& relativePath) const;
        bool Read(const std::string& relativePath, std::vector<uint8_t>& outBytes) const;

        // Find the first entry whose filename (basename) matches filename (case-insensitive). Returns the pack key or empty string.
        std::string FindFirstByFilename(const std::string& filename) const;

        const std::filesystem::path& GetPath() const { return m_FilePath; }

    private:
        static std::string NormalizePathKey(const std::string& path);

        std::filesystem::path m_FilePath;
        bool m_IsLoaded = false;
        std::unordered_map<std::string, AssetPackEntry> m_Entries;
    };

    class AssetPackWriter
    {
    public:
        AssetPackWriter() = default;
        ~AssetPackWriter() = default;

        // Adds an entry from raw memory (copied into internal storage until WriteToFile)
        void AddEntry(const std::string& relativePath, const std::vector<uint8_t>& bytes, uint8_t compression = 0);

        // Adds an entry by reading a file from disk at diskPath
        bool AddFile(const std::string& relativePath, const std::filesystem::path& diskPath, uint8_t compression = 0);

        // Write the pack to disk
        bool WriteToFile(const std::filesystem::path& outPackPath) const;

    private:
        struct PendingEntry
        {
            std::string Path;
            std::vector<uint8_t> Data;
            uint8_t Compression = 0;
        };

        static std::string NormalizePathKey(const std::string& path);

        std::vector<PendingEntry> m_Pending;
    };
}


