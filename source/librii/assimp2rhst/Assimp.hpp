#pragma once

#include <glm/vec3.hpp>
#include <librii/assimp2rhst/InclusionMask.hpp>
#include <librii/rhst/RHST.hpp>
#include <memory>
#include <vendor/assimp/Importer.hpp>
#include <vendor/assimp/postprocess.h>

// TODO: Should not depend on this
#include <LibBadUIFramework/Plugins.hpp>

namespace librii::assimp2rhst {

static constexpr u32 DefaultFlags =
    aiProcess_GenSmoothNormals | aiProcess_RemoveRedundantMaterials |
    aiProcess_FindDegenerates | aiProcess_FindInvalidData |
    aiProcess_OptimizeMeshes | aiProcess_Debone | aiProcess_OptimizeGraph |
    aiProcess_RemoveComponent;

static constexpr u32 AlwaysFlags =
    aiProcess_JoinIdenticalVertices   // Merge doubles
    | aiProcess_ValidateDataStructure //
    | aiProcess_Triangulate // We only accept triangles, everything else is
                            // rejected
    | aiProcess_SortByPType // For filtering out non-triangles
    | aiProcess_PopulateArmatureData // Used by bones
    | aiProcess_GenUVCoords | aiProcess_GenBoundingBoxes | aiProcess_FlipUVs //
    | aiProcess_FlipWindingOrder                                             //
    ;
constexpr u32 ClampMipMapDimension(u32 x) {
  // Round down to last power of two
  const u32 old = x;
  u32 res = 0;
  while (x >>= 1)
    ++res;
  x = (1 << res);
  // Round up
  if ((x << 1) - old < old - x)
    x <<= 1;
  return x;
}
struct Settings {
  u32 mAiFlags = AlwaysFlags | DefaultFlags;
  float mMagnification = 1.0f;
  glm::vec3 mModelTint{1.0f, 1.0f, 1.0f};

  // if mGenerateMipMaps, create mip levels until < mMinMipDimension or >
  // mMaxMipCount
  bool mGenerateMipMaps = true;
  int mMinMipDimension = 32;
  int mMaxMipCount = 5;
  // Set stencil outline if alpha
  bool mAutoTransparent = true;
  //
  u32 mDataToInclude = DefaultInclusionMask();

  bool mIgnoreRootTransform = false;
};

using KCallback = std::function<void(kpi::IOMessageClass message_class,
                                     const std::string_view domain,
                                     const std::string_view message_body)>;

// Lifetime is tied to that of importer
const aiScene* ReadScene(KCallback callback, std::span<const u8> file,
                         std::string path, const Settings& settings,
                         Assimp::Importer& importer);

Result<librii::rhst::SceneTree> ToSceneTree(const aiScene* scene,
                                            const Settings& settings);

Result<librii::rhst::SceneTree> DoImport(std::string path, KCallback callback,
                                         std::span<const u8> file,
                                         const Settings& settings);

} // namespace librii::assimp2rhst
