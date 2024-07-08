#pragma once

/// @file Modern C++ version of an assimp scene. Only includes data we actually
/// need.

#include <array>
#include <core/common.h>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <set>
#include <string>
#include <vector>

struct aiScene;

namespace librii::lra {

struct SceneAttrs {
  /// Not a fully loaded scene; should not proceed
  bool Incomplete = false;

  /// Set by aiPostProcess_ValidateDS
  bool Validated = false;

  /// Set by aiPostProcess_ValidateDS
  /// e.g. missing texture
  bool HasValidationIssues = false;

  /// Set by aiProcess_JoinIdenticalVertices
  bool HasSharedVertices = false;
};

struct Face {
  std::vector<unsigned int> indices;
};

/// A single influence of a bone on a vertex.
struct VertexWeight {
  u32 vertexIndex = 0;
  /// The strength of the influence in the range [0, 1].
  /// The influence from all bones at one vertex amounts to 1.
  float weight = 1.0f;
};

struct Bone {
  std::string name;

  /// Per-vertex weights
  std::vector<VertexWeight> weights;

  // mArmature; we ignore
  u32 nodeIndex;
  // mOffsetMatrix; we ignore
};

struct Mesh {
  /// Use SortByPrimitiveType to ensure size() == 1
  // std::set<PrimitiveType> primitive_types;

  /// Position vectors
  std::vector<glm::vec3> positions;

  /// Normal vectors: may contain NaN
  std::vector<glm::vec3> normals;

  // Tangents; we ignore
  // Bitangents; we ignore

  /// Color vectors; most vecs will usually be empty
  std::array<std::vector<glm::vec4>, 8> colors;

  /// UV vectors; most vecs will usually be empty
  std::array<std::vector<glm::vec2>, 8> uvs;

  // Note: We don't support U/UVW coords like assimp do es

  /// Usually a list of triangles
  std::vector<Face> faces;

  /// Deformations on the triangles
  std::vector<Bone> bones;

  uint32_t materialIndex = 0;

  std::string name;

  // AnimMeshes; we ignore
  // Morping; we ignore

  glm::vec3 min;
  glm::vec3 max;

  // aiMesh helpers
  bool HasPositions() const { return !positions.empty(); }
  bool HasNormals() const { return !normals.empty(); }
  bool HasVertexColors(unsigned int i) const {
    return i < colors.size() && !colors[i].empty();
  }
  bool HasTextureCoords(unsigned int i) const {
    return i < uvs.size() && !uvs[i].empty();
  }
};

struct Material {
  std::string name;

  // Dramatic simplification
  std::string texture;
};

struct Node {
  /// May be empty if unused!
  std::string name;

  /// Relative to parent
  glm::mat4 xform;

  int parent = -1;

  /// Indices
  std::vector<size_t> children;

  ///
  std::vector<unsigned int> meshes;

  // mMetaData; we ignore
};

struct Animation {
  std::string name;

  /// Duration of animation.
  double frameCount;

  /// May be 0!
  double fps;

  // TODO: Remainder
};

/// Abstracted view of scene. The large change here is that nodes are stored as
/// a flat array, rather than tree.
struct Scene {
  /// Set by loading/validation passes
  SceneAttrs flags;

  /// Root node is at first index
  std::vector<Node> nodes;

  /// Holds triangle data
  std::vector<Mesh> meshes;

  /// Holds material data
  std::vector<Material> materials;

  /// Holds animation data
  std::vector<Animation> animations;

  // Embedded textures; we ignore
  // Lights; we ignore
  // Cameras; we ignore
  // Metadata; we ignore
};

Scene ReadScene(const aiScene& scn);

/// Assimp post-pass will first split by prim type. This will discard
/// non-triangle prims.
void DropNonTriangularMeshes(Scene& scn);

/// Assimp violates this all the time when splitting meshes.
void MakeMeshNamesUnique(Scene& scn);

/// Write scene to diff-able JSON
std::string PrintJSON(const Scene& scn);

} // namespace librii::lra
