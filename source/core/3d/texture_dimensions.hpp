#pragma once

namespace riistudio::core {

template <typename T> struct TextureDimensions {
  T width, height;

  constexpr bool isPowerOfTwo() {
    return !(width & (width - 1)) && (!height & (height - 1));
  }
  constexpr bool isBlockAligned() {
    return !(width % 0x20) && !(height % 0x20);
  }

  bool operator==(const TextureDimensions& rhs) const {
    return width == rhs.width && height == rhs.height;
  }
  bool operator!=(const TextureDimensions& rhs) const {
    return !operator==(rhs);
  }
};

} // namespace riistudio::core
