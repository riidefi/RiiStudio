// Assimp importer.

#include "AssImporter.hpp"
#include "AssLogger.hpp"
#include "Utility.hpp"
#include <algorithm>
#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Plugins.hpp>
#include <core/util/gui.hpp>
#include <map>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/j3d/Scene.hpp>
#include <span>
#include <unordered_map>
#include <vendor/assimp/DefaultLogger.hpp>
#include <vendor/assimp/Importer.hpp>
#include <vendor/assimp/postprocess.h>
#include <vendor/assimp/scene.h>
#include <vendor/fa5/IconsFontAwesome5.h>
#undef min

namespace riistudio::ass {

static constexpr std::array<std::string_view, 4> supported_endings = {
    ".dae", ".obj", ".fbx", ".smd"};

struct AssReader {
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

  std::string canRead(const std::string& file,
                      oishii::BinaryReader& reader) const;
  void read(kpi::IOTransaction& transaction);
  void render();

  State state = State::Unengaged;
  // Hack (we know importer will not be copied):
  // Won't be necessary when IBinaryDeserializable is split into Factory and
  // Instance, and Instance does not require copyable.
  std::shared_ptr<Assimp::Importer> importer =
      std::make_shared<Assimp::Importer>();
  std::optional<AssImporter> helper = std::nullopt;
  std::set<std::pair<std::size_t, std::string>> unresolved;
  std::vector<std::pair<std::size_t, std::vector<u8>>> additional_textures;

  static constexpr u32 DefaultFlags =
      aiProcess_GenSmoothNormals | aiProcess_RemoveRedundantMaterials |
      aiProcess_FindDegenerates | aiProcess_FindInvalidData |
      aiProcess_OptimizeMeshes | aiProcess_Debone | aiProcess_OptimizeGraph |
      aiProcess_RemoveComponent;
  static constexpr u32 AlwaysFlags =
      aiProcess_JoinIdenticalVertices | aiProcess_ValidateDataStructure |
      aiProcess_Triangulate | aiProcess_SortByPType |
      aiProcess_PopulateArmatureData | aiProcess_GenUVCoords |
      aiProcess_GenBoundingBoxes | aiProcess_FlipUVs |
      aiProcess_FlipWindingOrder;
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
  u32 data_to_include = aiComponent_NORMALS | aiComponent_COLORS |
                        aiComponent_TEXCOORDS | aiComponent_TEXTURES |
                        aiComponent_MESHES | aiComponent_MATERIALS;
  u32 data_to_exclude() const {
    return (~data_to_include) & ((aiComponent_MATERIALS << 1) - 1);
  }
};

kpi::Register<AssReader, kpi::Reader> AssInstaller;

std::string AssReader::canRead(const std::string& file,
                               oishii::BinaryReader& reader) const {
  std::string lower_file(file);
  for (auto& c : lower_file)
    c = tolower(c);
  if (std::find_if(supported_endings.begin(), supported_endings.end(),
                   [&](const std::string_view& ending) {
                     return lower_file.ends_with(ending);
                   }) == supported_endings.end()) {
    return "";
  }

  return typeid(lib3d::Scene).name();
}

void AssReader::read(kpi::IOTransaction& transaction) {
  // Ask for properties
  if (state == State::Unengaged) {
    state = State::WaitForSettings;
    transaction.state = kpi::TransactionState::ConfigureProperties;
    return;
  }
  // Expect settings to have been configured.
  // Load assimp file and check for texture dependencies
  if (state == State::WaitForSettings) {

    std::string path(transaction.data.getProvider()->getFilePath());

    // Assimp requires it be heap allocated, so it can delete it for us.
    AssLogger* logger = new AssLogger(transaction.callback, getFileShort(path));
    Assimp::DefaultLogger::set(logger);

    const u32 exclusion_mask = data_to_exclude();
    if (exclusion_mask & aiComponent_NORMALS)
      puts("Excluding normals");
    if (exclusion_mask & aiComponent_TANGENTS_AND_BITANGENTS)
      puts("Excluding tangents/bitangents");
    if (exclusion_mask & aiComponent_COLORS)
      puts("Excluding colors");
    if (exclusion_mask & aiComponent_TEXCOORDS)
      puts("Excluding uvs");
    if (exclusion_mask & aiComponent_BONEWEIGHTS)
      puts("Excluding boneweights");
    if (exclusion_mask & aiComponent_ANIMATIONS)
      puts("Excluding animations");
    if (exclusion_mask & aiComponent_TEXTURES)
      puts("Excluding textures");
    if (exclusion_mask & aiComponent_LIGHTS)
      puts("Excluding lights");
    if (exclusion_mask & aiComponent_CAMERAS)
      puts("Excluding cameras");
    if (exclusion_mask & aiComponent_MESHES)
      puts("Excluding meshes");
    if (exclusion_mask & aiComponent_MATERIALS)
      puts("Excluding materials");
    importer->SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, exclusion_mask);

    if (mMagnification != 1.0f) {
      ass_flags |= aiProcess_GlobalScale;
      importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                                 mMagnification);
    }

    importer->ReadFileFromMemory(transaction.data.data(),
                                 transaction.data.size(),
                                 aiProcess_PreTransformVertices, path.c_str());
    const auto* pScene = importer->ApplyPostProcessing(ass_flags);

    if (!pScene) {
      transaction.state = kpi::TransactionState::Failure;
      return;
    }
    double unit_scale = 0.0;
    pScene->mMetaData->Get("UnitScaleFactor", unit_scale);

    // Handle custom units
    if (unit_scale != 0.0) {
      // FBX-only property: internally in centimeters
      importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                                 (1.0 / unit_scale) / 100.0);
      importer->ApplyPostProcessing(aiProcess_GlobalScale);
    }

    // this calls `delete` on logger
    Assimp::DefaultLogger::set(nullptr);
    if (pScene == nullptr) {
      // We will never be called again..
      // transaction.callback(kpi::IOMessageClass::Error, getFileShort(path),
      //                      importer->GetErrorString());
      transaction.state = kpi::TransactionState::Failure;
      return;
    }
    helper.emplace(pScene, &transaction.node);
    std::vector<std::string> mat_merge;
    unresolved = helper->PrepareAss(mGenerateMipMaps, mMinMipDimension,
                                    mMaxMipCount, path);

    state = State::WaitForTextureDependencies;
    // This step might be optional
    if (!unresolved.empty()) {
      transaction.state = kpi::TransactionState::ResolveDependencies;
      transaction.resolvedFiles.resize(unresolved.size());
      transaction.unresolvedFiles.reserve(unresolved.size());
      for (auto& missing : unresolved)
        transaction.unresolvedFiles.push_back(missing.second);
      return;
    }
  }
  if (state == State::WaitForTextureDependencies) {
    if (!unresolved.empty()) {
      for (std::size_t i = 0; i < transaction.resolvedFiles.size(); ++i) {
        auto& found = transaction.resolvedFiles[i];
        if (found.empty())
          continue;
        additional_textures.emplace_back(i, std::move(found));
      }
    }
    helper->SetTransaction(transaction);
    helper->ImportAss(additional_textures, mGenerateMipMaps, mMinMipDimension,
                      mMaxMipCount, mAutoTransparent,
                      glm::vec3(model_tint[0], model_tint[1], model_tint[2]));
    if (transaction.state == kpi::TransactionState::Failure) {
      state = State::Completed;
      return;
    }
    transaction.state = kpi::TransactionState::Complete;
    state = State::Completed;
    // And we die~
  }
}

void AssReader::render() {
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
    ImGui::InputFloat("Model Scale"_j, &mMagnification);
    // aiProcess_ForceGenNormals - TODO
    // aiProcess_DropNormals - TODO

    int import_config = 0;
    ImGui::Combo("Import Config"_j, &import_config, "World (single bone)\0\0");
  }

  if (ImGui::CollapsingHeader((const char*)ICON_FA_IMAGES u8" Mip Maps",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Generate Mipmaps"_j, &mGenerateMipMaps);
    {
      util::ConditionalActive g(mGenerateMipMaps);
      ImGui::Indent(50);
      ImGui::SliderInt("Minimum mipmap dimension."_j, &mMinMipDimension, 1,
                       512);
      // round down to last power of two
      const int old = mMinMipDimension;
      int res = 0;
      while (mMinMipDimension >>= 1)
        ++res;
      mMinMipDimension = (1 << res);
      // round up
      if ((mMinMipDimension << 1) - old < old - mMinMipDimension)
        mMinMipDimension <<= 1;

      ImGui::SliderInt("Maximum number of mipmaps."_j, &mMaxMipCount, 0, 8);

      ImGui::Indent(-50);
    }
  }
  if (ImGui::CollapsingHeader((const char*)ICON_FA_BRUSH u8" Material Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox(
        "Detect transparent textures, and configure materials accordingly."_j,
        &mAutoTransparent);
    ImGui::CheckboxFlags("Combine identical materials"_j, &ass_flags,
                         aiProcess_RemoveRedundantMaterials);
    ImGui::CheckboxFlags("Bake UV coord scale/rotate/translate"_j, &ass_flags,
                         aiProcess_TransformUVCoords);
    ImGui::ColorEdit3("Model Tint"_j, model_tint.data());
  }
  if (ImGui::CollapsingHeader(
          (const char*)ICON_FA_PROJECT_DIAGRAM u8" Mesh Settings",
          ImGuiTreeNodeFlags_DefaultOpen)) {
    // aiProcess_FindDegenerates - TODO
    ImGui::CheckboxFlags("Remove degenerate triangles"_j, &ass_flags,
                         aiProcess_FindDegenerates);
    // aiProcess_FindInvalidData - TODO
    ImGui::CheckboxFlags("Remove invalid data"_j, &ass_flags,
                         aiProcess_FindInvalidData);
    // aiProcess_FixInfacingNormals
    ImGui::CheckboxFlags("Fix flipped normals"_j, &ass_flags,
                         aiProcess_FixInfacingNormals);
    // aiProcess_OptimizeMeshes
    ImGui::CheckboxFlags("Optimize meshes"_j, &ass_flags,
                         aiProcess_OptimizeMeshes);
    // aiProcess_OptimizeGraph
    ImGui::CheckboxFlags("Compress bones (for static scenes)"_j, &ass_flags,
                         aiProcess_OptimizeGraph);
  }
  if (ImGui::CollapsingHeader(
          (const char*)ICON_FA_PROJECT_DIAGRAM u8" Data to Import",
          ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::CheckboxFlags(
        (const char*)ICON_FA_SORT_AMOUNT_UP u8" Vertex Normals",
        &data_to_include, aiComponent_NORMALS);
    // aiComponent_TANGENTS_AND_BITANGENTS: Unsupported
    ImGui::CheckboxFlags((const char*)ICON_FA_PALETTE u8" Vertex Colors",
                         &data_to_include, aiComponent_COLORS);
    ImGui::CheckboxFlags((const char*)ICON_FA_GLOBE u8" UV Maps",
                         &data_to_include, aiComponent_TEXCOORDS);
    ImGui::CheckboxFlags((const char*)ICON_FA_BONE u8" Bone Weights",
                         &data_to_include, aiComponent_BONEWEIGHTS);
    // aiComponent_ANIMATIONS: Unsupported
    // TODO: aiComponent_TEXTURES: Unsupported
    // aiComponent_LIGHTS: Unsupported
    // aiComponent_CAMERAS: Unsupported
    ImGui::CheckboxFlags((const char*)ICON_FA_PROJECT_DIAGRAM u8" Meshes",
                         &data_to_include, aiComponent_MESHES);
    ImGui::CheckboxFlags((const char*)ICON_FA_PAINT_BRUSH u8" Materials",
                         &data_to_include, aiComponent_MATERIALS);
  }
}

} // namespace riistudio::ass
