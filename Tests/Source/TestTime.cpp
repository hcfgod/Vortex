#include <doctest/doctest.h>
#include <Engine/Time/Time.h>
#include <thread>

using namespace Vortex;

TEST_CASE("Time initializes and starts at zero")
{
	Time::Initialize();
	CHECK(Time::GetTime() == doctest::Approx(0.0f));
	CHECK(Time::GetUnscaledTime() == doctest::Approx(0.0f));
	CHECK(Time::GetDeltaTime() == doctest::Approx(0.0f));
	CHECK(Time::GetUnscaledDeltaTime() == doctest::Approx(0.0f));
	CHECK(Time::GetFrameCount() == 0);
	Time::Shutdown();
}

TEST_CASE("Time Update advances time and frame count")
{
	Time::Initialize();
	Time::SetTimeScale(1.0f);
	auto startFrames = Time::GetFrameCount();
	Time::Update();
	CHECK(Time::GetFrameCount() == startFrames + 1);
	CHECK(Time::GetUnscaledDeltaTime() >= 0.0f);
	CHECK(Time::GetDeltaTime() >= 0.0f);
	CHECK(Time::GetUnscaledTime() >= Time::GetUnscaledDeltaTime());
	Time::Shutdown();
}

TEST_CASE("TimeScale affects DeltaTime but not UnscaledDeltaTime")
{
	Time::Initialize();
	Time::SetTimeScale(0.5f);
	Time::Update();
	float unscaled = Time::GetUnscaledDeltaTime();
	float scaled = Time::GetDeltaTime();
	CHECK(scaled == doctest::Approx(unscaled * 0.5f).epsilon(0.2f));
	Time::SetTimeScale(2.0f);
	Time::Update();
	unscaled = Time::GetUnscaledDeltaTime();
	scaled = Time::GetDeltaTime();
	CHECK(scaled == doctest::Approx(unscaled * 2.0f).epsilon(0.2f));
	Time::Shutdown();
}

TEST_CASE("MaxDeltaTime clamps large frame gaps")
{
	Time::Initialize();
	Time::SetTimeScale(1.0f);
	Time::SetMaxDeltaTime(0.05f); // 50 ms
	// Simulate a sleep longer than max delta
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	Time::Update();
	CHECK(Time::GetUnscaledDeltaTime() <= 0.051f);
	CHECK(Time::GetDeltaTime() <= 0.051f);
	Time::Shutdown();
}

TEST_CASE("High-resolution timestamp and conversion are sane")
{
	auto t0 = Time::GetHighResolutionTimestamp();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	auto t1 = Time::GetHighResolutionTimestamp();
	CHECK(t1 > t0);
	double sec = Time::NanosecondsToSeconds(static_cast<uint64_t>(1'000'000));
	CHECK(sec == doctest::Approx(0.001));
}
