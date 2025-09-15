#include <doctest/doctest.h>
#include <Core/Async/Task.h>
#include <Core/Async/Coroutine.h>
#include <Core/Async/CoroutineScheduler.h>
#include <thread>

using namespace Vortex;

static Task<int> AddAsync(int a, int b)
{
	co_return a + b;
}

TEST_CASE("Task completes and returns value")
{
	// Initialize a scheduler on this thread
	CoroutineScheduler scheduler;
	CHECK(scheduler.Initialize().IsSuccess());
	CoroutineScheduler::SetGlobal(&scheduler);

	Task<int> t = AddAsync(2, 3);
	int result = t.GetResult();
	CHECK(result == 5);

	// Shutdown scheduler
	CHECK(scheduler.Shutdown().IsSuccess());
}


