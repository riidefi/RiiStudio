#pragma once

#include <core/kpi/Plugins.hpp>
#include <glm/vec3.hpp>
#include <librii/rhst/RHST.hpp>
#include <memory>
#include <plugins/ass/InclusionMask.hpp>
#include <vendor/assimp/postprocess.h>

namespace riistudio::ass {

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

// Builtin-in ImGui UI for `Settings`
void RenderContextSettings(Settings& ctx);

Result<librii::rhst::SceneTree> DoImport(std::string path,
                                         kpi::IOTransaction& transaction,
                                         const Settings& settings);

std::unique_ptr<kpi::IBinaryDeserializer> CreatePlugin();

} // namespace riistudio::ass
