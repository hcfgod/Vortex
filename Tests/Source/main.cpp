#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <Vortex.h>
#include <Core/Common/UUID.h>

TEST_CASE("Sanity: 1 + 1 == 2") {
    CHECK(1 + 1 == 2);
}
