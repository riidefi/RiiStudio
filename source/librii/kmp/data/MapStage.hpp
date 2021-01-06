#pragma once

#include <core/common.h> // u16
#include <glm/vec3.hpp>  // glm::vec3

namespace librii::kmp {

enum class StartPosition { Standard, Near };
enum class Corner { Left, Right };

class Stage {
public:
  bool operator==(const Stage&) const = default;

  u8 mLapCount;
  Corner mCorner;
  StartPosition mStartPosition;
  u8 mFlareTobi; //!< If the flash "jumps"
  struct lensFlareOptions_t {
    bool operator==(const lensFlareOptions_t&) const = default;
    u8 a, r, g, b;
  } mLensFlareOptions;

  // --- Pre Revision 2320: End of Structure

  u8 mUnk08;
  u8 _;
  u16 mSpeedModifier; //!< Used by the speed modifier cheat code as the
                      //!< two most significant bytes of a f32. (Originally
                      //!< implicit pad)
};

} // namespace librii::kmp
