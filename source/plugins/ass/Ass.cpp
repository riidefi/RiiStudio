// Assimp importer.
#include "Ass.hpp"

#include "AssImporter.hpp"
#include "LogScope.hpp"
#include "Logger.hpp"
#include "SupportedFiles.hpp"
#include "Utility.hpp"
#include <core/kpi/Plugins.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/rhst/RHSTImporter.hpp>
#include <vendor/assimp/DefaultLogger.hpp>
#include <vendor/assimp/Importer.hpp>
#include <vendor/assimp/scene.h>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::ass {

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

void RenderContextSettings(Settings& ctx) {
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
    // ImGui::CheckboxFlags("Cache Locality Optimization", &mAiFlags,
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
    ImGui::CheckboxFlags("Combine identical materials"_j, &ctx.mAiFlags,
                         aiProcess_RemoveRedundantMaterials);
    ImGui::CheckboxFlags("Bake UV coord scale/rotate/translate"_j,
                         &ctx.mAiFlags, aiProcess_TransformUVCoords);
    ImGui::ColorEdit3("Model Tint"_j, glm::value_ptr(ctx.mModelTint));
  }
  if (ImGui::CollapsingHeader(
          (const char*)ICON_FA_PROJECT_DIAGRAM u8" Mesh Settings",
          ImGuiTreeNodeFlags_DefaultOpen)) {
    // aiProcess_FindDegenerates - TODO
    ImGui::CheckboxFlags("Remove degenerate triangles"_j, &ctx.mAiFlags,
                         aiProcess_FindDegenerates);
    // aiProcess_FindInvalidData - TODO
    ImGui::CheckboxFlags("Remove invalid data"_j, &ctx.mAiFlags,
                         aiProcess_FindInvalidData);
    // aiProcess_FixInfacingNormals
    ImGui::CheckboxFlags("Fix flipped normals"_j, &ctx.mAiFlags,
                         aiProcess_FixInfacingNormals);
    // aiProcess_OptimizeMeshes
    ImGui::CheckboxFlags("Optimize meshes"_j, &ctx.mAiFlags,
                         aiProcess_OptimizeMeshes);
    // aiProcess_OptimizeGraph
    ImGui::CheckboxFlags("Compress bones (for static scenes)"_j, &ctx.mAiFlags,
                         aiProcess_OptimizeGraph);
  }
  if (ImGui::CollapsingHeader(
          (const char*)ICON_FA_PROJECT_DIAGRAM u8" Data to Import",
          ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::CheckboxFlags(
        (const char*)ICON_FA_SORT_AMOUNT_UP u8" Vertex Normals",
        &ctx.mDataToInclude, aiComponent_NORMALS);
    // aiComponent_TANGENTS_AND_BITANGENTS: Unsupported
    ImGui::CheckboxFlags((const char*)ICON_FA_PALETTE u8" Vertex Colors",
                         &ctx.mDataToInclude, aiComponent_COLORS);
    ImGui::CheckboxFlags((const char*)ICON_FA_GLOBE u8" UV Maps",
                         &ctx.mDataToInclude, aiComponent_TEXCOORDS);
    ImGui::CheckboxFlags((const char*)ICON_FA_BONE u8" Bone Weights",
                         &ctx.mDataToInclude, aiComponent_BONEWEIGHTS);
    // aiComponent_ANIMATIONS: Unsupported
    // TODO: aiComponent_TEXTURES: Unsupported
    // aiComponent_LIGHTS: Unsupported
    // aiComponent_CAMERAS: Unsupported
    ImGui::CheckboxFlags((const char*)ICON_FA_PROJECT_DIAGRAM u8" Meshes",
                         &ctx.mDataToInclude, aiComponent_MESHES);
    ImGui::CheckboxFlags((const char*)ICON_FA_PAINT_BRUSH u8" Materials",
                         &ctx.mDataToInclude, aiComponent_MATERIALS);
  }
}

// Lifetime is tied to that of importer
const aiScene* ReadScene(kpi::IOTransaction& transaction, std::string path,
                         const Settings& settings, Assimp::Importer& importer) {
  AssimpLoggerScope g_assimplogger(
      std::make_unique<AssimpLogger>(transaction.callback, getFileShort(path)));

  // Only include components we asked for
  {
    const u32 exclusion_mask = FlipExclusionMask(settings.mDataToInclude);
    DebugPrintExclusionMask(exclusion_mask);
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, exclusion_mask);
  }
  u32 aiFlags = settings.mAiFlags;
  if (settings.mMagnification != 1.0f) {
    aiFlags |= aiProcess_GlobalScale;
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                              settings.mMagnification);
  }

  importer.ReadFileFromMemory(transaction.data.data(), transaction.data.size(),
                              aiProcess_PreTransformVertices, path.c_str());
  auto* pScene = importer.ApplyPostProcessing(aiFlags);

  if (!pScene) {
    return nullptr;
  }
  double unit_scale = 0.0;
  pScene->mMetaData->Get("UnitScaleFactor", unit_scale);

  // Handle custom units
  if (unit_scale != 0.0) {
    // FBX-only property: internally in centimeters
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                              (1.0 / unit_scale) / 100.0);
    importer.ApplyPostProcessing(aiProcess_GlobalScale);
  }
  return pScene;
}

Result<librii::rhst::SceneTree> ToSceneTree(const aiScene* scene,
                                            kpi::IOTransaction& transaction,
                                            const Settings& settings) {
  AssImporter importer(scene);
  importer.SetTransaction(transaction);
  return importer.Import(settings);
}

// Export-only
Result<librii::rhst::SceneTree> DoImport(std::string path,
                                         kpi::IOTransaction& transaction,
                                         const Settings& settings) {
  Assimp::Importer importer;
  auto* pScene = ReadScene(transaction, path, settings, importer);
  if (!pScene) {
    return std::unexpected("Assimp failed to read scene");
  }
  return ToSceneTree(pScene, transaction, settings);
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

    mContext->mScene =
        ReadScene(transaction, path, mContext->mSettings, *mContext->importer);
    if (!mContext->mScene) {
      transaction.state = kpi::TransactionState::Failure;
      return;
    }
    GotoNextState();
  }

  void StateWaitForTextureDependencies(kpi::IOTransaction& transaction) {
    auto sceneTree =
        ToSceneTree(mContext->mScene, transaction, mContext->mSettings);
    if (!sceneTree) {
      transaction.callback(kpi::IOMessageClass::Error, "Assimp Importer",
                           sceneTree.error());
      transaction.state = kpi::TransactionState::Failure;
      GotoExitState();
      return;
    }
    transaction.state = kpi::TransactionState::Complete;

    if (transaction.state == kpi::TransactionState::Complete) {
      transaction.state = kpi::TransactionState::Failure;
      riistudio::rhst::CompileRHST(*sceneTree, transaction);
    }

    switch (transaction.state) {
    case kpi::TransactionState::Complete:
      return;
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
    if (transaction.state == kpi::TransactionState::ConfigureProperties) {
      return;
    }
  }
  if (mContext->state == State::WaitForSettings) {
    StateWaitForSettings(transaction);
    // There are no textures, so fallthrough to final state
  }
  if (mContext->state == State::WaitForTextureDependencies) {
    StateWaitForTextureDependencies(transaction);
    return;
  }
}

void AssimpPlugin::render() {
  if (mContext) {
    RenderContextSettings(mContext->mSettings);
  } else {
    ImGui::Text("There is no context");
  }
}

} // namespace riistudio::ass
