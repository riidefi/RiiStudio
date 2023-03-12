/*!
 * @brief Headers for CheckPaths, ItemPaths, EnemyPaths and Rails (Paths)
 */
#pragma once

#include <array>               // std::array
#include <core/common.h>       // u32
#include <glm/vec2.hpp>        // glm::vec2
#include <glm/vec3.hpp>        // glm::vec3
#include <rsl/SmallVector.hpp> // rsl::small_vector

namespace librii::kmp {

struct CheckPoint {
  glm::vec2 mLeft;
  glm::vec2 mRight;

  u8 mRespawnIndex = 0;
  u8 mLapCheck = 0xFF;

  bool operator==(const CheckPoint&) const = default;
};

template <typename PointT> struct DirectedGraph {
  rsl::small_vector<PointT, 16> points;

  // Over 6 cannot be serialized
  rsl::small_vector<u8, 6> mPredecessors;
  rsl::small_vector<u8, 6> mSuccessors;
  std::array<u8, 2> misc;

  bool operator==(const DirectedGraph&) const = default;
};

struct CheckPath : public DirectedGraph<CheckPoint> {
  bool operator==(const CheckPath&) const = default;
};

struct EnemyPoint {
  glm::vec3 position;
  f32 deviation{};
  std::array<u8, 4> param;

  bool operator==(const EnemyPoint&) const = default;
};

struct EnemyPath : public DirectedGraph<EnemyPoint> {
  bool operator==(const EnemyPath&) const = default;
};

struct ItemPoint {
  glm::vec3 position{0.0f};
  f32 deviation{0.0f};
  std::array<u8, 4> param;

  bool operator==(const ItemPoint&) const = default;
};

struct ItemPath : public DirectedGraph<ItemPoint> {
  bool operator==(const ItemPath&) const = default;
};

enum class Interpolation {
  //! Move along the shortest distance from A to B.
  Linear,
  //! Points are interpolated as cubic splines
  Spline,
};
enum class LoopPolicy {
  //! Implicit connection from end to start: animation loops unidirectionally
  Closed,
  //!< Ping-pong between the start and end.
  Open,
};

struct RailPoint {
  glm::vec3 position;
  std::array<u16, 2> params;

  bool operator==(const RailPoint&) const = default;
};

struct Rail {
  Interpolation interpolation{Interpolation::Spline};
  LoopPolicy loopPolicy{LoopPolicy::Closed};
  rsl::small_vector<RailPoint, 16> points;

  bool operator==(const Rail&) const = default;
};

using Path = Rail;

inline bool IsLapCheck(const CheckPoint& p) { return p.mLapCheck != 0xFF; }

inline bool IsLapCheck(const CheckPath& p) {
  return std::any_of(p.points.begin(), p.points.end(),
                     [](const CheckPoint& p) { return IsLapCheck(p); });
}

} // namespace librii::kmp
