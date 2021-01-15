#pragma once

#include <librii/gx.h>

namespace librii::hx {

struct CullMode {
  bool front = true;
  bool back = false;

  CullMode() = default;
  CullMode(bool f, bool b) : front(f), back(b) {}
  CullMode(librii::gx::CullMode c) { set(c); }
  bool operator==(const CullMode&) const = default;

  void set(librii::gx::CullMode c) {
    switch (c) {
    case librii::gx::CullMode::All:
      front = back = false;
      break;
    case librii::gx::CullMode::None:
      front = back = true;
      break;
    case librii::gx::CullMode::Front:
      front = false;
      back = true;
      break;
    case librii::gx::CullMode::Back:
      front = true;
      back = false;
      break;
    default:
      assert(!"Invalid cull mode");
      front = back = true;
      break;
    }
  }
  librii::gx::CullMode get() const noexcept {
    if (front && back)
      return librii::gx::CullMode::None;

    if (!front && !back)
      return librii::gx::CullMode::All;

    return front ? librii::gx::CullMode::Back : librii::gx::CullMode::Front;
  }
};

} // namespace librii::hx
