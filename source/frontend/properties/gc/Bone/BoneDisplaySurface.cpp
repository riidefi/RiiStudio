#include "BoneDisplaySurface.hpp"

namespace libcube::UI {

void SortDisplayCommands(librii::g3d::BoneData* gb) {
  std::stable_sort(gb->mDisplayCommands.begin(), gb->mDisplayCommands.end(),
                   [](auto& l, auto& r) { return l.mPrio < r.mPrio; });
}
void RemoveDupDrawCalls(librii::g3d::BoneData* gb) {
  std::stable_sort(gb->mDisplayCommands.begin(), gb->mDisplayCommands.end());
  gb->mDisplayCommands.erase(
      std::unique(gb->mDisplayCommands.begin(), gb->mDisplayCommands.end()),
      gb->mDisplayCommands.end());
}
void MakeMatsUnique(librii::g3d::BoneData* gb, riistudio::lib3d::Model* pMdl) {
  auto materials = pMdl->getMaterials();
  std::map<u32, std::vector<u32>> mat_to_poly; // mat -> poly, poly, ...

  for (auto& cmd : gb->mDisplayCommands) {
    mat_to_poly[cmd.mMaterial].push_back(cmd.mPoly);
  }

  std::map<std::pair<u32, u32>, u32> remap; // [mat, poly] -> mat
  for (auto& [mat, polys] : mat_to_poly) {
    if (polys.size() <= 1)
      continue;
    // Clone the material n times, assign to each
    for (int i = 1; i < polys.size(); ++i) {
      remap[{mat, polys[i]}] = materials.size();

      auto* dst = dynamic_cast<riistudio::g3d::Material*>(&materials.add());
      assert(dst);
      auto* src = dynamic_cast<riistudio::g3d::Material*>(&materials[mat]);
      assert(src);

      riistudio::g3d::Material& created = *dst;
      *dst = *src;
      created.setName(created.getName() + "#" + std::to_string(i));
    }
  }
  for (auto& cmd : gb->mDisplayCommands) {
    auto key = std::pair<u32, u32>{cmd.mMaterial, cmd.mPoly};

    auto found = remap.find(key);
    if (found != remap.end()) {
      cmd.mMaterial = found->second;
    }
  }
}

void NameMatsByPoly(librii::g3d::BoneData* gb, riistudio::lib3d::Model* pMdl) {
  auto materials = pMdl->getMaterials();
  auto meshes = pMdl->getMeshes();

  int i = 0;
  for (auto& cmd : gb->mDisplayCommands) {
    materials[cmd.mMaterial].setName(meshes[cmd.mPoly].getName() + "(Mat #" +
                                     std::to_string(i) + ")");
    ++i;
  }
}

void DrawDisplays(IBoneDelegate& bone,
                  kpi::PropertyDelegate<IBoneDelegate>& delegate) {
  ImGui::Text(
      (const char*)ICON_FA_EXCLAMATION_TRIANGLE u8" Display Properties do not "
                                                u8"currently support "
                                                u8"multi-selection.");

  if (ImGui::Button("Sort"_j)) {
    auto* gb = dynamic_cast<librii::g3d::BoneData*>(&bone);
    if (gb) {
      SortDisplayCommands(gb);
    }
    delegate.commit("Sorted");
  }

  if (ImGui::Button("Remove duplicate drawcalls"_j)) {
    auto* gb = dynamic_cast<librii::g3d::BoneData*>(&bone);
    if (gb) {
      RemoveDupDrawCalls(gb);
    }
    delegate.commit("Deduplicated");
  }

  if (ImGui::Button("Make materials unique"_j)) {
    auto* gb = dynamic_cast<librii::g3d::BoneData*>(&bone);
    if (gb) {
      riistudio::lib3d::Model* pMdl = dynamic_cast<riistudio::lib3d::Model*>(
          dynamic_cast<kpi::IObject*>(&bone)->childOf);
      MakeMatsUnique(gb, pMdl);
    }
    delegate.commit("Material Unique Pass");
  }
  if (ImGui::Button("Name Materials By Polygons"_j)) {
    auto* gb = dynamic_cast<librii::g3d::BoneData*>(&bone);
    if (gb) {
      riistudio::lib3d::Model* pMdl = dynamic_cast<riistudio::lib3d::Model*>(
          dynamic_cast<kpi::IObject*>(&bone)->childOf);
      NameMatsByPoly(gb, pMdl);
    }
    delegate.commit("Name Materials By Polygons");
  }

  auto folder_id_combo = [](const char* title, const auto& folder,
                            int& active) {
    ImGui::PushItemWidth(200);
    if (ImGui::BeginCombo(title, folder[active].getName().c_str())) {
      int j = 0;
      for (const auto& node : folder) {
        if (ImGui::Selectable(node.getName().c_str(), active == j)) {
          active = j;
        }

        if (active == j)
          ImGui::SetItemDefaultFocus();

        ++j;
      }

      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
  };

  const auto entry_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable |
                           ImGuiTableFlags_Resizable |
                           ImGuiTableFlags_Reorderable;
  if (ImGui::BeginTable("Entries"_j, 4, entry_flags)) {
    const riistudio::lib3d::Model* pMdl =
        dynamic_cast<const riistudio::lib3d::Model*>(
            dynamic_cast<const kpi::IObject*>(&bone)->childOf);
    const auto materials = pMdl->getMaterials();
    const auto polys = pMdl->getMeshes();

    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("ID"_j);
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted("Material"_j);
    ImGui::TableSetColumnIndex(2);
    ImGui::TextUnformatted("Polygon"_j);
    ImGui::TableSetColumnIndex(3);
    ImGui::TextUnformatted("Sorting Priority"_j);

    for (int i = 0; i < bone.getNumDisplays(); ++i) {
      ImGui::TableNextRow();

      ImGui::PushID(i);
      auto display = bone.getDisplay(i);

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%i", i);

      ImGui::TableSetColumnIndex(1);
      int matId = display.matId;
      folder_id_combo("Material"_j, materials, matId);
      display.matId = matId;

      ImGui::TableSetColumnIndex(2);
      int polyId = display.polyId;
      folder_id_combo("Polygon"_j, polys, polyId);
      display.polyId = polyId;

      ImGui::TableSetColumnIndex(3);
      int prio = display.prio;
      ImGui::InputInt("Sorting Priority"_j, &prio);
      display.prio = prio;

      if (!(bone.getDisplay(i) == display)) {
        bone.setDisplay(i, display);
        delegate.commit("Edit Bone Display Entry");
      }

      ImGui::PopID();
    }

    ImGui::EndTable();
  }
}

void drawProperty(kpi::PropertyDelegate<libcube::IBoneDelegate>& delegate,
                  BoneDisplaySurface) {
  auto& bone = delegate.getActive();
  DrawDisplays(bone, delegate);
}

kpi::DecentralizedInstaller AddBoneDisplaySurface([](kpi::ApplicationPlugins&) {
  auto& inst = kpi::PropertyViewManager::getInstance();
  inst.addPropertyView<libcube::IBoneDelegate, BoneDisplaySurface>();
});

} // namespace libcube::UI
