#include "WindowManager.hpp"

#include <mutex>
#include <queue>
#include <variant>

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
    : mInternal(std::make_unique<WindowManagerInternal>())
{}
WindowManager::~WindowManager()
{}

// TODO: Thread safety
static u32 gWindowIdCounter = 1;

u32 WindowManager::attachWindow(std::unique_ptr<Window> window)
{
	std::lock_guard<std::mutex> guard(mInternal->mQueue.mutex);
	DebugReport("Enqueing window attachment.\n");

	const auto new_id = gWindowIdCounter++;
	window->mId = new_id;
	window->parentId = mId;

	mInternal->mQueue.queue.emplace(WindowQueue::Action::AttachWindow, std::move(window));

	return new_id;
}

void WindowManager::detachWindow(u32 windowId)
{
	std::lock_guard<std::mutex> guard(mInternal->mQueue.mutex);
	DebugReport("Enqueing window detachment.\n");
	mInternal->mQueue.queue.emplace(WindowQueue::Action::DetachWindow, windowId);
}
u32 WindowManager::getWindowIndexById(u32 id) const
{
	std::lock_guard<std::mutex> guard(mInternal->mVect.mutex);

	auto end = std::find_if(mInternal->mVect.vector.begin(), mInternal->mVect.vector.end(), [&](auto& x) {
		return x->mId == id;
		});
	if (end == mInternal->mVect.vector.end())
		return -1;
	return static_cast<u32>(end - mInternal->mVect.vector.begin());
}
void WindowManager::processWindowQueue()
{
	std::lock_guard<std::mutex> queue_guard(mInternal->mQueue.mutex);
	std::lock_guard<std::mutex> window_guard(mInternal->mVect.mutex);

	while (!mInternal->mQueue.queue.empty())
	{
		switch (mInternal->mQueue.queue.front().getAction())
		{
		case WindowQueue::Action::AttachWindow: {
			DebugReport("Attaching window\n");

			auto target = mInternal->mQueue.queue.front().getAttachmentTarget();
			if (!target) { DebugReport("Fail\n"); break; }
			
			mInternal->mVect.append(std::move(target));
			break;
		}
		case WindowQueue::Action::DetachWindow:
			DebugReport("Detaching window.\n");

			mInternal->mVect.remove(mInternal->mQueue.queue.front().getDetachmentTarget());
			break;
		}

		mInternal->mQueue.queue.pop();
	}
}

// We could definitely implement a window stack.
// For now, only immediate parent is sent..
void WindowManager::drawChildren(Window* ctx) noexcept
{
	std::lock_guard<std::mutex> guard(mInternal->mVect.mutex);

	for (auto& window : mInternal->mVect.vector)
	{
		if (!window.get())
			continue;

		window->draw(this);

		if (!window->bOpen)
			detachWindow(window->mId);
	}
}

Window& WindowManager::getWindowIndexed(u32 idx)
{
	std::lock_guard<std::mutex> g(mInternal->mVect.mutex);

	return *mInternal->mVect.vector[idx].get();
}

u32 WindowManager::getNumWindows() const
{
	std::lock_guard<std::mutex> g(mInternal->mVect.mutex);

	return static_cast<u32>(mInternal->mVect.vector.size());
}
