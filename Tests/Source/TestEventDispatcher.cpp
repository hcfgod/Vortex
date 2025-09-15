#include <doctest/doctest.h>
#include <Core/Events/EventDispatcher.h>
#include <Core/Events/ApplicationEvent.h>

using namespace Vortex;

TEST_CASE("EventDispatcher queues and dispatches events to subscribers")
{
	EventDispatcher dispatcher;
	int handledCount = 0;

	auto subId = dispatcher.Subscribe<ApplicationStartedEvent>([&](const ApplicationStartedEvent&){ handledCount++; return true; });

	dispatcher.QueueEvent(ApplicationStartedEvent{});

	CHECK(dispatcher.GetQueuedEventCount() == 1);
	CHECK(dispatcher.ProcessQueuedEvents() == 1);
	CHECK(handledCount == 1);

	// Unsubscribe and ensure no further handling
	CHECK(dispatcher.Unsubscribe(subId) == true);

	dispatcher.QueueEvent(ApplicationStartedEvent{});
	CHECK(dispatcher.ProcessQueuedEvents() == 1);
	CHECK(handledCount == 1);
}


