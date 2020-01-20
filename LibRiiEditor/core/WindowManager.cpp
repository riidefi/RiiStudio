#include "WindowManager.hpp"

#include <mutex>
#include <queue>

struct WindowVector
{
	// Actions themselves not atomic, lock the mutex before using
	std::mutex mutex;
	std::vector<std::unique_ptr<Window>> vector;

	void append(std::unique_ptr<Window> ptr)
	{
		vector.push_back(std::move(ptr));
	}

	void remove(u32 id)
	{
		auto it = std::find_if(std::begin(vector), std::end(vector),
			[id](const auto& element) { return element->mId == id; });

		assert(it != vector.end());
		if (it != vector.end())
			vector.erase(it);
	}
};

class WindowQueue
{
public:
	enum class Action
	{
		AttachWindow,
		DetachWindow
	};
private:
	class Command
	{
	public:
		Command(Action a, std::unique_ptr<Window> t)
			: action(a), target(std::move(t))
		{}
		Command(Action a, u32 windowId_)
			: action(a), target(windowId_)
		{}

		Action getAction() const noexcept
		{
			return action;
		}

		std::unique_ptr<Window> getAttachmentTarget()
		{
			assert(action == Action::AttachWindow);

			return std::move(std::get<std::unique_ptr<Window>>(target));
		}

		u32 getDetachmentTarget()
		{
			assert(action == Action::DetachWindow);

			return std::get<u32>(target);
		}

	private:
		Action action;
		std::variant<std::unique_ptr<Window>, u32> target;
	};

public:
	std::mutex mutex;
	std::queue<Command> queue;
};

struct WindowManagerInternal
{
	WindowQueue mQueue;
	WindowVector mVect;
};

WindowManager::WindowManager()
{
	intern = new WindowManagerInternal();
}
WindowManager::~WindowManager()
{
	delete intern;
}

void WindowManager::attachWindow(std::unique_ptr<Window> window)
{
	std::lock_guard<std::mutex> guard(intern->mQueue.mutex);
	DebugReport("Enqueing window attachment.\n");
	intern->mQueue.queue.emplace(WindowQueue::Action::AttachWindow, std::move(window));
}

void WindowManager::detachWindow(u32 windowId)
{
	std::lock_guard<std::mutex> guard(intern->mQueue.mutex);
	DebugReport("Enqueing window detachment.\n");
	intern->mQueue.queue.emplace(WindowQueue::Action::DetachWindow, windowId);
}
u32 WindowManager::getWindowIndexById(u32 id) const
{
	std::lock_guard<std::mutex> guard(intern->mVect.mutex);

	auto end = std::find_if(intern->mVect.vector.begin(), intern->mVect.vector.end(), [&](auto& x) {
		return x->mId == id;
		});
	if (end == intern->mVect.vector.end())
		return -1;
	return end - intern->mVect.vector.begin();
}
void WindowManager::processWindowQueue()
{
	std::lock_guard<std::mutex> queue_guard(intern->mQueue.mutex);
	std::lock_guard<std::mutex> window_guard(intern->mVect.mutex);

	while (!intern->mQueue.queue.empty())
	{
		switch (intern->mQueue.queue.front().getAction())
		{
		case WindowQueue::Action::AttachWindow: {
			DebugReport("Attaching window\n");

			auto target = intern->mQueue.queue.front().getAttachmentTarget();

			// Assign it an ID
			target->mId = mWindowIdCounter++;
			mActive = target->mId;
			intern->mVect.append(std::move(target));
			break;
		}
		case WindowQueue::Action::DetachWindow:
			DebugReport("Detaching window.\n");

			intern->mVect.remove(intern->mQueue.queue.front().getDetachmentTarget());
			break;
		}

		intern->mQueue.queue.pop();
	}
}

void WindowManager::drawWindows(WindowContext* ctx)
{
	std::lock_guard<std::mutex> guard(intern->mVect.mutex);

	for (auto& window : intern->mVect.vector)
	{
		if (!window.get())
			continue;

		if (!bOnlyDrawActive || window->mId == mActive)
			window->draw(ctx);

		if (!window->bOpen)
			detachWindow(window->mId);
	}
}

Window& WindowManager::getWindowIndexed(u32 idx)
{
	std::lock_guard<std::mutex> g(intern->mVect.mutex);

	return *intern->mVect.vector[idx].get();
}

u32 WindowManager::getNumWindows() const
{
	std::lock_guard<std::mutex> g(intern->mVect.mutex);

	return intern->mVect.vector.size();
}
