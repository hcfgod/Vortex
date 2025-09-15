#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <Vortex.h>
#include <Core/Common/UUID.h>

// Initialize Vortex logging early for tests to avoid null logger dereferences
namespace {
struct VortexTestInit {
    VortexTestInit() {
        Vortex::LogConfig cfg;
        cfg.EnableAsync = false;
        cfg.EnableConsole = false;
        cfg.EnableFileLogging = false;
        cfg.LogLevel = spdlog::level::off;
        Vortex::Log::Init(cfg);
    }
    ~VortexTestInit() {
        Vortex::Log::Shutdown();
    }
} s_VortexTestInit;
}

TEST_CASE("Sanity: 1 + 1 == 2") {
    CHECK(1 + 1 == 2);
}
