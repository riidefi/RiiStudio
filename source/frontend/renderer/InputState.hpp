#pragma once

#include <glm/vec2.hpp>
#include <optional>

namespace riistudio::frontend {

struct InputState {
  bool forward : 1 = false;
  bool left : 1 = false;
  bool backward : 1 = false;
  bool right : 1 = false;

  bool up : 1 = false;
  bool down : 1 = false;

  bool clickSelect : 1 = false;
  bool clickView : 1 = false;

  struct MouseState {
    glm::vec2 position = {};
    float scroll = 0.0f;
  };
  std::optional<MouseState> mouse;
};

InputState buildInputState();

} // namespace riistudio::frontend
