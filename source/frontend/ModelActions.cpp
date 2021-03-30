#include <core/kpi/ActionMenu.hpp>
#include <core/util/gui.hpp>
#include <plugins/gc/Export/Scene.hpp>

#include <filesystem>

// TODO: Interdependency
#include <frontend/editor/EditorWindow.hpp>
#include <frontend/editor/ImporterWindow.hpp>
#include <frontend/root.hpp>

// TODO
#include <plugins/g3d/collection.hpp>

namespace riistudio::ass {

class ModelActions final : public kpi::ActionMenu<g3d::Model, ModelActions> {
public:
  frontend::EditorWindow* mEd = nullptr;
  void set_ed(void* ed) override {
    mEd = reinterpret_cast<frontend::EditorWindow*>(ed);
  }
  bool _context(g3d::Model& model) {
    if (ImGui::MenuItem("Replace"))
      is_import = true;

    return false;
  }
  bool _modal(g3d::Model& model) {
    if (is_import) {
      ImGui::OpenPopup("Replace");
      is_import = false;
      frontend::RootWindow::spInstance->requestFile();
    }

    if (ImGui::BeginPopupModal("Replace", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      if (mImporter) {
        assert(mEd);
        auto& dispatcher = dynamic_cast<lib3d::IDrawable*>(model.childOf)->getDispatcher();
        dispatcher.beginEdit();
        if (mImporter->abort()) {
          mImporter.reset();
          dispatcher.endEdit();
          ImGui::CloseCurrentPopup();

          ImGui::EndPopup();
          return false;
        }
        if (mImporter->attachEditor()) {
          assert(mEd);

		  dispatcher.endEdit();

          ImGui::CloseCurrentPopup();

          ImGui::EndPopup();
          return true;
        }
        mImporter->draw();

        ImGui::EndPopup();
        return false;
      }
      ImGui::Text("Please drop a DAE/FBX file here.");
      if (auto* file = frontend::RootWindow::spInstance->getFile();
          file != nullptr) {
        const auto path =
            std::filesystem::path(file->mPath).filename().string();
        ImGui::Text("Replace with %s?", path.c_str());
        if (ImGui::Button("Replace")) {
          mImporter = std::make_unique<frontend::ImporterWindow>(
              std::move(*file), model.childOf);
          frontend::RootWindow::spInstance->endRequestFile();
        }
        ImGui::SameLine();
      }
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
        frontend::RootWindow::spInstance->endRequestFile();
      }

      ImGui::EndPopup();
    }

    return false;
  }

private:
  bool is_import = false;

  std::unique_ptr<frontend::ImporterWindow> mImporter = nullptr;
};

kpi::DecentralizedInstaller
    ModelActionsInstaller([](kpi::ApplicationPlugins& installer) {
      kpi::ActionMenuManager::get().addMenu(std::make_unique<ModelActions>());
    });

} // namespace riistudio::ass
