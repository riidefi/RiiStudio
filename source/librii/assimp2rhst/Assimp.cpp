// Assimp importer.
#include "Assimp.hpp"

#include "Importer.hpp"
#include "LogScope.hpp"
#include "Logger.hpp"
#include "SupportedFiles.hpp"
#include "Utility.hpp"
#include <plugins/rhst/RHSTImporter.hpp>
#include <vendor/assimp/DefaultLogger.hpp>
#include <vendor/assimp/Importer.hpp>
#include <vendor/assimp/scene.h>
#include <vendor/assimp/version.h>

namespace librii::assimp2rhst {

// Lifetime is tied to that of importer
const aiScene* ReadScene(KCallback callback, std::span<const u8> file,
                         std::string path, const Settings& settings,
                         Assimp::Importer& importer) {
  AssimpLoggerScope g_assimplogger(
      std::make_unique<AssimpLogger>(callback, getFileShort(path)));

  rsl::info("Assimp: {}", aiGetLegalString());
  rsl::info("Assimp v{}.{}.{} ({})", aiGetVersionMajor(), aiGetVersionMinor(),
            aiGetVersionPatch(), aiGetBranchName());

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

  importer.ReadFileFromMemory(file.data(), file.size(), 0, path.c_str());
  if (settings.mIgnoreRootTransform) {
    importer.GetScene()->mRootNode->mTransformation = {};
  }
  auto* pScene = importer.ApplyPostProcessing(aiProcess_PreTransformVertices);
  if (!pScene) {
    return nullptr;
  }
  pScene = importer.ApplyPostProcessing(aiFlags);
  if (!pScene) {
    return nullptr;
  }
  double unit_scale = 0.0;
  pScene->mMetaData->Get("UnitScaleFactor", unit_scale);
  rsl::info("UnitScaleFactor: {}. (If previous != 0, then setting GLOBAL_SCALE "
            "to {}). Was {} (default {})",
            unit_scale, (1.0 / unit_scale) / 100.0,
            importer.GetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY),
            AI_CONFIG_GLOBAL_SCALE_FACTOR_DEFAULT);
  rsl::info("Magnification: {}", settings.mMagnification);

  // Handle custom units
  if (unit_scale != 0.0) {
    // FBX-only property: internally in centimeters
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY,
                              settings.mMagnification * (1.0 / unit_scale) /
                                  100.0);
    importer.ApplyPostProcessing(aiProcess_GlobalScale);
  }
  return pScene;
}

Result<librii::rhst::SceneTree> ToSceneTree(const aiScene* scene,
                                            const Settings& settings) {
  lra::Scene scn = lra::ReadScene(*scene);
  lra::DropNonTriangularMeshes(scn);
  lra::MakeMeshNamesUnique(scn);
  AssImporter importer(&scn);
  return importer.Import(settings);
}

// Export-only
Result<librii::rhst::SceneTree> DoImport(std::string path, KCallback callback,
                                         std::span<const u8> file,
                                         const Settings& settings) {
  Assimp::Importer importer;
  auto* pScene = ReadScene(callback, file, path, settings, importer);
  if (!pScene) {
    return std::unexpected("Assimp failed to read scene");
  }
  return ToSceneTree(pScene, settings);
}

} // namespace librii::assimp2rhst
