#pragma once

#include <frontend/root.hpp> // RootWindow

namespace riistudio::frontend {

struct MouseHider {
  // Otherwise we run into a bug where the mouse goes AWOL when you close the
  // window.
  ~MouseHider() { show(); }

  void show() { RootWindow::spInstance->showMouse(); }
  void hide() { RootWindow::spInstance->hideMouse(); }

  // Returns whether to camera process
  bool begin_interaction(bool focused) {
    if (!focused) {
      show();
      return false;
    }

    return true;
  }

  void end_interaction(bool user_clicked) {
    if (user_clicked) {
      hide();
    } else {
      show();
    }
  }
};

} // namespace riistudio::frontend
