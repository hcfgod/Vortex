#include <doctest/doctest.h>
#include <Core/Common/UUID.h>

using namespace Vortex;

TEST_CASE("UUID generates non-zero unique values") {
    UUID a = UUID::Generate();
    UUID b = UUID::Generate();
    CHECK(static_cast<uint64_t>(a) != 0);
    CHECK(static_cast<uint64_t>(b) != 0);
    CHECK(a != b);
}

TEST_CASE("UUID ToString is 16 hex chars uppercase") {
    UUID id = UUID(0x0123456789ABCDEFULL);
    std::string s = id.ToString();
    CHECK(s.size() == 16);
    CHECK(s == std::string("0123456789ABCDEF"));
}
