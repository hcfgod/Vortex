#include "vxpch.h"
#include "FileSystem.h"

#ifdef VX_USE_SDL
    #include "SDL3/SDL.h"
#endif

// Platform-specific headers for executable path detection
#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__) || defined(__unix__)
    #include <unistd.h>
    #include <limits.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <limits.h>
#endif

namespace Vortex
{
    std::optional<std::filesystem::path> FileSystem::GetExecutablePath()
    {
        #ifdef VX_USE_SDL
            return GetExecutablePath_SDL();
        #else
            return GetExecutablePath_Native();
        #endif
    }

    std::optional<std::filesystem::path> FileSystem::GetExecutableDirectory()
    {
        #ifdef VX_USE_SDL
            // SDL_GetBasePath() directly returns the directory containing the executable
            const char* basePath = SDL_GetBasePath();
            if (basePath) 
            {
                return std::filesystem::path(basePath);
            }
            return std::nullopt;
        #else
            // For native implementations, get the full path and return the parent directory
            auto execPath = GetExecutablePath();
            if (execPath.has_value()) {
                return execPath.value().parent_path();
            }
            return std::nullopt;
        #endif
    }

    std::optional<std::filesystem::path> FileSystem::GetExecutablePath_SDL()
    {
        #ifdef VX_USE_SDL
            // SDL_GetBasePath returns the directory, not the full executable path
            // We cannot get the exact executable path from SDL alone
            // Fall back to native methods even when SDL is available
            return GetExecutablePath_Native();
        #endif
        return std::nullopt;
    }

    std::optional<std::filesystem::path> FileSystem::GetExecutablePath_Native()
    {
        #ifdef _WIN32
            // Windows: Use GetModuleFileName
            char buffer[MAX_PATH];
            DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
            if (length > 0 && length < MAX_PATH) {
                return std::filesystem::path(buffer);
            }
        #elif defined(__linux__) || defined(__unix__)
            // Linux/Unix: Read from /proc/self/exe
            char buffer[PATH_MAX];
            ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
            if (length != -1) {
                buffer[length] = '\0';
                return std::filesystem::path(buffer);
            }
        #elif defined(__APPLE__)
            // macOS: Use _NSGetExecutablePath
            char buffer[PATH_MAX];
            uint32_t size = sizeof(buffer);
            if (_NSGetExecutablePath(buffer, &size) == 0) {
                return std::filesystem::path(buffer);
            }
        #endif
        return std::nullopt;
    }

    std::string FileSystem::FindConfigDirectory(const std::string& configSubPath)
    {
        auto exeDir = GetExecutableDirectory();
        if (!exeDir.has_value()) {
            // If we can't get the executable directory, return the relative path as fallback
            return configSubPath;
        }

        std::filesystem::path executableDir = exeDir.value();
        
        // Convert configSubPath to a filesystem path for proper path joining
        std::filesystem::path configPath = std::filesystem::path(configSubPath);
        
        // Try multiple possible locations for config directory
        std::vector<std::filesystem::path> searchPaths = {
            executableDir / configPath,                                         // Same dir as exe
            executableDir.parent_path() / configPath,                          // Parent dir
            executableDir.parent_path().parent_path() / configPath,            // Grandparent
            executableDir.parent_path().parent_path().parent_path() / configPath // Great-grandparent
        };

        for (const auto& path : searchPaths) {
            if (DirectoryExists(path)) {
                return path.string();
            }
        }

        // Fallback to relative path
        return configSubPath;
    }

    bool FileSystem::DirectoryExists(const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
    }

    bool FileSystem::FileExists(const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
    }

    bool FileSystem::CreateDirectories(const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::create_directories(path, ec);
    }
}
