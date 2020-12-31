#pragma once

#include <array>         // std::array
#include <core/common.h> // u32
#include <glm/vec3.hpp>  // glm::vec3

namespace librii::kmp {

class CourseMap;

enum class CannonType { Direct, Curved, Curved2 };

class Cannon {
  friend class KMP;

public:
  bool operator==(const Cannon&) const = default;

  CannonType getType() const { return mType; }
  void setType(CannonType t) { mType = t; }

  glm::vec3 getPosition() const { return mPosition; }
  void setPosition(const glm::vec3& p) { mPosition = p; }

  glm::vec3 getRotation() const { return mRotation; }
  void setRotation(const glm::vec3& r) { mRotation = r; }

private:
public:
  CannonType mType = CannonType::Direct;
  glm::vec3 mPosition{0.0f, 0.0f, 0.0f};
  glm::vec3 mRotation{0.0f, 0.0f, 0.0f};
};

} // namespace librii::kmp
