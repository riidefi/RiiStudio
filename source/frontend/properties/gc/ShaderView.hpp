#pragma once

#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/gc/Export/Material.hpp>

#include <vendor/ImGuiColorTextEdit/TextEditor.h>

namespace libcube::UI {

struct ShaderSurface final {
  static inline const char* name() { return "Pixel Shader"_j; };
  static inline const char* icon = (const char*)ICON_FA_CODE;

  // Mark this surface to be more than an IDL tag.
  int tag_stateful = 1;

  TextEditor editor;
  libcube::IGCMaterial* matKey = nullptr;

  ShaderSurface() {
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
  }
};

void drawProperty(kpi::PropertyDelegate<libcube::IGCMaterial>& delegate,
                  ShaderSurface& surface);

} // namespace libcube::UI
