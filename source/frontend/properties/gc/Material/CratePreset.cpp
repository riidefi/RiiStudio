#include <vendor/FileDialogues.hpp>

#include "Common.hpp"

#include <core/kpi/ActionMenu.hpp>
#include <core/kpi/RichNameManager.hpp>
#include <core/util/oishii.hpp>
#include <core/util/timestamp.hpp>
#include <frontend/widgets/IconManager.hpp>
#include <librii/crate/g3d_crate.hpp>
#include <librii/image/TextureExport.hpp>
#include <plate/Platform.hpp>
#include <plugins/g3d/collection.hpp>
#include <plugins/rhst/RHSTImporter.hpp>
#include <rsl/Defer.hpp>
#include <rsl/FsDialog.hpp>
#include <rsl/Stb.hpp>
#include <vendor/stb_image.h>

namespace libcube::UI {

void OverwriteWithG3dTex(libcube::Texture& tex,
                         const librii::g3d::TextureData& rep) {
  // Using more general approach to support J3D too
  // static_cast<librii::g3d::TextureData&>(scn.getTextures().add()) =
  //     *replacement;

  tex.setName(rep.name);
  tex.setTextureFormat(rep.format);
  tex.setWidth(rep.width);
  tex.setHeight(rep.height);
  tex.setImageCount(rep.number_of_images);
  tex.setLod(rep.custom_lod, rep.minLod, rep.maxLod);
  tex.setSourcePath(rep.sourcePath);
  tex.resizeData();
  memcpy(tex.getData(), rep.data.data(),
         std::min<u32>(tex.getEncodedSize(true), rep.data.size()));
}
struct ErrorState {
  std::string mTitle;
  std::string mError;
  bool mCloseRequested = false;

  ErrorState(std::string&& title) : mTitle(title) {}
  void enter(std::string&& err) {
    mError = err;
    ImGui::OpenPopup(mTitle.c_str());
  }
  void exit() { mCloseRequested = true; }
  void modal() {
    if (ImGui::BeginPopupModal(mTitle.c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("%s", mError.c_str());
      const bool keep_alive = !ImGui::Button("OK") || mCloseRequested;
      if (!keep_alive) {
        ImGui::CloseCurrentPopup();
        mError.clear();
        mCloseRequested = false;
      }
      ImGui::EndPopup();
    }
  }
};

class CrateReplaceAction
    : public kpi::ActionMenu<riistudio::g3d::Material, CrateReplaceAction> {
  bool replace = false;

  ErrorState mErrorState{"Preset Error"};

public:
  // Return true if the state of obj was mutated (add to history)
  bool _context(riistudio::g3d::Material& mat) {
    if (ImGui::MenuItem("Apply .mdl0mat material preset"_j)) {
      replace = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::Material& mat) {
    mErrorState.modal();

    if (replace) {
      replace = false;
      auto err = tryReplace(mat);
      if (err.size()) {
        err = "Cannot apply preset. " + err;
        mErrorState.enter(std::move(err));
        // We still return true, since this could've left us in a partially
        // mutated state
      }
      return true;
    }

    return false;
  }

private:
  std::string tryReplace(riistudio::g3d::Material& mat) {
    const auto path = rsl::OpenOneFile("Select preset"_j, "",
                                       {
                                           "MDL0Mat Files",
                                           "*.mdl0mat",
                                       });
    if (!path) {
      return path.error();
    }
    const auto preset = path->parent_path();
    return ApplyCratePresetToMaterial(mat, preset);
  }
};

class SaveAsTEX0 : public kpi::ActionMenu<libcube::Texture, SaveAsTEX0> {
  bool m_export = false;
  ErrorState m_errorState{"TEX0 Export Error"};

  std::string tryExport(const libcube::Texture& tex) {
    std::string path = tex.getName() + ".tex0";

    // Web version does not
    if (plate::Platform::supportsFileDialogues()) {
      auto rep = rsl::SaveOneFile("Export Path"_j, path,
                                  {
                                      "TEX0 texture (*.tex0)",
                                      "*.tex0",
                                  });
      if (!rep) {
        return rep.error();
      }
      path = rep->string();
    }

    librii::g3d::TextureData tex0{
        .name = tex.getName(),
        .format = tex.getTextureFormat(),
        .width = tex.getWidth(),
        .height = tex.getHeight(),
        .number_of_images = tex.getImageCount(),
        .custom_lod = true,
        .minLod = tex.getMinLod(),
        .maxLod = tex.getMaxLod(),
        .sourcePath = tex.getSourcePath(),
        .data = {},
    };
    tex0.data.resize(tex.getEncodedSize(true));
    memcpy(tex0.data.data(), tex.getData(),
           std::min<u32>(tex.getEncodedSize(true), tex0.data.size()));
    auto buf = librii::crate::WriteTEX0(tex0);
    if (buf.empty()) {
      return "librii::crate::WriteTEX0 failed";
    }

    plate::Platform::writeFile(buf, path);
    return {};
  }

public:
  bool _context(libcube::Texture&) {
    if (ImGui::MenuItem("Save as .tex0"_j)) {
      m_export = true;
    }

    return false;
  }

  bool _modal(libcube::Texture& tex) {
    m_errorState.modal();

    if (m_export) {
      m_export = false;
      auto err = tryExport(tex);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
    }

    return false;
  }
};
class SaveAsSRT0 : public kpi::ActionMenu<riistudio::g3d::SRT0, SaveAsSRT0> {
  bool m_export = false;
  ErrorState m_errorState{"SRT0 Export Error"};

  std::string tryExport(const librii::g3d::SrtAnimationArchive& arc) {
    std::string path = arc.name + ".srt0";

    // Web version does not
    if (rsl::FileDialogsSupported()) {
      auto choice = rsl::SaveOneFile("Export Path"_j, path,
                                     {
                                         "SRT0 file (*.srt0)",
                                         "*.srt0",
                                     });
      if (!choice) {
        return choice.error();
      }
      path = choice->string();
    }

    auto buf = librii::crate::WriteSRT0(arc);
    if (!buf) {
      return "librii::crate::WriteSRT0 failed: " + buf.error();
    }
    if (buf->empty()) {
      return "librii::crate::WriteSRT0 failed";
    }

    plate::Platform::writeFile(*buf, path);
    return {};
  }

public:
  bool _context(riistudio::g3d::SRT0&) {
    if (ImGui::MenuItem("Save as .srt0"_j)) {
      m_export = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::SRT0& srt) {
    m_errorState.modal();

    if (m_export) {
      m_export = false;
      auto err = tryExport(srt);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
    }

    return false;
  }
};

class SaveAsMDL0MatShade
    : public kpi::ActionMenu<riistudio::g3d::Material, SaveAsMDL0MatShade> {
  bool m_export = false;
  ErrorState m_errorState{"SaveAsMDL0MatShade Export Error"};

  std::string tryExport(const librii::g3d::G3dMaterialData& mat) {
    std::string path = mat.name + ".mdl0mat";

    // Web version does not
    if (rsl::FileDialogsSupported()) {
      auto choice = rsl::SaveOneFile("Export Path"_j, path,
                                     {
                                         "MDL0Mat file (*.mdl0mat)",
                                         "*.mdl0mat",
                                     });
      if (!choice) {
        return choice.error();
      }
      path = choice->string();
    }

    auto buf = librii::crate::WriteMDL0Mat(mat);
    if (buf.empty()) {
      return "librii::crate::WriteMDL0Mat failed";
    }
    plate::Platform::writeFile(buf, path);

    {
      auto fs_path = std::filesystem::path(path);
      auto shade =
          fs_path.replace_filename(fs_path.stem().string() + ".mdl0shade");
      auto buf = librii::crate::WriteMDL0Shade(mat);
      if (buf.empty()) {
        return "librii::crate::WriteMDL0Shade failed";
      }
      plate::Platform::writeFile(buf, shade.string());
    }

    return {};
  }

public:
  bool _context(riistudio::g3d::Material&) {
    if (ImGui::MenuItem("Save as .mdl0mat/.mdl0shade"_j)) {
      m_export = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::Material& srt) {
    m_errorState.modal();

    if (m_export) {
      m_export = false;
      auto err = tryExport(srt);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
    }

    return false;
  }
};

class ReplaceWithTEX0
    : public kpi::ActionMenu<libcube::Texture, ReplaceWithTEX0> {
  bool m_import = false;
  ErrorState m_errorState{"TEX0 Import Error"};

  std::string tryImport(libcube::Texture& tex) {
    const auto file = rsl::ReadOneFile("Import Path"_j, "",
                                       {
                                           "TEX0 texture",
                                           "*.tex0",
                                       });
    if (!file) {
      return file.error();
    }
    auto replacement = librii::crate::ReadTEX0(file->data);
    if (!replacement) {
      return "Failed to read .tex0 at \"" + file->path.string() + "\"\n" +
             replacement.error();
    }

    auto name = tex.getName();
    OverwriteWithG3dTex(tex, *replacement);
    tex.setName(name);
    tex.onUpdate();

    return {};
  }

public:
  bool _context(libcube::Texture&) {
    if (ImGui::MenuItem("Replace with .tex0"_j)) {
      m_import = true;
    }

    return false;
  }

  bool _modal(libcube::Texture& tex) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImport(tex);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return true;
    }

    return false;
  }
};
class ReplaceWithSRT0
    : public kpi::ActionMenu<riistudio::g3d::SRT0, ReplaceWithSRT0> {
  bool m_import = false;
  ErrorState m_errorState{"SRT0 Import Error"};

  std::string tryImport(riistudio::g3d::SRT0& srt) {
    const auto file = rsl::ReadOneFile("Import Path"_j, "",
                                       {
                                           "SRT0 Animation",
                                           "*.srt0",
                                       });
    if (!file) {
      return file.error();
    }
    auto replacement = librii::crate::ReadSRT0(file->data);
    auto name = srt.getName();
    static_cast<librii::g3d::SrtAnimationArchive&>(srt) = *replacement;
    srt.setName(name);

    return {};
  }

public:
  bool _context(riistudio::g3d::SRT0&) {
    if (ImGui::MenuItem("Replace with .srt0"_j)) {
      m_import = true;
    }

    return false;
  }

  bool _modal(riistudio::g3d::SRT0& srt) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImport(srt);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return true;
    }

    return false;
  }
};

std::string tryImportMany(libcube::Scene& scn) {
  const auto files = rsl::ReadManyFile("Import Path"_j, "",
                                       {
                                           "Image files",
                                           "*.tex0;*.png;*.tga;*.jpg;*.bmp",
                                           "TEX0 Files",
                                           "*.tex0",
                                           "PNG Files",
                                           "*.png",
                                           "TGA Files",
                                           "*.tga",
                                           "JPG Files",
                                           "*.jpg",
                                           "BMP Files",
                                           "*.bmp",
                                           "All Files",
                                           "*",
                                       });
  if (!files) {
    return files.error();
  }

  for (const auto& file : *files) {
    const auto& path = file.path.string();
    if (path.ends_with(".tex0")) {
      auto rep = librii::crate::ReadTEX0(file.data);
      if (!rep) {
        return "Failed to read .tex0 at \"" + std::string(path) + "\"\n" +
               rep.error();
      }
      OverwriteWithG3dTex(scn.getTextures().add(), *rep);

      continue;
    }

    auto image = rsl::stb::load(path);
    if (!image) {
      return std::format("Failed to import texture {}: stb_image didn't "
                         "recognize it or didn't exist.",
                         path);
    }
    std::vector<u8> scratch;
    auto& tex = scn.getTextures().add();
    const auto ok = riistudio::rhst::importTexture(
        tex, image->data.data(), scratch, true, 64, 4, image->width,
        image->height, image->channels);
    if (!ok) {
      return std::format("Failed to import texture {}: {}", path, ok.error());
    }
    tex.setName(file.path.stem().string());
  }

  return {};
}

std::string tryImportManySrt(riistudio::g3d::Collection& scn) {
  const auto files = rsl::ReadManyFile("Import Path"_j, "",
                                       {
                                           "SRT0 Animations",
                                           "*.srt0",
                                       });
  if (!files) {
    return files.error();
  }

  for (const auto& file : *files) {
    const auto& path = file.path.string();
    if (path.ends_with(".srt0")) {
      auto rep = librii::crate::ReadSRT0(file.data);
      if (!rep) {
        return "Failed to read .srt0 at \"" + std::string(path) + "\"\n" +
               rep.error();
      }
      static_cast<librii::g3d::SrtAnimationArchive&>(scn.getAnim_Srts().add()) =
          *rep;
    }
  }

  return {};
}
Result<std::vector<librii::crate::CrateAnimation>> tryImportManyRsPreset() {
  const auto files =
      TRY(rsl::ReadManyFile("Import Path"_j, "",
                            {
                                ".rspreset Material/Animation presets",
                                "*.rspreset",
                            }));

  std::vector<librii::crate::CrateAnimation> presets;

  for (const auto& file : files) {
    const auto& path = file.path.string();
    if (!path.ends_with(".rspreset")) {
      continue;
    }
    auto rep = librii::crate::ReadRSPreset(file.data);
    if (!rep) {
      return std::unexpected(std::format(
          "Failed to read .rspreset at \"{}\"\n{}", path, rep.error()));
    }
    presets.push_back(*rep);
  }

  return presets;
}

struct MergeAction {
  riistudio::g3d::Model* target_model;
  std::vector<librii::crate::CrateAnimation> source;
  // target_selection.size() == target_model->getMaterials().size()
  std::vector<std::optional<size_t>> source_selection;

  [[nodiscard]] Result<void>
  Populate(riistudio::g3d::Model* target_model_,
           std::vector<librii::crate::CrateAnimation>&& source_) {
    target_model = target_model_;
    source = std::move(source_);
    source_selection.clear();
    source_selection.resize(target_model->getMaterials().size());
    return RestoreDefaults();
  }

  [[nodiscard]] Result<void> MergeNothing() {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    std::ranges::fill(source_selection, std::nullopt);
    return {};
  }

  [[nodiscard]] Result<void> RestoreDefaults() {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    std::unordered_map<std::string, size_t> lut;
    for (auto&& [idx, preset] : rsl::enumerate(source)) {
      lut[preset.mat.name] = idx;
    }
    // TODO: rsl::ToList() is a hack
    for (auto&& [target_idx, target] :
         rsl::enumerate(target_model->getMaterials() | rsl::ToList())) {
      auto it = lut.find(target.name);
      if (it != lut.end()) {
        size_t source_idx = it->second;
        EXPECT(source_idx < source.size());
        source_selection[target_idx] = source_idx;
      } else {
        source_selection[target_idx] = std::nullopt;
      }
    }
    return {};
  }

  [[nodiscard]] Result<void>
  SetMergeSelection(size_t target_index, std::optional<size_t> source_index) {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    EXPECT(target_index < target_model->getMaterials().size());
    if (source_index.has_value()) {
      EXPECT(*source_index < source.size());
      source_selection[target_index] = *source_index;
    } else {
      source_selection[target_index] = std::nullopt;
    }
    return {};
  }

  [[nodiscard]] Result<void> DontMerge(size_t target_index) {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    EXPECT(target_index < target_model->getMaterials().size());
    source_selection[target_index] = std::nullopt;
    return {};
  }

  [[nodiscard]] Result<void> RestoreDefault(size_t target_index) {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    EXPECT(target_index < target_model->getMaterials().size());
    auto name = target_model->getMaterials()[target_index].name;
    // Since later entries in LUT will override previous ones, we need to
    // iterate backwards
    std::optional<size_t> source_index = std::nullopt;
    // TODO: rsl::ToList() is a hack
    for (auto& [idx, source] :
         std::views::reverse(rsl::enumerate(source) | rsl::ToList())) {
      if (source.mat.name == name) {
        source_index = idx;
        break;
      }
    }
    source_selection[target_index] = source_index;
    return {};
  }

  [[nodiscard]] Result<std::vector<std::string>> PerformMerge() {
    EXPECT(target_model != nullptr);
    EXPECT(source_selection.size() == target_model->getMaterials().size());
    std::vector<std::string> logs;
    for (size_t i = 0; i < target_model->getMaterials().size(); ++i) {
      auto source_index = source_selection[i];
      if (!source_index.has_value()) {
        continue;
      }
      auto& target_mat = target_model->getMaterials()[i];
      auto& source_mat = source[*source_index];
      auto err =
          riistudio::g3d::ApplyCratePresetToMaterial(target_mat, source_mat);
      if (err.size()) {
        logs.push_back(
            std::format("Attempted to merge {} into {}. Failed with error: {}",
                        source_mat.mat.name, target_mat.name, err));
      }
    }
    return logs;
  }
};

void DrawComboSourceMat(MergeAction& action, int& item) {
  if (item >= std::ssize(action.source)) {
    item = 0;
  }
  char buf[128];
  if (item < 0) {
    snprintf(buf, sizeof(buf), "(None)");
  } else {
    snprintf(buf, sizeof(buf), "Preset: %s %s",
             (const char*)ICON_FA_PAINT_BRUSH,
             action.source[item].mat.name.c_str());
  }

  if (ImGui::BeginCombo("##combo", buf)) {
    for (int i = -1; i < static_cast<int>(action.source.size()); ++i) {
      bool selected = i == item;
      if (i < 0) {
        snprintf(buf, sizeof(buf), "(None)");
      } else {
        snprintf(buf, sizeof(buf), "Preset: %s %s",
                 (const char*)ICON_FA_PAINT_BRUSH,
                 action.source[i].mat.name.c_str());
      }
      if (ImGui::Selectable(buf, &selected)) {
        item = i;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
}
void DrawComboSourceMat(MergeAction& action, std::optional<size_t>& item) {
  int sel = item.value_or(-1);
  DrawComboSourceMat(action, sel);
  if (sel < 0) {
    item = std::nullopt;
  } else if (sel <= action.source.size()) {
    item = sel;
  }
}

struct MergeUIResult {
  struct None {};
  struct OK {
    std::vector<std::string> logs;
  };
  struct Cancel {};
  enum {
    None_i,
    OK_i,
    Cancel_i,
  };
};
using MergeUIResult_t =
    std::variant<MergeUIResult::None, MergeUIResult::OK, MergeUIResult::Cancel>;

[[nodiscard]] Result<MergeUIResult_t> DrawMergeActionUI(MergeAction& action) {
  static size_t selected_row = 0;
  if (selected_row >= action.source_selection.size()) {
    selected_row = 0;
  }

  // Child 1: no border, enable horizontal scrollbar
  {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("ChildL",
                      ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 400),
                      false, window_flags);
    if (ImGui::BeginTable("split", 2,
                          ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_NoSavedSettings |
                              ImGuiTableFlags_PadOuterX)) {
      ImGui::TableSetupColumn("Target Material");
      ImGui::TableSetupColumn("Preset to apply");
      ImGui::TableHeadersRow();
      for (size_t i = 0; i < action.source_selection.size(); i++) {
        ImGui::TableNextRow();
        char buf[128]{};
        snprintf(buf, sizeof(buf), "%s %s", (const char*)ICON_FA_PAINT_BRUSH,
                 action.target_model->getMaterials()[i].name.c_str());
        ImGui::TableSetColumnIndex(0);
        bool selected = i == selected_row;
        // Grab arrow keys with |=
        selected |= ImGui::Selectable(buf, &selected,
                                      ImGuiSelectableFlags_SpanAllColumns);
        if (selected || ImGui::IsItemFocused()) {
          selected_row = i;
        }
        ImGui::TableSetColumnIndex(1);
        auto sel = action.source_selection[i];
        if (!sel.has_value() || *sel >= action.source.size()) {
          ImGui::TextUnformatted("(None)");
        } else {
          ImGui::TextUnformatted((const char*)ICON_FA_LONG_ARROW_ALT_LEFT);
          ImGui::SameLine();
          riistudio::g3d::Material tmp;
          ImGui::PushStyleColor(
              ImGuiCol_Text,
              kpi::RichNameManager::getInstance().getRich(&tmp).getIconColor());
          ImGui::TextUnformatted((const char*)ICON_FA_PAINT_BRUSH);
          ImGui::PopStyleColor();
          ImGui::SameLine();
          ImGui::TextUnformatted(
              (const char*)action.source[*sel].mat.name.c_str());
        }
      }
      ImGui::EndTable();
    }
    ImGui::EndChild();
  }

  ImGui::SameLine();

  // Child 2: rounded border
  {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    ImGui::BeginChild("ChildR", ImVec2(0, 400), true, window_flags);
    RSL_DEFER(ImGui::EndChild());

    ImGui::Text("Merge Material Selection:");
    EXPECT(selected_row < action.source_selection.size());
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    DrawComboSourceMat(action, action.source_selection[selected_row]);

    auto button = ImVec2{ImGui::GetContentRegionAvail().x / 2.0f, 0};
    if (ImGui::Button("Do not merge", button)) {
      TRY(action.DontMerge(selected_row));
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore selection", button)) {
      TRY(action.RestoreDefault(selected_row));
    }

    auto sel = action.source_selection[selected_row];
    if (sel.has_value()) {
      EXPECT(*sel < action.source.size());
      auto& p = action.source[*sel];

      // TODO: We shouldn't need a singleton
      auto* icons = riistudio::IconManager::get();
      EXPECT(icons != nullptr);
      int it = 0;
      for (auto& samp : p.mat.samplers) {
        if (it++ != 0) {
          ImGui::SameLine();
        }
        auto tex = std::ranges::find_if(
            p.tex, [&](auto& x) { return x.name == samp.mTexture; });
        EXPECT(tex != p.tex.end());
        // TODO: This burns through generation IDs
        riistudio::g3d::Texture dyn;
        auto path = p.mat.name + tex->name;
        auto hash = std::hash<std::string>()(path);
        static_cast<librii::g3d::TextureData&>(dyn) = *tex;
        dyn.mGenerationId = hash;
        icons->drawImageIcon(&dyn, 64);
      }

      if (ImGui::BeginTable("Info", 2, ImGuiTableFlags_PadOuterX)) {
        ImGui::TableSetupColumn("Field");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        auto DrawRow = [&](const char* desc, const char* val) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextUnformatted(desc);
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(val);
        };
        DrawRow("Author", librii::crate::GetAuthor(p).value_or("?").c_str());
        DrawRow("Date", librii::crate::GetDateCreated(p).value_or("?").c_str());
        DrawRow("Comment", librii::crate::GetComment(p).value_or("?").c_str());
        DrawRow("Tool", librii::crate::GetTool(p).value_or("?").c_str());
        ImGui::EndTable();
      }
      ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt,
                            ImGui::GetStyleColorVec4(ImGuiCol_TableRowBg));
      RSL_DEFER(ImGui::PopStyleColor());

      if (ImGui::BeginTable("Animations", 2,
                            ImGuiTableFlags_PadOuterX |
                                ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Included?");
        ImGui::TableHeadersRow();

        auto DrawRow = [&](const char* desc, size_t count) {
          bool included = count > 0;

          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          if (included) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                   IM_COL32(0, 255, 0, 50));
          } else {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                   IM_COL32(255, 0, 0, 50));
          }

          ImGui::TextUnformatted(desc);
          ImGui::TableSetColumnIndex(1);
          if (included) {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                   IM_COL32(0, 255, 0, 50));
          } else {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                   IM_COL32(255, 0, 0, 50));
          }
          ImGui::Text("%s", included ? "Yes" : "No");
        };
        DrawRow("Texture SRT animation", p.srt.size());
        DrawRow("Texture pattern animation", p.pat.size());
        DrawRow("Shader color animation", p.clr.size());
        ImGui::EndTable();
      }
    }
    ImGui::SetCursorPosY(ImGui::GetWindowContentRegionMax().y - 48.0f);
    auto wide = ImVec2{ImGui::GetContentRegionAvail().x, 0};
    if (ImGui::Button("Merge nothing", wide)) {
      TRY(action.MergeNothing());
    }
    if (ImGui::Button("Restore defaults", wide)) {
      TRY(action.RestoreDefaults());
    }
  }
  ImGui::Separator();

  auto button = ImVec2{75, 0};
  ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - button.x * 2);
  {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 255, 0, 100));
    RSL_DEFER(ImGui::PopStyleColor());
    if (ImGui::Button("OK", button)) {
      auto logs = TRY(action.PerformMerge());
      return MergeUIResult::OK{std::move(logs)};
    }
  }
  ImGui::SameLine();
  {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 0, 0, 100));
    RSL_DEFER(ImGui::PopStyleColor());
    if (ImGui::Button("Cancel", button)) {
      return MergeUIResult::Cancel{};
    }
  }
  return MergeUIResult::None{};
}

class ImportPresetsAction
    : public kpi::ActionMenu<riistudio::g3d::Model, ImportPresetsAction> {
  struct State {
    struct None {};
    struct ImportDialog {};
    struct MergeDialog {
      MergeAction m_action;
    };
    struct Error : public ErrorState {
      Error(std::string&& err) : ErrorState("Preset Import Error") {
        ErrorState::enter(std::move(err));
      }
    };
  };
  std::variant<State::None, State::ImportDialog, State::MergeDialog,
               State::Error>
      m_state{State::None{}};

public:
  bool _context(riistudio::g3d::Model&) {
    if (ImGui::MenuItem("Import multiple presets"_j)) {
      m_state = State::ImportDialog{};
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Model& mdl) {
    if (m_state.index() == 3) {
      State::Error& error = *std::get_if<State::Error>(&m_state);
      error.enter(std::move(error.mError));
      error.modal();
      if (error.mError.empty()) {
        m_state = State::None{};
      }
      return kpi::NO_CHANGE;
    }

    if (m_state.index() == 1) {
      auto presets = tryImportManyRsPreset();
      if (!presets) {
        m_state = State::Error{std::move(presets.error())};
        return kpi::NO_CHANGE;
      }
      MergeAction action;
      auto ok = action.Populate(&mdl, std::move(*presets));
      if (!ok) {
        m_state = State::Error{std::move(presets.error())};
        return kpi::NO_CHANGE;
      }
      m_state = State::MergeDialog{std::move(action)};
    }
    if (m_state.index() == 2) {
      auto& merge = *std::get_if<State::MergeDialog>(&m_state);
      ImGui::OpenPopup("Merge");
      ImGui::SetNextWindowSize(ImVec2{800.0f, 446.0f});
      if (ImGui::BeginPopup("Merge")) {
        RSL_DEFER(ImGui::EndPopup());
        auto ok = DrawMergeActionUI(merge.m_action);
        if (!ok) {
          m_state = State::Error{std::move(ok.error())};
          return kpi::NO_CHANGE;
        }
        switch (ok->index()) {
        case MergeUIResult::OK_i: {
          auto logs = std::move(std::get_if<MergeUIResult::OK>(&*ok)->logs);
          if (logs.size()) {
            auto err = std::format("The following errors were encountered:\n{}",
                                   rsl::join(logs, "\n"));
            m_state = State::Error{std::move(err)};
            return kpi::CHANGE;
          } else {
            m_state = State::None{};
            return kpi::CHANGE_NEED_RESET;
          }
        }
        case MergeUIResult::Cancel_i:
          m_state = State::None{};
          return kpi::NO_CHANGE;
        case MergeUIResult::None_i:
          return kpi::NO_CHANGE;
        }
      }
    }

    return kpi::NO_CHANGE;
  }
};

class ImportTexturesAction
    : public kpi::ActionMenu<libcube::Scene, ImportTexturesAction> {
  bool m_import = false;
  ErrorState m_errorState{"Textures Import Error"};

public:
  bool _context(libcube::Scene&) {
    if (ImGui::MenuItem("Import textures"_j)) {
      m_import = true;
    }

    return false;
  }

  kpi::ChangeType _modal(libcube::Scene& scn) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImportMany(scn);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

class ImportSRTAction
    : public kpi::ActionMenu<riistudio::g3d::Collection, ImportSRTAction> {
  bool m_import = false;
  ErrorState m_errorState{"SRT Import Error"};

public:
  bool _context(riistudio::g3d::Collection&) {
    if (ImGui::MenuItem("Import Texture SRT Animations"_j)) {
      m_import = true;
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Collection& scn) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImportManySrt(scn);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

struct TextEdit {
  std::vector<char> m_buffer;

  void Reset(const std::string& name) {
    m_buffer.resize(name.size() + 1024);
    std::fill(m_buffer.begin(), m_buffer.end(), '\0');
    m_buffer.insert(m_buffer.begin(), name.begin(), name.end());
  }

  bool Modal(const std::string& name) {
    auto new_name = std::string(m_buffer.data());
    bool ok = ImGui::InputText(
        name != new_name ? "Name*###Name" : "Name###Name", m_buffer.data(),
        m_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);
    return ok;
  }

  std::string String() const { return std::string(m_buffer.data()); }
};

class RenameNode : public kpi::ActionMenu<kpi::IObject, RenameNode> {
  bool m_rename = false;
  TextEdit m_textEdit;

public:
  bool _context(kpi::IObject& obj) {
    // Really silly way to determine if an object can be renamed
    auto name = obj.getName();
    obj.setName(obj.getName() + "_");
    bool can_rename = obj.getName() != name;
    obj.setName(name);

    if (can_rename && ImGui::MenuItem("Rename"_j)) {
      m_rename = true;
      m_textEdit.Reset(name);
    }

    return false;
  }

  kpi::ChangeType _modal(kpi::IObject& obj) {
    if (m_rename) {
      m_rename = false;
      ImGui::OpenPopup("Rename");
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter());
    const auto wflags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_AlwaysAutoResize;
    bool open = true;
    if (ImGui::BeginPopupModal("Rename", &open, wflags)) {
      RSL_DEFER(ImGui::EndPopup());
      const auto name = obj.getName();
      bool ok = m_textEdit.Modal(name);
      if (ImGui::Button("Rename") || ok) {
        const auto new_name = m_textEdit.String();
        if (name == new_name) {
          ImGui::CloseCurrentPopup();
          return kpi::NO_CHANGE;
        }
        obj.setName(new_name);
        ImGui::CloseCurrentPopup();
        return kpi::CHANGE;
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") || !open) {
        ImGui::CloseCurrentPopup();
        return kpi::NO_CHANGE;
      }
    }

    return kpi::NO_CHANGE;
  }
};
std::string tryImportRsPreset(riistudio::g3d::Material& mat) {
  const auto file = rsl::ReadOneFile("Select preset"_j, "",
                                     {
                                         "rspreset Files",
                                         "*.rspreset",
                                     });
  if (!file) {
    return file.error();
  }
  return ApplyRSPresetToMaterial(mat, file->data);
}
class ApplyRsPreset
    : public kpi::ActionMenu<riistudio::g3d::Material, ApplyRsPreset> {
  bool m_import = false;
  ErrorState m_errorState{"RSPreset Import Error"};

public:
  bool _context(riistudio::g3d::Material&) {
    if (ImGui::MenuItem("Apply .rspreset material preset"_j)) {
      m_import = true;
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Material& mat) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryImportRsPreset(mat);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

std::string tryExportRsPresetMat(std::string path,
                                 const riistudio::g3d::Material& mat) {
  auto anim = CreatePresetFromMaterial(mat);
  if (!anim) {
    return "Failed to create preset bundle: " + anim.error();
  }

  auto buf = librii::crate::WriteRSPreset(*anim);
  if (buf.empty()) {
    return "librii::crate::WriteRSPreset failed";
  }

  plate::Platform::writeFile(buf, path);
  return {};
}

std::string tryExportRsPreset(const riistudio::g3d::Material& mat) {
  std::string path = mat.name + ".rspreset";
  // Web version doesn't support this
  if (rsl::FileDialogsSupported()) {
    const auto file = rsl::SaveOneFile("Select preset"_j, path,
                                       {
                                           "rspreset Files",
                                           "*.rspreset",
                                       });
    if (!file) {
      return file.error();
    }
    path = file->string();
  }
  return tryExportRsPresetMat(path, mat);
}

std::string tryExportRsPresetALL(auto&& mats) {
  if (!rsl::FileDialogsSupported()) {
    return "File dialogues unsupported on this platform";
  }
  const auto folder = pfd::select_folder("Output folder"_j, "").result();
  if (folder.empty()) {
    return folder;
  }
  auto path = std::filesystem::path(folder);
  if (!std::filesystem::exists(path)) {
    return "Folder doesn't exist";
  }
  if (!std::filesystem::is_directory(path)) {
    return "Not a folder";
  }
  for (auto& mat : mats) {
    auto ok =
        tryExportRsPresetMat((path / (mat.name + ".rspreset")).string(), mat);
    if (ok.size()) {
      return ok;
    }
  }
  return {};
}

class MakeRsPreset
    : public kpi::ActionMenu<riistudio::g3d::Material, MakeRsPreset> {
  bool m_import = false;
  ErrorState m_errorState{"RSPreset Export Error"};

public:
  bool _context(riistudio::g3d::Material&) {
    if (ImGui::MenuItem("Create material preset"_j)) {
      m_import = true;
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Material& mat) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryExportRsPreset(mat);
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

class MakeRsPresetALL
    : public kpi::ActionMenu<riistudio::g3d::Model, MakeRsPresetALL> {
  bool m_import = false;
  ErrorState m_errorState{"RSPreset Export Error"};

public:
  bool _context(riistudio::g3d::Model&) {
    if (ImGui::MenuItem("Export all materials as presets")) {
      m_import = true;
    }
    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Model& model) {
    m_errorState.modal();

    if (m_import) {
      m_import = false;
      auto err = tryExportRsPresetALL(model.getMaterials());
      if (!err.empty()) {
        m_errorState.enter(std::move(err));
      }
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

struct AddChild : public kpi::ActionMenu<riistudio::g3d::Bone, AddChild> {
  bool m_do = false;
  bool _context(riistudio::g3d::Bone&) {
    if (ImGui::MenuItem("Add child"_j)) {
      m_do = true;
    }

    return false;
  }

  kpi::ChangeType _modal(riistudio::g3d::Bone& bone) {
    if (m_do) {
      m_do = false;
      auto* col = bone.collectionOf;
      col->add();
      auto childId = col->size() - 1;
      auto* child = dynamic_cast<riistudio::g3d::Bone*>(col->atObject(childId));
      assert(child);
      child->id = childId;
      child->mParent = bone.id;
      auto* mdl = dynamic_cast<riistudio::g3d::Model*>(bone.childOf);
      assert(mdl);
      bone.mChildren.push_back(childId);
      return kpi::CHANGE_NEED_RESET;
    }

    return kpi::NO_CHANGE;
  }
};

kpi::DecentralizedInstaller
    CrateReplaceInstaller([](kpi::ApplicationPlugins& installer) {
      auto& action_menu = kpi::ActionMenuManager::get();
      action_menu.addMenu(std::make_unique<AddChild>());
      action_menu.addMenu(std::make_unique<RenameNode>());
      action_menu.addMenu(std::make_unique<SaveAsTEX0>());
      action_menu.addMenu(std::make_unique<SaveAsSRT0>());
      action_menu.addMenu(std::make_unique<MakeRsPreset>());
      action_menu.addMenu(std::make_unique<MakeRsPresetALL>());
      action_menu.addMenu(std::make_unique<ImportPresetsAction>());
      action_menu.addMenu(std::make_unique<SaveAsMDL0MatShade>());
      if (rsl::FileDialogsSupported()) {
        action_menu.addMenu(std::make_unique<ApplyRsPreset>());
        action_menu.addMenu(std::make_unique<CrateReplaceAction>());
        action_menu.addMenu(std::make_unique<ReplaceWithTEX0>());
        action_menu.addMenu(std::make_unique<ReplaceWithSRT0>());
        action_menu.addMenu(std::make_unique<ImportTexturesAction>());
        action_menu.addMenu(std::make_unique<ImportSRTAction>());
      }
    });

} // namespace libcube::UI
