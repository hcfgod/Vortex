#pragma once
#include <string.h>

namespace Vortex
{
    /**
     * @brief Shared initialization utilities for all entry points
     * 
     * This class extracts common initialization logic that needs to happen
     * before the Engine or Application are created, eliminating duplication
     * between different entry point implementations.
     */
    class Bootstrap
    {
    public:
        /**
         * @brief Initialize configuration and logging systems
         * @param argc Command line argument count (optional)
         * @param argv Command line arguments (optional)
         * @return Directory where config files were found
         */
        static std::string InitializeEarlySubsystems(int argc = 0, char** argv = nullptr);

        /**
         * @brief Load all configuration layers in the correct priority order
         * @param configDir Directory containing config files
         * @return True if all critical configs loaded successfully
         */
        static bool LoadConfigurationLayers(const std::string& configDir);

        /**
         * @brief Initialize the logging system from configuration
         * @return True if logging was initialized successfully
         */
        static bool InitializeLoggingFromConfig();

    private:
        static bool s_Initialized;
        static std::string s_ConfigDirectory;
    };
}