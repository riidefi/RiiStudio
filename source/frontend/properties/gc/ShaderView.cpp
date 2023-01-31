#include <core/3d/i3dmodel.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>

#include <vendor/ImGuiColorTextEdit/TextEditor.h>

#include <sstream>

namespace libcube::UI {

struct ShaderSurface final {
  static inline const char* name() { return "Pixel Shader"_j; };
  static inline const char* icon = (const char*)ICON_FA_CODE;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful = 1;

  TextEditor editor;
  riistudio::lib3d::Material* matKey = nullptr;

  ShaderSurface() {
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
  }
};

void drawProperty(kpi::PropertyDelegate<riistudio::lib3d::Material>& delegate,
                  ShaderSurface& surface) {
  auto& mat = delegate.getActive();

  // Reset val (after applie to observers)
  mat.applyCacheAgain = false;

  if (ImGui::Button("Apply"_j)) {
    mat.applyCacheAgain = true;
    mat.onUpdate();
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset"_j)) {
    mat.onUpdate();
    surface.matKey = nullptr;
  }

  if (surface.editor.GetText().empty() || surface.matKey != &mat) {
    surface.matKey = &mat;
    surface.editor.SetText(mat.cachedPixelShader);
  }

  TextEditor::ErrorMarkers tmp;

  if (mat.isShaderError) {
    std::istringstream stream(mat.shaderError);
    std::string line;
    while (std::getline(stream, line)) {
      if (line.rfind("ERROR: ", 0))
        continue;

      int nline, ncol;
      auto buf = std::make_unique<char[]>(line.size() + 1);
      std::sscanf(line.c_str(), "ERROR: %i:%i: %[^\n]", &ncol, &nline,
                  buf.get());
#ifdef _WIN32
      --nline; // For some reason line numbers are off? Probably the version
               // pragma?
#endif
      tmp.emplace(nline, buf.get());
    }
  }
  surface.editor.SetErrorMarkers(tmp);

  ImGui::SetWindowFontScale(1.2f);
  surface.editor.Render("Pixel Shader"_j, ImGui::GetContentRegionAvail());
  ImGui::SetWindowFontScale(1.0f);

  if (auto text = surface.editor.GetText();
      text != mat.cachedPixelShader.c_str())
    mat.cachedPixelShader = text;
}

#ifdef BUILD_DEBUG
kpi::RegisterPropertyView<riistudio::lib3d::Material, ShaderSurface>
    ShaderSurfaceInstaller;
#endif

} // namespace libcube::UI
