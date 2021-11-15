/*!
 * @brief Headers for CheckPaths, ItemPaths, EnemyPaths and Rails (Paths)
 */
#pragma once

#include <array>                  // std::array
#include <core/common.h>          // u32
#include <glm/vec2.hpp>           // glm::vec2
#include <glm/vec3.hpp>           // glm::vec3
#include <llvm/ADT/SmallVector.h> // llvm::SmallVector

namespace librii::kmp {

class CheckPoint {
public:
  bool operator==(const CheckPoint&) const = default;

  glm::vec2 getLeft() const { return mLeft; }
  void setLeft(const glm::vec2& p) { mLeft = p; }

  glm::vec2 getRight() const { return mRight; }
  void setRight(const glm::vec2& p) { mRight = p; }

  u8 getRespawnIndex() const { return mRespawnIndex; }
  void setRespawnIndex(u8 r) { mRespawnIndex = r; }

  u8 getLapCheck() const { return mLapCheck; }
  void setLapCheck(u8 c) { mLapCheck = c; }

public:
  glm::vec2 mLeft;
  glm::vec2 mRight;

  u8 mRespawnIndex = 0;
  u8 mLapCheck = 0xFF;
};

template <typename PointT> struct DirectedGraph {
  bool operator==(const DirectedGraph&) const = default;

  llvm::SmallVector<PointT, 16> mPoints;

  // Over 6 cannot be serialized
  llvm::SmallVector<u8, 6> mPredecessors;
  llvm::SmallVector<u8, 6> mSuccessors;
  std::array<u8, 2> misc;
};

class CheckPath : public DirectedGraph<CheckPoint> {};

class EnemyPoint {
public:
  bool operator==(const EnemyPoint&) const = default;

public:
  glm::vec3 position;
  f32 deviation;
  std::array<u8, 4> param;
};

class EnemyPath : public DirectedGraph<EnemyPoint> {};

class ItemPoint {
public:
  bool operator==(const ItemPoint&) const = default;

public:
  glm::vec3 position{0.0f};
  f32 deviation{0.0f};
  std::array<u8, 4> param;
};

class ItemPath : public DirectedGraph<ItemPoint> {};

enum class Interpolation {
  Linear, //!< Move along the shortest distance from A to B.
  Spline  //!< Points are interpolated as cubic splines
};
enum class LoopPolicy {
  Closed, //!< Implicit connection from end to start: animation loops
          //!< unidirectionally.
  Open    //!< Ping-pong between the start and end.
};

class Rail {
public:
  bool operator==(const Rail&) const = default;

  class Point {
  public:
    bool operator==(const Point&) const = default;

    glm::vec3 position;
    std::array<u16, 2> params;
  };

  Interpolation getInterpolation() const { return mInterpolation; }
  void setInterpolation(Interpolation p) { mInterpolation = p; }
  bool isBezier() const { return mInterpolation == Interpolation::Spline; }
  void setBezier(bool b) {
    mInterpolation = b ? Interpolation::Spline : Interpolation::Linear;
  }

  LoopPolicy getLoopPolicy() const { return mPolicy; }
  void setLoopPolicy(LoopPolicy p) { mPolicy = p; }
  bool isClosed() const { return mPolicy == LoopPolicy::Closed; }
  void setClosed(bool c) {
    mPolicy = c ? LoopPolicy::Closed : LoopPolicy::Open;
  }

  auto begin() { return mPoints.begin(); }
  auto end() { return mPoints.end(); }
  auto begin() const { return mPoints.begin(); }
  auto end() const { return mPoints.end(); }

  void resize(std::size_t sz) { mPoints.resize(sz); }
  std::size_t size() const { return mPoints.size(); }

private:
  Interpolation mInterpolation;
  LoopPolicy mPolicy;
  llvm::SmallVector<Point, 16> mPoints;
};

using Path = Rail;

inline bool IsLapCheck(const CheckPoint& p) { return p.mLapCheck != 0xFF; }

inline bool IsLapCheck(const CheckPath& p) {
  return std::any_of(p.mPoints.begin(), p.mPoints.end(),
                     [](const CheckPoint& p) { return IsLapCheck(p); });
}

} // namespace librii::kmp
