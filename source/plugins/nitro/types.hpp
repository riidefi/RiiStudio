/*!
 * @brief Fixed-point DS types.
 */


#include <cmath>               // std::roundf
#include <core/common.h>       // u32
#include <type_traits>         // std::is_integral_v
#include <vendor/glm/vec3.hpp> // glm::vec3

#pragma push(packed)

namespace riistudio::nitro {

template <typename T> struct fx {
  static_assert(std::is_integral_v<T>, "T must be an integer");

  operator float() const { return static_cast<float>(hex) / 4096.0f; }
  fx operator=(const fx& rhs) {
    hex = rhs.hex;
    return *this;
  }
  fx operator=(float rhs) {
    hex = std::roundf(rhs / 4096.0f);
    return *this;
  }

  fx() = default;
  fx(const fx& val) : hex(val.hex) {}
  fx(float val) { *this = val; }

  T hex = 0;
};

using fx32 = fx<s32>;
using fx16 = fx<s16>;

static_assert(sizeof(fx32) == 4 && sizeof(fx16) == 2, "Invalidly sized types");

template <typename T> struct VecFx {
  operator glm::vec3() const { return {x, y, z}; }
  VecFx& operator=(const glm::vec3& vec) {
    x = vec.x;
    y = vec.y;
    z = vec.z;
  }

  fx<T> x = 0.0f, y = 0.0f, z = 0.0f;
};

using VecFx32 = VecFx<s32>;
using VecFx16 = VecFx<s16>;

static_assert(sizeof(VecFx32) == 4 * 3 && sizeof(VecFx16) == 2 * 3,
              "Invalidly sized types");

union ARGB1555 {
  struct {
    u16 a : 1;
    u16 r : 5;
    u16 g : 5;
    u16 b : 5;
  };

  operator glm::vec3() const {
    return {static_cast<float>(r) / 32.0f, static_cast<float>(g) / 32.0f,
            static_cast<float>(b) / 32.0f};
  }
  void operator=(const glm::vec3& vec) {
    r = std::roundf(vec.x * 32.0f);
    g = std::roundf(vec.y * 32.0f);
    b = std::roundf(vec.z * 32.0f);
  }

  u16 hex = 0;
};

static_assert(sizeof(ARGB1555) == 2, "Invalidly sized types");

} // namespace riistudio::nitro

#pragma pop(packed)
