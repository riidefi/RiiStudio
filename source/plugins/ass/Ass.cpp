// Assimp importer.
#include "Ass.hpp"

#include "AssImporter.hpp"
#include "AssLogger.hpp"
#include "Utility.hpp"
#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Plugins.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/j3d/Scene.hpp>
#include <vendor/assimp/DefaultLogger.hpp>
#include <vendor/assimp/Importer.hpp>
#include <vendor/assimp/postprocess.h>
#include <vendor/assimp/scene.h>
#include <vendor/fa5/IconsFontAwesome5.h>
#undef min

#include "InclusionMask.hpp"
#include "LogScope.hpp"
#include "SupportedFiles.hpp"

IMPORT_STD;

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

// TODO
u32 ClampMipMapDimension(u32 x) {
  // round down to last power of two
  const int old = x;
  int res = 0;
  while (x >>= 1)
    ++res;
  x = (1 << res);
  // round up
  if ((x << 1) - old < old - x)
    x <<= 1;

  return x;
}

enum class State {
  Unengaged,
  // send settings request, set mode to
  WaitForSettings,
  // Special step: Determine what to keep and what to discard
  WaitForReplace,
  // check for texture dependencies
  // tell the importer to fix them or abort
  WaitForTextureDependencies,
  // Now we actually import!
  // And we're done:
  Completed
};
struct AssimpContext {
  State state = State::Unengaged;
  // Hack (we know importer will not be copied):
  // Won't be necessary when IBinaryDeserializable is split into Factory and
  // Instance, and Instance does not require copyable.
  std::shared_ptr<Assimp::Importer> importer =
      std::make_shared<Assimp::Importer>();
  std::optional<AssImporter> helper = std::nullopt;
  std::set<std::pair<std::size_t, std::string>> unresolved;
  std::vector<std::pair<std::size_t, std::vector<u8>>> additional_textures;
  std::vector<std::size_t> requestedUnresolvedToID;

  u32 ass_flags = AlwaysFlags | DefaultFlags;
  float mMagnification = 1.0f;
  std::array<f32, 3> model_tint = {1.0f, 1.0f, 1.0f};

  // if mGenerateMipMaps, create mip levels until < mMinMipDimension or >
  // mMaxMipCount
  bool mGenerateMipMaps = true;
  int mMinMipDimension = 32;
  int mMaxMipCount = 5;
  // Set stencil outline if alpha
  bool mAutoTransparent = true;
  //
  u32 data_to_include = DefaultInclusionMask();
};

void RenderContextSettings(AssimpContext& ctx) {
  if (ImGui::CollapsingHeader("Importing Settings"_j,
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    //
    // Never enabled:
    //
    // aiProcess_CalcTangentSpace - We don't support NBT anywhere
    // aiProcess_JoinIdenticalVertices - We already reindex? I don't think we
    // need this.
    // aiProcess_MakeLeftHanded - TODO
    // aiProcess_EmbedTextures - TODO

    //
    // Always enabled
    //
    // aiProcess_Triangulate - Always enabled
    // aiProcess_SortByPType - Always on: We don't support lines/points, so we
    // have to filter them out.
    // aiProcess_GenBoundingBoxes - Always on
    // aiProcess_PopulateArmatureData - Always on
    // aiProcess_GenUVCoords - Always on
    // aiProcess_FlipUVs - TODO
    // aiProcess_FlipWindingOrder - TODO
    // aiProcess_ValidateDataStructure - No reason not to?
    // aiProcess_RemoveComponent  - Deslect all to disable

    //
    // Configure
    //
    // aiProcess_GenNormals - TODO
    // aiProcess_GenSmoothNormals - TODO
    // aiProcess_SplitLargeMeshes - We probably don't need this?
    // aiProcess_PreTransformVertices - Doubt this is desirable..
    // aiProcess_LimitBoneWeights - TODO
    // aiProcess_ImproveCacheLocality
    // TODO
    // ImGui::CheckboxFlags("Cache Locality Optimization", &ass_flags,
    //                      aiProcess_ImproveCacheLocality);

    // aiProcess_FindInstances - TODO

    // aiProcess_SplitByBoneCount - ?
    // aiProcess_Debone - TODO
    // aiProcess_GlobalScale
    ImGui::InputFloat("Model Scale"_j, &ctx.mMagnification);
    // aiProcess_ForceGenNormals - TODO
    // aiProcess_DropNormals - TODO

    int import_config = 0;
    ImGui::Combo("Import Config"_j, &import_config, "World (single bone)\0\0");
  }

  if (ImGui::CollapsingHeader((const char*)ICON_FA_IMAGES u8" Mip Maps",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Generate Mipmaps"_j, &ctx.mGenerateMipMaps);
    {
      util::ConditionalActive g(ctx.mGenerateMipMaps);
      ImGui::Indent(50);
      ImGui::SliderInt("Minimum mipmap dimension."_j, &ctx.mMinMipDimension, 1,
                       512);
      ctx.mMinMipDimension = ClampMipMapDimension(ctx.mMinMipDimension);

      ImGui::SliderInt("Maximum number of mipmaps."_j, &ctx.mMaxMipCount, 0, 8);

      ImGui::Indent(-50);
    }
  }
  if (ImGui::CollapsingHeader((const char*)ICON_FA_BRUSH u8" Material Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox(
        "Detect transparent textures, and configure materials accordingly."_j,
        &ctx.mAutoTransparent);
    ImGui::CheckboxFlags("Combine identical materials"_j, &ctx.ass_flags,
                         aiProcess_RemoveRedundantMaterials);
    ImGui::CheckboxFlags("Bake UV coord scale/rotate/translate"_j,
                         &ctx.ass_flags, aiProcess_TransformUVCoords);
    ImGui::ColorEdit3("Model Tint"_j, ctx.model_tint.data());
  }
  if (ImGui::CollapsingHeader(
          (const char*)ICON_FA_PROJECT_DIAGRAM u8" Mesh Settings",
          ImGuiTreeNodeFlags_DefaultOpen)) {
    // aiProcess_FindDegenerates - TODO
    ImGui::CheckboxFlags("Remove degenerate triangles"_j, &ctx.ass_flags,
                         aiProcess_FindDegenerates);
    // aiProcess_FindInvalidData - TODO
    ImGui::CheckboxFlags("Remove invalid data"_j, &ctx.ass_flags,
                         aiProcess_FindInvalidData);
    // aiProcess_FixInfacingNormals
    ImGui::CheckboxFlags("Fix flipped normals"_j, &ctx.ass_flags,
                         aiProcess_FixInfacingNormals);
    // aiProcess_OptimizeMeshes
    ImGui::CheckboxFlags("Optimize meshes"_j, &ctx.ass_flags,
                         aiProcess_OptimizeMeshes);
    // aiProcess_OptimizeGraph
    ImGui::CheckboxFlags("Compress bones (for static scenes)"_j, &ctx.ass_flags,
                         aiProcess_OptimizeGraph);
  }
  if (ImGui::CollapsingHeader(
          (const char*)ICON_FA_PROJECT_DIAGRAM u8" Data to Import",
          ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::CheckboxFlags(
        (const char*)ICON_FA_SORT_AMOUNT_UP u8" Vertex Normals",
        &ctx.data_to_include, aiComponent_NORMALS);
    // aiComponent_TANGENTS_AND_BITANGENTS: Unsupported
    ImGui::CheckboxFlags((const char*)ICON_FA_PALETTE u8" Vertex Colors",
                         &ctx.data_to_include, aiComponent_COLORS);
    ImGui::CheckboxFlags((const char*)ICON_FA_GLOBE u8" UV Maps",
                         &ctx.data_to_include, aiComponent_TEXCOORDS);
    ImGui::CheckboxFlags((const char*)ICON_FA_BONE u8" Bone Weights",
                         &ctx.data_to_include, aiComponent_BONEWEIGHTS);
    // aiComponent_ANIMATIONS: Unsupported
    // TODO: aiComponent_TEXTURES: Unsupported
    // aiComponent_LIGHTS: Unsupported
    // aiComponent_CAMERAS: Unsupported
    ImGui::CheckboxFlags((const char*)ICON_FA_PROJECT_DIAGRAM u8" Meshes",
                         &ctx.data_to_include, aiComponent_MESHES);
    ImGui::CheckboxFlags((const char*)ICON_FA_PAINT_BRUSH u8" Materials",
                         &ctx.data_to_include, aiComponent_MATERIALS);
  }
}

class AssimpPlugin {
public:
  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const;

  void read(kpi::IOTransaction& transaction);
  void render();

  void GotoNextState() {
    assert(static_cast<int>(mContext->state) <
           static_cast<int>(State::Completed));
    mContext->state = static_cast<State>(static_cast<int>(mContext->state) + 1);
  }
  void GotoExitState() { mContext->state = State::Completed; }

  void StateUnengaged(kpi::IOTransaction& transaction) {
    // Ask for properties
    transaction.state = kpi::TransactionState::ConfigureProperties;
    GotoNextState();
  }

  // Expect settings to have been configured.
  // Load assimp file and check for texture dependencies
  void StateWaitForSettings(kpi::IOTransaction& transaction) {
    std::string path(transaction.data.getProvider()->getFilePath());

    {
      AssimpLoggerScope g_assimplogger(std::make_unique<AssLogger>(
          transaction.callback, getFileShort(path)));

      // Only include components we asked for
      {
        const u32 exclusion_mask = FlipExclusionMask(mContext->data_to_include);
        DebugPrintExclusionMask(exclusion_mask);
        mContext->importer->SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
                                               exclusion_mask);
      }

      if (mContext->mMagnification != 1.0f) {
        mContext->ass_flags |= aiProcess_GlobalScale;
        mContext->importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                                             mContext->mMagnification);
      }

      mContext->importer->ReadFileFromMemory(
          transaction.data.data(), transaction.data.size(),
          aiProcess_PreTransformVertices, path.c_str());
      const auto* pScene =
          mContext->importer->ApplyPostProcessing(mContext->ass_flags);

      if (!pScene) {
        transaction.state = kpi::TransactionState::Failure;
        return;
      }
      double unit_scale = 0.0;
      pScene->mMetaData->Get("UnitScaleFactor", unit_scale);

      // Handle custom units
      if (unit_scale != 0.0) {
        // FBX-only property: internally in centimeters
        mContext->importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                                             (1.0 / unit_scale) / 100.0);
        mContext->importer->ApplyPostProcessing(aiProcess_GlobalScale);
      }
      if (pScene == nullptr) {
        // We will never be called again..
        // transaction.callback(kpi::IOMessageClass::Error, getFileShort(path),
        //                      importer->GetErrorString());
        transaction.state = kpi::TransactionState::Failure;
        return;
      }

      mContext->helper.emplace(pScene, &transaction.node);
    }

    std::vector<std::string> mat_merge;
    mContext->unresolved = mContext->helper->PrepareAss(
        mContext->mGenerateMipMaps, mContext->mMinMipDimension,
        mContext->mMaxMipCount, path);

    mContext->state = State::WaitForTextureDependencies;
    // This step might be optional
    if (!mContext->unresolved.empty()) {
      transaction.state = kpi::TransactionState::ResolveDependencies;
      transaction.resolvedFiles.resize(mContext->unresolved.size());
      transaction.unresolvedFiles.reserve(mContext->unresolved.size());
      for (auto& missing : mContext->unresolved) {
        transaction.unresolvedFiles.push_back(missing.second);
        mContext->requestedUnresolvedToID.push_back(missing.first);
      }
    }
  }

  void StateWaitForTextureDependencies(kpi::IOTransaction& transaction) {
    if (!mContext->unresolved.empty()) {
      for (std::size_t i = 0; i < transaction.resolvedFiles.size(); ++i) {
        auto& found = transaction.resolvedFiles[i];
        if (found.empty())
          continue;

        // Without this we would replace texture 0, even if texture 7 is the one
        // missing.
        const auto tex_index = mContext->requestedUnresolvedToID[i];
        mContext->additional_textures.emplace_back(tex_index, std::move(found));
      }
    }
    mContext->helper->SetTransaction(transaction);
    mContext->helper->ImportAss(
        mContext->additional_textures, mContext->mGenerateMipMaps,
        mContext->mMinMipDimension, mContext->mMaxMipCount,
        mContext->mAutoTransparent,
        glm::vec3(mContext->model_tint[0], mContext->model_tint[1],
                  mContext->model_tint[2]));

    switch (transaction.state) {
    case kpi::TransactionState::Failure:
      transaction.state = kpi::TransactionState::Failure;
      GotoExitState();
      return;

    // Still read the file
    case kpi::TransactionState::FailureToSave:
      transaction.state = kpi::TransactionState::Complete;
      GotoExitState();
      return;

    // Should not be possible to get in this state, programmer error
    case kpi::TransactionState::ConfigureProperties:
    case kpi::TransactionState::ResolveDependencies:
      assert(!"Invalid state");
      transaction.state = kpi::TransactionState::Failure;
      GotoExitState();
      return;
    }
  }

  std::shared_ptr<AssimpContext> mContext;
};

std::unique_ptr<kpi::IBinaryDeserializer> CreatePlugin() {
  return std::make_unique<
      kpi::detail::ApplicationPluginsImpl::TBinaryDeserializer<AssimpPlugin>>();
}

std::string AssimpPlugin::canRead(const std::string& file,
                                  oishii::BinaryReader& reader) const {
  if (!IsExtensionSupported(file)) {
    return "";
  }

  return typeid(lib3d::Scene).name();
}

void AssimpPlugin::read(kpi::IOTransaction& transaction) {
  if (mContext == nullptr) {
    mContext = std::make_shared<AssimpContext>();
  }

  // Ask for properties
  if (mContext->state == State::Unengaged) {
    StateUnengaged(transaction);
    return;
  }
  if (mContext->state == State::WaitForSettings) {
    StateWaitForSettings(transaction);
    if (!mContext->unresolved.empty()) {
      return;
    }
    // There are no textures, so fallthrough to final state
  }
  if (mContext->state == State::WaitForTextureDependencies) {
    StateWaitForTextureDependencies(transaction);
    return;
  }
}

void AssimpPlugin::render() {
  if (mContext) {
    RenderContextSettings(*mContext);
  } else {
    ImGui::Text("There is no context");
  }
}

} // namespace riistudio::ass
