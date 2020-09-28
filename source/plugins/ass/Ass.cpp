// Assimp importer.

#include "AssImporter.hpp"
#include "AssLogger.hpp"
#include "Utility.hpp"
#include <algorithm>
#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Node.hpp>
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
      aiProcess_GenSmoothNormals | aiProcess_ValidateDataStructure |
      aiProcess_RemoveRedundantMaterials | aiProcess_FindDegenerates |
      aiProcess_FindInvalidData | aiProcess_FindInstances |
      aiProcess_OptimizeMeshes | aiProcess_Debone | aiProcess_OptimizeGraph;
  static constexpr u32 AlwaysFlags =
      aiProcess_Triangulate | aiProcess_SortByPType |
      aiProcess_PopulateArmatureData | aiProcess_GenUVCoords |
      aiProcess_GenBoundingBoxes | aiProcess_FlipUVs |
      aiProcess_FlipWindingOrder;
  u32 ass_flags = AlwaysFlags | DefaultFlags;
  float mMagnification = 1.0f;

  // if mGenerateMipMaps, create mip levels until < mMinMipDimension or >
  // mMaxMipCount
  bool mGenerateMipMaps = true;
  int mMinMipDimension = 32;
  int mMaxMipCount = 5;
  // Set stencil outline if alpha
  bool mAutoTransparent = true;
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

    if (mMagnification != 1.0f) {
      ass_flags |= aiProcess_GlobalScale;
      importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                                 mMagnification);
    }
    const auto* pScene = importer->ReadFileFromMemory(transaction.data.data(),
                                                      transaction.data.size(),
                                                      ass_flags, path.c_str());
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
	unresolved =
        helper->PrepareAss(mGenerateMipMaps, mMinMipDimension, mMaxMipCount);

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
    helper->ImportAss(additional_textures, mGenerateMipMaps, mMinMipDimension,
                      mMaxMipCount, mAutoTransparent);
    transaction.state = kpi::TransactionState::Complete;
    state = State::Completed;
    // And we die~
  }
}

void AssReader::render() {
  if (ImGui::CollapsingHeader("Importing Settings",
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

    //
    // Configure
    //
    // aiProcess_RemoveComponent  - TODO
    // aiProcess_GenNormals - TODO
    // aiProcess_GenSmoothNormals - TODO
    // aiProcess_SplitLargeMeshes - We probably don't need this?
    // aiProcess_PreTransformVertices - Doubt this is desirable..
    // aiProcess_LimitBoneWeights - TODO
    // aiProcess_ValidateDataStructure
    ImGui::CheckboxFlags("Data Validation", &ass_flags,
                         aiProcess_ValidateDataStructure);
    // aiProcess_ImproveCacheLocality
    // TODO
    // ImGui::CheckboxFlags("Cache Locality Optimization", &ass_flags,
    //                      aiProcess_ImproveCacheLocality);
    // aiProcess_RemoveRedundantMaterials
    ImGui::CheckboxFlags("Material Deduplication", &ass_flags,
                         aiProcess_RemoveRedundantMaterials);
    // aiProcess_FixInfacingNormals
    ImGui::CheckboxFlags("Fix flipped normals", &ass_flags,
                         aiProcess_FixInfacingNormals);
    // aiProcess_FindDegenerates - TODO
    ImGui::CheckboxFlags("Remove degenerate triangles", &ass_flags,
                         aiProcess_FindDegenerates);
    // aiProcess_FindInvalidData - TODO
    ImGui::CheckboxFlags("Remove invalid data", &ass_flags,
                         aiProcess_FindInvalidData);
    // aiProcess_TransformUVCoords - Undesirable?
    ImGui::CheckboxFlags("Bake material-transformed UV coords", &ass_flags,
                         aiProcess_TransformUVCoords);
    // aiProcess_FindInstances - TODO
    // aiProcess_OptimizeMeshes
    ImGui::CheckboxFlags("Optimize meshes", &ass_flags,
                         aiProcess_OptimizeMeshes);
    // aiProcess_OptimizeGraph
    ImGui::CheckboxFlags("Compress bones (for static scenes)", &ass_flags,
                         aiProcess_OptimizeGraph);
    // aiProcess_SplitByBoneCount - ?
    // aiProcess_Debone - TODO
    // aiProcess_GlobalScale
    ImGui::InputFloat("Magnification", &mMagnification);
    // aiProcess_ForceGenNormals - TODO
    // aiProcess_DropNormals - TODO
  }

  if (ImGui::CollapsingHeader("BMD Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Generate Mipmaps", &mGenerateMipMaps);
    {
      util::ConditionalActive g(mGenerateMipMaps);
      ImGui::Indent(50);
      ImGui::SliderInt("Minimum mipmap dimension.", &mMinMipDimension, 1, 512);
      // round down to last power of two
      const int old = mMinMipDimension;
      int res = 0;
      while (mMinMipDimension >>= 1)
        ++res;
      mMinMipDimension = (1 << res);
      // round up
      if ((mMinMipDimension << 1) - old < old - mMinMipDimension)
        mMinMipDimension <<= 1;

      ImGui::SliderInt("Maximum number of mipmaps.", &mMaxMipCount, 0, 8);

      ImGui::Indent(-50);
    }
    ImGui::Checkbox("Automatically set transparent materials",
                    &mAutoTransparent);
  }
}

} // namespace riistudio::ass
