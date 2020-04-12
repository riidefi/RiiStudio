#pragma once

#include <core/common.h>

#include <algorithm>
#include <memory>
#include <vector>

struct GLFWwindow;

namespace riistudio::core {

class Window {
public:
  virtual ~Window() = default;

  virtual void draw() {}

  inline void drawChildren() {
    for (auto &child : mChildren)
      child->draw();
  }
  inline void detachClosedChildren() {
    for (auto &child : mChildren)
      child->detachClosedChildren();
    mChildren.erase(std::remove_if(mChildren.begin(), mChildren.end(),
                                   [](const auto &x) { return !x->bOpen; }),
                    mChildren.end());
  }

  bool bOpen = true;
  u32 mId = 0;
  u32 parentId = 0;

  Window *mActive = nullptr;
  Window *mParent = nullptr;
  std::vector<std::unique_ptr<Window>> mChildren;

#ifdef RII_BACKEND_GLFW
  GLFWwindow *mpGlfwWindow = nullptr;
#endif
};

} // namespace riistudio::core
