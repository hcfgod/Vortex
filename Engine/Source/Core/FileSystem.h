#pragma once

#include <filesystem>
#include <string>
#include <optional>

namespace Vortex
{
    /**
     * @brief Utility class for cross-platform filesystem operations
     * 
     * This class provides platform-specific filesystem operations, with fallbacks
     * when SDL is not available. It abstracts away platform differences for
     * common operations like getting the executable path.
     */
    class FileSystem
    {
    public:
        /**
         * @brief Get the directory containing the current executable
         * 
         * Uses SDL_GetBasePath() when VX_USE_SDL is defined, otherwise falls back
         * to platform-specific methods (GetModuleFileName on Windows, /proc/self/exe
         * on Linux, _NSGetExecutablePath on macOS).
         * 
         * @return std::optional<std::filesystem::path> The executable directory path,
         *         or std::nullopt if it could not be determined
         */
        static std::optional<std::filesystem::path> GetExecutableDirectory();

        /**
         * @brief Get the full path to the current executable
         * 
         * Similar to GetExecutableDirectory() but returns the full path to the
         * executable file itself, not just its containing directory.
         * 
         * @return std::optional<std::filesystem::path> The executable file path,
         *         or std::nullopt if it could not be determined
         */
        static std::optional<std::filesystem::path> GetExecutablePath();

        /**
         * @brief Find a configuration directory by searching common locations
         * 
         * Searches for a configuration directory relative to the executable path.
         * Tries multiple possible locations in order:
         * - executable_dir/Config/Engine
         * - executable_dir/../Config/Engine  
         * - executable_dir/../../Config/Engine
         * - executable_dir/../../../Config/Engine
         * 
         * @param configSubPath The subdirectory path to search for (e.g., "Config/Engine")
         * @return std::string The found configuration directory path, or the configSubPath
         *         as a fallback if no directory was found
         */
        static std::string FindConfigDirectory(const std::string& configSubPath = "Config/Engine");

        /**
         * @brief Check if a path exists and is a directory
         * 
         * @param path The path to check
         * @return bool True if the path exists and is a directory
         */
        static bool DirectoryExists(const std::filesystem::path& path);

        /**
         * @brief Check if a path exists and is a regular file
         * 
         * @param path The path to check
         * @return bool True if the path exists and is a regular file
         */
        static bool FileExists(const std::filesystem::path& path);

        /**
         * @brief Create a directory and all necessary parent directories
         * 
         * @param path The directory path to create
         * @return bool True if the directory was created or already exists
         */
        static bool CreateDirectories(const std::filesystem::path& path);

    private:
        // Platform-specific implementation helpers
        static std::optional<std::filesystem::path> GetExecutablePath_SDL();
        static std::optional<std::filesystem::path> GetExecutablePath_Native();
    };
}
