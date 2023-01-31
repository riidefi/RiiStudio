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
};

enum class State {
  Unengaged,
  // send settings request, set mode to
  WaitForSettings,
  // check for texture dependencies
  // tell the importer to fix them or abort
  WaitForTextureDependencies,
  // Now we actually import!
  // And we're done:
  Completed
};
struct AssimpContext {
  State state = State::Unengaged;
  const aiScene* mScene = nullptr;
  Settings mSettings;

  // Hack (we know importer will not be copied):
  // Won't be necessary when IBinaryDeserializable is split into Factory and
  // Instance, and Instance does not require copyable.
  std::shared_ptr<Assimp::Importer> importer =
      std::make_shared<Assimp::Importer>();
};

// Lifetime is tied to that of importer
const aiScene* ReadScene(kpi::IOTransaction& transaction, std::string path,
                         const Settings& settings, Assimp::Importer& importer);

Result<librii::rhst::SceneTree> ToSceneTree(const aiScene* scene,
                                            kpi::IOTransaction& transaction,
                                            const Settings& settings);

Result<librii::rhst::SceneTree> DoImport(std::string path,
                                         kpi::IOTransaction& transaction,
                                         const Settings& settings);

} // namespace librii::assimp2rhst
