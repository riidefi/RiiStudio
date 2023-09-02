#pragma once

#include <librii/math/aabb.hpp>
#include <string>
#include <vector>

namespace kpi {
struct LightIOTransaction;
}

namespace librii::g3d {

enum class ScalingRule {
  Standard,
  XSI,
  Maya,
};
enum class TextureMatrixMode {
  Maya,
  XSI,
  Max,
};
enum class EnvelopeMatrixMode {
  Normal,
  Approximation,
  Precision,
};

struct G3DModelDataData {
  ScalingRule mScalingRule = ScalingRule::Standard;
  TextureMatrixMode mTexMtxMode = TextureMatrixMode::Maya;
  EnvelopeMatrixMode mEvpMtxMode = EnvelopeMatrixMode::Normal;
  std::string sourceLocation;
  librii::math::AABB aabb;

  std::string mName = "course";

  bool operator==(const G3DModelDataData& rhs) const = default;
};

struct ModelInfo {
  librii::g3d::ScalingRule scalingRule = ScalingRule::Maya;
  librii::g3d::TextureMatrixMode texMtxMode = TextureMatrixMode::Maya;
  // numVerts: recomputed
  // numTris: recomputed
  std::string sourceLocation;
  // numViewMtx: recomputed
  // normalMtxArray: recomputed
  // texMtxArray: recomputed
  // boundVolume: not supported
  librii::g3d::EnvelopeMatrixMode evpMtxMode = EnvelopeMatrixMode::Normal;
  glm::vec3 min{0.0f, 0.0f, 0.0f};
  glm::vec3 max{0.0f, 0.0f, 0.0f};
};
struct DrawMatrix {
  struct MatrixWeight {
    u32 boneId;
    f32 weight;

    MatrixWeight() : boneId(-1), weight(0.0f) {}
    MatrixWeight(u32 b, f32 w) : boneId(b), weight(w) {}

    bool operator==(const MatrixWeight& rhs) const = default;
  };

  std::vector<MatrixWeight> mWeights; // 1 weight -> singlebound, no envelope

  bool operator==(const DrawMatrix& rhs) const = default;
};
struct BoneData;
class PositionBuffer;
class NormalBuffer;
class ColorBuffer;
class TextureCoordinateBuffer;
struct G3dMaterialData;
struct PolygonData;

class BinaryModel;

struct Model {
  std::string name;
  ModelInfo info;
  std::vector<BoneData> bones;
  std::vector<PositionBuffer> positions;
  std::vector<NormalBuffer> normals;
  std::vector<ColorBuffer> colors;
  std::vector<TextureCoordinateBuffer> texcoords;
  std::vector<G3dMaterialData> materials;
  // BinaryTevs: included in materials
  std::vector<PolygonData> meshes;
  // bytecodes: Applied to bones; remainder goes here (bone matrices are
  // excluded)
  std::vector<DrawMatrix> matrices;

  static Result<Model> from(const BinaryModel& model,
                            kpi::LightIOTransaction& transaction,
                            std::string_view transaction_path);
  Result<BinaryModel> binary() const;

#ifndef MODEL_DEF
  // Allows us to forward declare the AnimIO stuff
  Model();
  Model(const Model&);
  Model(Model&&) noexcept;
  ~Model();
#endif
};

} // namespace librii::g3d
