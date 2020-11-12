#pragma once

#include <core/common.h>

#include <algorithm>
#include <memory>
#include <vector>

struct GLFWwindow;

namespace riistudio::frontend {

struct IWindow {
  virtual ~IWindow() = default;
  virtual void draw() {}
  virtual bool isOpen() const = 0;
};

template <typename TWindow = IWindow, typename TWindowParent = TWindow>
class Window : public IWindow {
public:
  Window() {}
  Window(TWindow&& rhs) {
    *this = rhs;
    for (auto& child : mChildren)
      child.mParent = this;
  }
  Window(const Window&) = delete;
  ~Window() { detachClosedChildren(); }

  void drawChildren() {
    for (int i = 0; i < mChildren.size(); ++i) {
      mChildren[i]->draw();
    }
  }
  void detachClosedChildren() {
    bool reassign_active = false;

    mChildren.erase(std::remove_if(mChildren.begin(), mChildren.end(),
                                   [&](const auto& x) {
                                     if (x->isOpen())
                                       return false;

                                     if (x.get() == getActive())
                                       reassign_active = true;

                                     return true;
                                   }),
                    mChildren.end());

    if (reassign_active) {
      mActive = mChildren.empty() ? nullptr : mChildren[0].get();
    }
  }

  bool isOpen() const override { return bOpen; }
  bool* getOpen() { return &bOpen; }
  void close() { bOpen = false; }

  TWindowParent* getParent() { return mParent; }

  TWindow* getActive() { return mActive; }
  void setActive(TWindow* active) { mActive = active; }

  bool hasChildren() const { return !mChildren.empty(); }

  TWindow& attachWindow(std::unique_ptr<TWindow> child) {
    TWindow& ref = *child.get();
    if (mChildren.empty())
      mActive = child.get();
    mChildren.push_back(std::move(child));
    if constexpr (std::is_base_of<Window, TWindow>::value) {
      ref.mParent = this;
#ifdef RII_BACKEND_GLFW
      ref.mpGlfwWindow = mpGlfwWindow;
#endif
    }
    return ref;
  }

  void detachAllChildren() { mChildren.clear(); }

private:
  bool bOpen = true;

  TWindow* mActive = nullptr;
  TWindowParent* mParent = nullptr;
  std::vector<std::unique_ptr<TWindow>> mChildren;

#ifdef RII_BACKEND_GLFW
public:
  GLFWwindow* mpGlfwWindow = nullptr;
#endif
};

} // namespace riistudio::frontend
