#include "AssimpImporter.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <rsl/Defer.hpp>

#include <frontend/legacy_editor/EditorWindow.hpp>
#include <frontend/root.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>

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

  if (ImGui::Checkbox("Emulate BrawlBox Model Scale",
                      &ctx.mIgnoreRootTransform)) {
    // Checkbox clicked, no need to show popup
  } else if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
    ImGui::TextUnformatted("This is incorrect and goes against the COLLADA "
                           "(.dae) specification but may be useful");
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }

  // aiProcess_ForceGenNormals - TODO
  // aiProcess_DropNormals - TODO

  int import_config = 0;
  ImGui::Combo("Import Config"_j, &import_config, "World (single bone)\0\0");
}

void MipMapSettings(librii::assimp2rhst::Settings& ctx) {
  ImGui::Checkbox("Generate Mipmaps"_j, &ctx.mGenerateMipMaps);
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
    ImGui::TextUnformatted(
        "Mipmaps are pre-calculated, optimized texture versions at different "
        "resolutions that improve rendering quality and performance. Strongly "
        "recommended to keep enabled for better visual quality at different "
        "distances.");
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
  {
    util::ConditionalActive g(ctx.mGenerateMipMaps);
    ImGui::Indent(50);
    ImGui::SliderInt("Minimum mipmap dimension."_j, &ctx.mMinMipDimension, 1,
                     512);
    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
      ImGui::TextUnformatted("Default: 32 pixels. We won't generate mipmap LOD "
                             "levels smaller than this dimension.");
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
    ctx.mMinMipDimension =
        librii::assimp2rhst::ClampMipMapDimension(ctx.mMinMipDimension);

    ImGui::SliderInt("Maximum number of mipmaps."_j, &ctx.mMaxMipCount, 0, 8);
    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);
      ImGui::TextUnformatted(
          "Default: 5. We won't generate more than this many LOD levels, even "
          "if the smallest LOD level is above our minimum mip dimension (for "
          "instance on a very large image).");
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }

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
  if (ImGui::BeginTable("DataToImportTable", 2, ImGuiTableFlags_Borders)) {
    ImGui::TableSetupColumn("Component", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Include", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(
        (const char*)ICON_FA_SORT_AMOUNT_UP u8" Vertex Normals");
    ImGui::TableNextColumn();
    ImGui::CheckboxFlags("##normals", &ctx.mDataToInclude, aiComponent_NORMALS);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted((const char*)ICON_FA_PALETTE u8" Vertex Colors");
    ImGui::TableNextColumn();
    ImGui::CheckboxFlags("##colors", &ctx.mDataToInclude, aiComponent_COLORS);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted((const char*)ICON_FA_GLOBE u8" UV Maps");
    ImGui::TableNextColumn();
    ImGui::CheckboxFlags("##uvmaps", &ctx.mDataToInclude,
                         aiComponent_TEXCOORDS);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted((const char*)ICON_FA_BONE u8" Bone Weights");
    ImGui::TableNextColumn();
    ImGui::CheckboxFlags("##bones", &ctx.mDataToInclude,
                         aiComponent_BONEWEIGHTS);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted((const char*)ICON_FA_PROJECT_DIAGRAM u8" Meshes");
    ImGui::TableNextColumn();
    ImGui::CheckboxFlags("##meshes", &ctx.mDataToInclude, aiComponent_MESHES);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted((const char*)ICON_FA_PAINT_BRUSH u8" Materials");
    ImGui::TableNextColumn();
    ImGui::CheckboxFlags("##materials", &ctx.mDataToInclude,
                         aiComponent_MATERIALS);

    ImGui::EndTable();
  }

  // Note: These components are unsupported:
  // - aiComponent_TANGENTS_AND_BITANGENTS
  // - aiComponent_ANIMATIONS
  // - aiComponent_TEXTURES (TODO)
  // - aiComponent_LIGHTS
  // - aiComponent_CAMERAS
}
void MeshOptimizationSettings(librii::assimp2rhst::Settings& ctx,
                              bool& tristrip) {
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

class AssimpEditorTabSheet {
public:
  void Draw(std::function<void(void)> draw) {
    std::vector<std::string> icons{(const char*)ICON_FA_EXPAND_ARROWS_ALT,
                                   (const char*)ICON_FA_IMAGES,
                                   (const char*)ICON_FA_BRUSH,
                                   (const char*)ICON_FA_PROJECT_DIAGRAM,
                                   (const char*)ICON_FA_TRASH,
                                   (const char*)ICON_FA_PROJECT_DIAGRAM};
    std::vector<const char*> labels{"Scale",
                                    "Mip Maps",
                                    "Material Settings",
                                    "Pre-BRRES Optimization",
                                    "Data to Import",
                                    "BRRES Optimization"};
    std::vector<ImVec4> iconColors{
        ImVec4(0.5f, 0.8f, 1.0f, 1.0f), // Blue for Scale
        ImVec4(1.0f, 0.5f, 0.8f, 1.0f), // Pink for Mip Maps
        ImVec4(0.8f, 0.6f, 0.2f, 1.0f), // Orange for Material Settings
        ImVec4(0.2f, 0.8f, 0.4f, 1.0f), // Green for Pre-BRRES Optimization
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f), // Red for Data to Import
        ImVec4(0.4f, 0.7f, 0.9f, 1.0f)  // Light blue for BRRES Optimization
    };

    std::function<bool(int)> drawTab = [&](int index) {
      switch (index) {
      case 0:
        ImporterSettings(ctx);
        break;
      case 1:
        MipMapSettings(ctx);
        break;
      case 2:
        MaterialSettings(ctx);
        break;
      case 3:
        MeshSettings(ctx);
        break;
      case 4:
        MaskSettings(ctx);
        break;
      case 5:
        MeshOptimizationSettings(ctx, tristrip);
        break;
      default:
        return false;
      }
      return true;
    };
    std::function<void(int)> drawTabTitle = [&](int index) {
      bool isNonDefault = false;
      switch (index) {
      case 0:
        isNonDefault = ctx.mMagnification != 1.0f || ctx.mIgnoreRootTransform;
        break;
      case 1:
        isNonDefault = !ctx.mGenerateMipMaps || ctx.mMinMipDimension != 32 ||
                       ctx.mMaxMipCount != 5;
        break;
      case 2:
        isNonDefault =
            !ctx.mAutoTransparent ||
            (ctx.mAiFlags & aiProcess_RemoveRedundantMaterials) == 0 ||
            ctx.mAiFlags & aiProcess_TransformUVCoords ||
            ctx.mModelTint != glm::vec3(1.0f);
        break;
      case 3:
        isNonDefault = ctx.mAiFlags != (librii::assimp2rhst::AlwaysFlags |
                                        librii::assimp2rhst::DefaultFlags);
        break;
      case 4:
        isNonDefault =
            ctx.mDataToInclude != librii::assimp2rhst::DefaultInclusionMask();
        break;
      case 5:
        isNonDefault = !tristrip;
        break;
      }

      // Draw colored icon
      ImGui::PushStyleColor(ImGuiCol_Text,
                            ImGui::ColorConvertFloat4ToU32(iconColors[index]));
      ImGui::TextUnformatted(icons[index].c_str());
      ImGui::PopStyleColor();

      ImGui::SameLine(0, 0);

      // Draw label with conditional yellow color for non-default settings
      if (isNonDefault) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
        ImGui::TextUnformatted(labels[index]);
        ImGui::PopStyleColor();
      } else {
        ImGui::Text(" %s", labels[index]);
      }
    };

    std::vector<std::string> titles(icons.size());
    for (size_t i = 0; i < icons.size(); i++) {
      titles[i] = std::string(icons[i]) + std::string(" ") + labels[i];
    }

    DrawPropertyEditorWidgetV2_Header(m_state, false);
    DrawPropertyEditorWidgetV3_Body(m_state, drawTab, drawTabTitle, titles);
  }
  PropertyEditorState& m_state;
  librii::assimp2rhst::Settings& ctx;
  bool& tristrip;
};

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

  AssimpEditorTabSheet sheet{m_propState, ctx, tristrip};
  sheet.Draw([]() {});
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
