#include "WindowManager.hpp"

#include <mutex>
#include <queue>

void WindowManager::attachWindow(std::unique_ptr<Window> window)
{
	std::lock_guard<std::mutex> guard(mWindowQueue.mutex);
	printf("Enqueing window attachment.\n");
	mWindowQueue.queue.push(WindowQueueCommand(WindowQueueCommand::Action::AttachWindow, std::move(window)));
}

void WindowManager::detachWindow(u32 windowId)
{
	std::lock_guard<std::mutex> guard(mWindowQueue.mutex);
	printf("Enqueing window detachment.\n");
	mWindowQueue.queue.push(WindowQueueCommand(WindowQueueCommand::Action::DetachWindow, windowId));
}

void WindowManager::processWindowQueue()
{
	std::lock_guard<std::mutex> queue_guard(mWindowQueue.mutex);
	std::lock_guard<std::mutex> window_guard(mWindows.mutex);

	while (!mWindowQueue.queue.empty())
	{
		switch (mWindowQueue.queue.front().getAction())
		{
		case WindowQueueCommand::Action::AttachWindow:
			printf("Attaching window\n");
			// Assign it an ID
			mWindowQueue.queue.front().target_a->mId = mWindowIdCounter++;
			mWindows.append(std::move(mWindowQueue.queue.front().target_a));
			break;
		case WindowQueueCommand::Action::DetachWindow:
			printf("Detaching window.\n");
			mWindows.remove(mWindowQueue.queue.front().getDetachmentTarget());
			break;
		}

		mWindowQueue.queue.pop();
	}
}

void WindowManager::drawWindows(WindowContext* ctx)
{
	std::lock_guard<std::mutex> guard(mWindows.mutex);

	for (auto& window : mWindows.vector)
	{
		if (!window.get())
			continue;

		window->draw(ctx);
		if (!window->bOpen)
			detachWindow(window->mId);
	}
}