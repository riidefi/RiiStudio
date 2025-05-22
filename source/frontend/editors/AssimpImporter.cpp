#include "AssimpImporter.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <rsl/Defer.hpp>

#include <frontend/legacy_editor/EditorWindow.hpp>
#include <frontend/root.hpp>

namespace riistudio::frontend {

namespace {
void ImporterSettings(librii::assimp2rhst::Settings& ctx) {
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
  ImGui::Checkbox("Emulate BrawlBox Model Scale (This is incorrect and goes "
                  "against the COLLADA (.dae) specification but may be useful)",
                  &ctx.mIgnoreRootTransform);
  // aiProcess_ForceGenNormals - TODO
  // aiProcess_DropNormals - TODO

  int import_config = 0;
  ImGui::Combo("Import Config"_j, &import_config, "World (single bone)\0\0");
}

void MipMapSettings(librii::assimp2rhst::Settings& ctx) {
  ImGui::Checkbox("Generate Mipmaps"_j, &ctx.mGenerateMipMaps);
  {
    util::ConditionalActive g(ctx.mGenerateMipMaps);
    ImGui::Indent(50);
    ImGui::SliderInt("Minimum mipmap dimension."_j, &ctx.mMinMipDimension, 1,
                     512);
    ctx.mMinMipDimension =
        librii::assimp2rhst::ClampMipMapDimension(ctx.mMinMipDimension);

    ImGui::SliderInt("Maximum number of mipmaps."_j, &ctx.mMaxMipCount, 0, 8);

    ImGui::Indent(-50);
  }
}

void MaterialSettings(librii::assimp2rhst::Settings& ctx) {
  ImGui::Checkbox(
      "Detect transparent textures, and configure materials accordingly."_j,
      &ctx.mAutoTransparent);
  ImGui::CheckboxFlags("Combine identical materials"_j, &ctx.mAiFlags,
                       aiProcess_RemoveRedundantMaterials);
  ImGui::CheckboxFlags("Bake UV coord scale/rotate/translate"_j, &ctx.mAiFlags,
                       aiProcess_TransformUVCoords);
  ImGui::ColorEdit3("Model Tint"_j, glm::value_ptr(ctx.mModelTint));
}

void MeshSettings(librii::assimp2rhst::Settings& ctx) {
  if (ImGui::BeginChild("HelpBox2",
                        ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 1.5f),
                        true, ImGuiWindowFlags_NoScrollbar)) {
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f),
                       (const char*)ICON_FA_BOOK_OPEN);
    ImGui::SameLine();
    ImGui::Text("Documentation (Developer): ");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("postprocess.h (assimp)",
                           "https://github.com/assimp/assimp/blob/master/"
                           "include/assimp/postprocess.h#L396");
  }
  ImGui::EndChild();
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
  ImGui::CheckboxFlags("Simplify multi-influence weighting into singlebinds"_j,
                       &ctx.mAiFlags, aiProcess_Debone);
  ImGui::CheckboxFlags("Regenerate scene hierarchy", &ctx.mAiFlags,
                       aiProcess_PreTransformVertices);
}

void MaskSettings(librii::assimp2rhst::Settings& ctx) {
  ImGui::CheckboxFlags((const char*)ICON_FA_SORT_AMOUNT_UP u8" Vertex Normals",
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
void MeshOptimizationSettings(librii::assimp2rhst::Settings& ctx, bool& tristrip) {
  if (ImGui::BeginChild("HelpBox3",
                        ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 1.5f),
                        true, ImGuiWindowFlags_NoScrollbar)) {
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f),
                       (const char*)ICON_FA_BOOK_OPEN);
    ImGui::SameLine();
    ImGui::Text("Documentation (Developer): ");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL(
        "https://wiki.tockdom.com/wiki/"
        "Creating_a_BRRES_with_RiiStudio#Technical_Explanation");
  }
  ImGui::EndChild();
  ImGui::Checkbox(
      (const char*)ICON_FA_PROJECT_DIAGRAM u8" Triangle Strip Meshes?",
      &tristrip);
}
} // namespace

void AssimpEditorPropertyGrid::Draw(librii::assimp2rhst::Settings& ctx,
                                    bool& tristrip) {
  if (ImGui::BeginChild("HelpBox",
                        ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 1.5f),
                        true, ImGuiWindowFlags_NoScrollbar)) {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
                       (const char*)ICON_FA_BOOK_OPEN);
    ImGui::SameLine();
    ImGui::Text("Documentation: ");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("https://wiki.tockdom.com/wiki/"
                           "Creating_a_BRRES_with_RiiStudio#RiiStudio");
  }
  ImGui::EndChild();

  if (ImGui::CollapsingHeader("Importing Settings"_j,
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImporterSettings(ctx);
  }

  if (ImGui::CollapsingHeader((const char*)ICON_FA_IMAGES u8" Mip Maps",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    MipMapSettings(ctx);
  }
  if (ImGui::CollapsingHeader((const char*)ICON_FA_BRUSH u8" Material Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    MaterialSettings(ctx);
  }
  if (ImGui::CollapsingHeader(
          (const char*)ICON_FA_PROJECT_DIAGRAM u8" Mesh Settings",
          ImGuiTreeNodeFlags_DefaultOpen)) {
    MeshSettings(ctx);
  }
  if (ImGui::CollapsingHeader(
          (const char*)ICON_FA_PROJECT_DIAGRAM u8" Data to Import",
          ImGuiTreeNodeFlags_DefaultOpen)) {
    MaskSettings(ctx);
  }
  if (ImGui::CollapsingHeader(
          (const char*)ICON_FA_PROJECT_DIAGRAM u8" Mesh Optimization",
          ImGuiTreeNodeFlags_DefaultOpen)) {
    MeshOptimizationSettings(ctx, tristrip);
  }
}

void AssimpImporter::draw_() {
  setName("Assimp Importer: " + m_path);

  auto full = ImVec2{ImGui::GetContentRegionAvail().x, 0.0f};

  if (m_state == State::PickFormat) {
    ImGui::Text("Pick which file format to create.");
    if (ImGui::Button("BRRES Model", full)) {
      m_result = std::make_unique<riistudio::g3d::Collection>();
      m_extension = ".brres";
      m_state = State::Settings;
    }
    if (ImGui::Button("BMD Model", full)) {
      m_result = std::make_unique<riistudio::j3d::Collection>();
      m_extension = ".bmd";
      m_state = State::Settings;
    }
  }
  if (m_state == State::Settings) {
    auto h = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginChild("SW", ImVec2{0.0f, h - 26.0f})) {
      m_grid.Draw(m_settings, tristrip);
      ImGui::EndChild();
    }
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 255, 0, 100));
    RSL_DEFER(ImGui::PopStyleColor());
    if (ImGui::Button("Next", full)) {
      launchImporter();
      m_state = State::Wait;
    }
  }
  if (m_state == State::Wait) {
    auto status = uiThreadGetStatus();
    ImGui::TextUnformatted(status.tagline.c_str());
    ImGui::ProgressBar(status.progress);
  }
  // Moved here by importer thread
  else if (m_state == State::Done) {
    ImGui::Text("Done");
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 255, 0, 100));
    RSL_DEFER(ImGui::PopStyleColor());
    if (ImGui::Button("Next", full)) {
      if (!m_result.get()) {
        m_err += "\nResult has already been moved.";
        m_state = State::Fail;
        return;
      }
      std::unique_ptr<kpi::INode> node(
          dynamic_cast<kpi::INode*>(m_result.release()));
      if (!node) {
        m_err += "\nCannot attach editor window :(";
        m_state = State::Fail;
        return;
      }
      auto path = std::filesystem::path(m_path);
      path.replace_extension(m_extension);
      if (auto* g = dynamic_cast<g3d::Collection*>(node.get())) {
        node.release();
        std::unique_ptr<g3d::Collection> gp(g);
        auto ed = std::make_unique<BRRESEditor>(path.string());
        ed->attach(std::move(gp));
        RootWindow::spInstance->attachWindow(std::move(ed));
      } else if (auto* j = dynamic_cast<j3d::Collection*>(node.get())) {
        node.release();
        std::unique_ptr<j3d::Collection> jp(j);
        auto ed = std::make_unique<BMDEditor>(path.string());
        ed->attach(std::move(jp));
        RootWindow::spInstance->attachWindow(std::move(ed));
      }
      close();
    }
  } else if (m_state == State::Fail) {
    ImGui::Text("Failed");
    ImGui::TextUnformatted(m_err.c_str());
  }
}

} // namespace riistudio::frontend
