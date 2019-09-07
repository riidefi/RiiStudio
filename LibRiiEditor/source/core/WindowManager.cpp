#include "WindowManager.hpp"

#include <mutex>
#include <queue>

void WindowManager::attachWindow(std::unique_ptr<Window> window)
{
	std::lock_guard<std::mutex> guard(mWindowQueue.mutex);
	DebugReport("Enqueing window attachment.\n");
	mWindowQueue.queue.emplace(WindowQueue::Command::Action::AttachWindow, std::move(window));
}

void WindowManager::detachWindow(u32 windowId)
{
	std::lock_guard<std::mutex> guard(mWindowQueue.mutex);
	DebugReport("Enqueing window detachment.\n");
	mWindowQueue.queue.emplace(WindowQueue::Action::DetachWindow, windowId);
}

void WindowManager::processWindowQueue()
{
	std::lock_guard<std::mutex> queue_guard(mWindowQueue.mutex);
	std::lock_guard<std::mutex> window_guard(mWindows.mutex);

	while (!mWindowQueue.queue.empty())
	{
		switch (mWindowQueue.queue.front().getAction())
		{
		case WindowQueue::Action::AttachWindow: {
			DebugReport("Attaching window\n");

			auto target = mWindowQueue.queue.front().getAttachmentTarget();

			// Assign it an ID
			target->mId = mWindowIdCounter++;
			mWindows.append(std::move(target));
			break;
		}
		case WindowQueue::Action::DetachWindow:
			DebugReport("Detaching window.\n");

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