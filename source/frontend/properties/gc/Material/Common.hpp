#pragma once

#include <core/kpi/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/gc/Export/Material.hpp> // for GCMaterialData
// TODO: Awful interdependency. Fix this when icon system is stable
#include <frontend/editor/EditorWindow.hpp>

#define _AUTO_PROPERTY(delegate, before, after, val)                           \
  delegate.property(                                                           \
      before, after, [&](const auto& x) { return x.val; },                     \
      [&](auto& x, auto& y) {                                                  \
        x.val = y;                                                             \
        x.onUpdate();                                                          \
      })

#define AUTO_PROP(before, after)                                               \
  _AUTO_PROPERTY(delegate, delegate.getActive().getMaterialData().before,      \
                 after, getMaterialData().before)

namespace libcube::UI {

inline auto get_material_data = [](auto& x) -> GCMaterialData& {
  return (GCMaterialData&)x.getMaterialData();
};

MAYBE_UNUSED inline auto mat_prop = [](auto& delegate, auto member,
                                       const auto& after) {
  delegate.propertyEx(member, after, get_material_data);
};

inline auto mat_prop_ex = [](auto& delegate, auto member, auto draw) {
  delegate.propertyEx(member,
                      draw(delegate.getActive().getMaterialData().*member),
                      get_material_data);
};

inline bool IconSelectable(const char* text, bool selected,
                           const riistudio::lib3d::Texture* tex,
                           riistudio::frontend::EditorWindow* ed) {
  ImGui::PushID(tex->getName().c_str());
  auto s =
      ImGui::Selectable("##ID", selected, ImGuiSelectableFlags_None, {0, 32});
  ImGui::PopID();
  if (ed != nullptr) {
    ImGui::SameLine();
    ed->drawImageIcon(tex, 32);
  }
  ImGui::SameLine();
  ImGui::TextUnformatted(text);
  return s;
}
inline std::string TextureImageCombo(const char* current,
                                     kpi::ConstCollectionRange<Texture> images,
                                     riistudio::frontend::EditorWindow* ed) {
  std::string result;
  if (ImGui::BeginCombo("Name##Img", current)) {
    for (const auto& tex : images) {
      bool selected = tex.getName() == current;
      if (IconSelectable(tex.getName().c_str(), selected, &tex, ed)) {
        result = tex.getName();
      }
      if (selected)
        ImGui::SetItemDefaultFocus();
    }

    ImGui::EndCombo();
  }
  return result;
}
inline int
SamplerCombo(int current,
             rsl::array_vector<GCMaterialData::SamplerData, 8>& samplers,
             kpi::ConstCollectionRange<riistudio::lib3d::Texture> images,
             riistudio::frontend::EditorWindow* ed, bool allow_none = false) {
  int result = current;

  const auto format = [&](int id) -> std::string {
    if (id >= samplers.size())
      return "No selection"_j;
    return std::string("[") + std::to_string(id) + "] " + samplers[id].mTexture;
  };

  auto sid = std::string("Sampler ID"_j) + "##Img";
  if (ImGui::BeginCombo(sid.c_str(), samplers.empty()
                                         ? "No Samplers"_j
                                         : format(current).c_str())) {
    if (allow_none) {
      bool selected = current = 0xff;
      if (ImGui::Selectable("None"_j, selected)) {
        result = 0xff;
      }
      if (selected)
        ImGui::SetItemDefaultFocus();
    }

    for (int i = 0; i < samplers.size(); ++i) {
      bool selected = i == current;

      const riistudio::lib3d::Texture* curImg = nullptr;

      for (auto& it : images) {
        if (it.getName() == samplers[i].mTexture) {
          curImg = &it;
        }
      }
      if (curImg != nullptr) {
        if (IconSelectable(format(i).c_str(), selected, curImg, ed)) {
          result = i;
        }
      }
      if (selected)
        ImGui::SetItemDefaultFocus();
    }

    ImGui::EndCombo();
  }
  return result;
}

} // namespace libcube::UI
