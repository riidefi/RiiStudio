#include "frontend/legacy_editor/EditorWindow.hpp"
#include "frontend/legacy_editor/views/BmdBrresOutliner.hpp"
#include "frontend/widgets/OutlinerWidget.hpp"
#include "LibBadUIFramework/ActionMenu.hpp"
#include "librii/jparticle/JParticle.hpp"
#include "librii/jparticle/JEFFJPA1.hpp"

#include <frontend/IEditor.hpp>
#include <frontend/legacy_editor/StudioWindow.hpp>
#include <frontend/widgets/AutoHistory.hpp>
#include <frontend/widgets/PropertyEditorWidget.hpp>
#include <rsl/FsDialog.hpp>

namespace riistudio::frontend {
class JpaEditor;


// Implements a single tab for now
class JpaEditorTabSheet : private PropertyEditorWidget {
public:
  void Draw(std::function<void(void)> draw) {
    // No concurrent access
    assert(m_drawTab == nullptr);
    m_drawTab = draw;
    DrawTabWidget(false);
    Tabs();
    m_drawTab = nullptr;
  }

private:
  std::vector<std::string> TabTitles() override { return {"JPAC"}; }

  bool Tab(int index) override {
    m_drawTab();
    return true;
  }

  std::function<void(void)> m_drawTab = nullptr;
};


class JpaEditorPropertyGrid {
public:
  void Draw(librii::jpa::JPADynamicsBlock* block);
  void Draw(librii::jpa::JPABaseShapeBlock* block);
  void Draw(librii::jpa::JPAExtraShapeBlock* block);
  void Draw(librii::jpa::JPAFieldBlock* block);
};

using JPABlockSelection = std::variant<librii::jpa::JPADynamicsBlock*,
                                       librii::jpa::JPABaseShapeBlock*,
                                       librii::jpa::JPAExtraShapeBlock*,
                                       librii::jpa::JPAFieldBlock*,
                                       std::string,
                                       std::monostate>;

class JpaEditorTreeView {
public:
  void Draw(std::vector<librii::jpa::JPAResource>& resources,
            std::vector<std::vector<u8>> &tex) {

    auto str = std::format("JPA resources ({} entries)", num_entries);
    if (ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      // ImGui::TreePush("JPA_resources");
      u32 entryNum = 0;
      for (auto& x : resources) {
        auto str = std::format("JPA resource {}", entryNum);
        if (ImGui::TreeNodeEx(str.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
          if (ImGui::BeginPopupContextItem(str.c_str())) {

            if (!x.esp1) {
              if(ImGui::MenuItem("Add Extra Shape")) {
                auto extraShape = new librii::jpa::JPAExtraShapeBlock();
                x.esp1 = extraShape;
              }
            }
            if (!x.etx1) {
              ImGui::MenuItem("Add Extra Texture");
            }
            if (!x.ssp1) {
              ImGui::MenuItem("Add Child Shape");
            }

            ImGui::EndPopup();
          }

          if (ImGui::Selectable("Emitter")) {
            selected = x.bem1;
          }
          if (ImGui::Selectable("Base Shape")) {
            selected = x.bsp1;
          }
          if (x.esp1) {
            if (ImGui::Selectable("Extra Shape")) {
              selected = x.esp1;
            }
            if (ImGui::BeginPopupContextItem(nullptr)) {
              if (ImGui::MenuItem("Delete")) {
                delete x.esp1;
                x.esp1 = nullptr;
                selected = {};
              }
              ImGui::EndPopup();
            }
          }

            if (ImGui::TreeNodeEx("Fields", ImGuiTreeNodeFlags_DefaultOpen)) {
              if (ImGui::BeginPopupContextItem(str.c_str())) {
                if(ImGui::MenuItem("Add Field")) {
                  auto field = new librii::jpa::JPAFieldBlock();
                  x.fld1.push_back(field);
                }

                ImGui::EndPopup();
              }
                for (int i = 0; i < x.fld1.size(); i++) {

		  auto field = x.fld1[i];
                  auto fieldName = std::format(
                    "{} Field",
                    convertIntToFieldTypeName(static_cast<u8>(field->type)));
                auto ID = std::format("{} Field", i );

                ImGui::PushID(ID.c_str());
                if (ImGui::Selectable(fieldName.c_str())) {
                  selected = field;
                }
                ImGui::PopID();
                if (ImGui::BeginPopupContextItem(nullptr)) {
                  if(ImGui::MenuItem("Delete")) {
                    x.fld1.erase(x.fld1.begin() + i);
                  }

                  ImGui::EndPopup();
                }
              }
          }
          ImGui::TreePop();
        }
        entryNum++;
      }
      ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
      for (auto& x : tex) {
        auto* c = reinterpret_cast<const char*>(x.data()+0x4);
        if (ImGui::Selectable(c)) {
        }
        if (ImGui::BeginPopupContextItem(c)) {
          if (ImGui::MenuItem("Export")) {
              exportBTI(c, x);
          }
          if (ImGui::MenuItem("Import")) {

          }
          if (ImGui::MenuItem("Delete")) {
          }
          ImGui::EndPopup();
        }
      }
    }

  }
  void exportBTI(const char* btiName, std::vector<u8> data) {
    auto default_filename = std::filesystem::path("").filename();
    default_filename.replace_filename(btiName);
    std::vector<std::string> filters{"??? (*.bti)", "*.bti"};
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::ErrorDialog("No saving - No file selected");
      return;
    }
    auto path = results->string();

    if (!path.ends_with(".bti")) {
      path += ".bti";
    }

    rsl::trace("Attempting to save to {}", path);
    oishii::Writer writer(std::endian::big);
    for (auto& b:data) {
      writer.write(b);
    }
    writer.saveToDisk(path);
  }

  u32 num_entries = 0;
  JPABlockSelection selected = {};
  u32 block_selected = 0;

private:
  std::string convertIntToFieldTypeName(u8 id) {
    switch (id) {
    case 0:
      return "Gravity";
    case 1:
      return "Air";
    case 2:
      return "Magnet";
    case 3:
      return "Newton";
    case 4:
      return "Vortex";
    case 5:
      return "Random";
    case 6:
      return "Drag";
    case 7:
      return "Convection";
    case 8:
      return "Spin";
    default:
      return "Unknown";
    }
  }
};

class JpaEditor : public frontend::StudioWindow, public IEditor {
public:
  JpaEditor()
    : StudioWindow("JPA Editor: <unknown>", DockSetting::Dockspace) {
    // setWindowFlag(ImGuiWindowFlags_MenuBar);
  }

  JpaEditor(const JpaEditor&) = delete;
  JpaEditor(JpaEditor&&) = delete;

  void draw_() override {
    setName("JPA Editor: " + m_path);
    if (ImGui::Begin((idIfyChild("Outliner")).c_str())) {
      m_tree.num_entries = m_jpa.resources.size();
      m_tree.selected = m_selected;
      // m_tree.tex.clear();
      // m_tree.res.clear();
      // for (auto& x : m_jpa.resources) {
      //   m_tree.res.push_back(x);
      // }
      // for (auto& x : m_jpa.textures) {
      //   m_tree.tex.push_back(x);
      // }
      m_tree.Draw(m_jpa.resources, m_jpa.textures);
      m_selected = m_tree.selected;
    }
    ImGui::End();

    if (ImGui::Begin((idIfyChild("Properties")).c_str())) {
      if (auto* x = std::get_if<librii::jpa::JPADynamicsBlock*>(&m_selected)) {
        m_sheet.Draw([&]() {
          if (*x) {
            m_grid.Draw(*x);
          }
        });
      } else if (auto* x = std::get_if<librii::jpa::JPABaseShapeBlock*>(
          &m_selected)) {
        m_sheet.Draw([&]() {
          if (*x) {
            m_grid.Draw(*x);
          }
        });
      } else if (auto* x =
          std::get_if<librii::jpa::JPAExtraShapeBlock*>(&m_selected)) {
        m_sheet.Draw([&]() {
          if (*x) {
            m_grid.Draw(*x);
          }
        });
      } else if (auto* x =
          std::get_if<librii::jpa::JPAFieldBlock*>(&m_selected)) {
        m_sheet.Draw([&]() {
          if (*x) {
            m_grid.Draw(*x);
          }
        });
      }
    }
    ImGui::End();

    // m_history.update(m_jpa);
  }

  ImGuiID buildDock(ImGuiID root_id) override {
    ImGuiID next = root_id;
    ImGuiID dock_right_id =
        ImGui::DockBuilderSplitNode(next, ImGuiDir_Left, 0.4f, nullptr, &next);
    ImGuiID dock_left_id =
        ImGui::DockBuilderSplitNode(next, ImGuiDir_Right, 1.0f, nullptr, &next);

    ImGui::DockBuilderDockWindow(idIfyChild("Outliner").c_str(), dock_right_id);
    ImGui::DockBuilderDockWindow(idIfyChild("Properties").c_str(),
                                 dock_left_id);
    return next;
  }

  std::string discordStatus() const override {
    return "Editing a ??? (.jpa) file.";
  }

  void openFile(std::span<const u8> buf, std::string_view path) {
    auto btk = librii::jpa::JPAC::fromMemory(buf, path);
    if (!btk) {
      rsl::ErrorDialogFmt("Failed to parse JPA\n{}", btk.error());
      return;
    }
    m_jpa = *btk;
    m_path = path;
  }

  void saveAs(std::string_view path) {
    rsl::trace("Attempting to save to {}", path);
    oishii::Writer writer(std::endian::big);
    SaveAsJEFFJP(writer, m_jpa);
    writer.saveToDisk(path);
  }

  void saveButton() override {
    rsl::trace("Attempting to save to {}", m_path);
    if (m_path.empty()) {
      saveAsButton();
      return;
    }
    saveAs(m_path);
  }

  void saveAsButton() override {
    auto default_filename = std::filesystem::path(m_path).filename();
    std::vector<std::string> filters{"??? (*.jpa)", "*.jpa"};
    auto results =
        rsl::SaveOneFile("Save File"_j, default_filename.string(), filters);
    if (!results) {
      rsl::ErrorDialog("No saving - No file selected");
      return;
    }
    auto path = results->string();

    if (!path.ends_with(".jpa")) {
      path += ".jpa";
    }

    saveAs(path);
  }

  friend class JpaEditorTreeView;

private:
  JpaEditorPropertyGrid m_grid;
  JpaEditorTabSheet m_sheet;
  JpaEditorTreeView m_tree;
  std::string m_path;
  librii::jpa::JPAC m_jpa;
  // lvl::AutoHistory<librii::j3d::BTK> m_history;
  JPABlockSelection m_selected = {};


};

} // namespace riistudio::frontend
