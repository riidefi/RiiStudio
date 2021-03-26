#pragma once

#include <array>
#include <plugins/gc/Export/Texture.hpp>

namespace libcube {
struct Texture;
}

namespace libcube::UI {

struct ResizeAction {
  struct ResizeDimension {
    const char* name = "?";
    int before = -1;
    int value = -1;
    bool constrained = true;
  };

  std::array<ResizeDimension, 2> resize{
      ResizeDimension{"Width ", -1, -1, false},
      ResizeDimension{"Height", -1, -1, true}};

  librii::image::ResizingAlgorithm resizealgo =
      librii::image::ResizingAlgorithm::Lanczos;

  void resize_reset();

  // return if the window should stay alive
  bool resize_draw(Texture& data, bool* changed = nullptr);
};

struct ReformatAction {

  int reformatOpt = -1;

  void reformat_reset() { reformatOpt = -1; }

  // return if the window should stay alive
  bool reformat_draw(Texture& data, bool* changed = nullptr);
};

} // namespace libcube::UI
