#include <doctest/doctest.h>
#include <Engine/Input/KeyCodes.h>
#include <Engine/Input/MouseCodes.h>

using namespace Vortex;

TEST_CASE("KeyCodes and MouseCodes basic values are stable")
{
	// These checks assert that some common codes map to expected numeric values
	// Adjust if the enum changes intentionally.
	CHECK(static_cast<int>(KeyCode::Space) >= 0);
	CHECK(static_cast<int>(KeyCode::Enter) >= 0);
	CHECK(static_cast<int>(MouseCode::ButtonLeft) == 0);
	CHECK(static_cast<int>(MouseCode::ButtonRight) == 1);
}
