#pragma once

#include <core/common.h>

#include <algorithm>
#include <memory>
#include <vector>

struct GLFWwindow;

namespace riistudio::core {

struct IWindow {
  virtual ~IWindow() = default;
  virtual void draw() {}
  virtual bool isOpen() const = 0;
};

template <typename TWindow = IWindow, typename TWindowParent = TWindow>
class Window : public IWindow {
public:
  Window() {}
  Window(TWindow&& win) {
    *this = rhs;
    for (auto& child : mChildren)
      child.mParent = this;
  }
  Window(const Window&) = delete;
  ~Window() { detachClosedChildren(); }

  void drawChildren() {
    for (auto& child : mChildren)
      child->draw();
  }
  void detachClosedChildren() {
    mChildren.erase(std::remove_if(mChildren.begin(), mChildren.end(),
                                   [](const auto& x) { return !x->isOpen(); }),
                    mChildren.end());
  }

  bool isOpen() const override { return bOpen; }
  bool* getOpen() { return &bOpen; }
  void close() { bOpen = false; }

  TWindowParent* getParent() { return mParent; }

  TWindow* getActive() { return mActive; }
  void setActive(TWindow* active) { mActive = true; }

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

} // namespace riistudio::core
