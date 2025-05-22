#include "G3dUniformAnimView.hpp"
#include <imcxx/Widgets.hpp>
#include <librii/g3d/io/ArchiveIO.hpp>
#include <librii/g3d/io/JSON.hpp>
#include <nlohmann/json.hpp>

namespace riistudio::g3d {

void drawProperty(kpi::PropertyDelegate<riistudio::g3d::CLR0>& dl,
                 G3dUniformAnimDataSurface) {
  auto& anim = dl.getActive();
  
  // Create a BRRES archive with just this CLR0
  librii::g3d::Archive archive;
  archive.clrs.push_back(anim);

  // Convert to JSON
  auto result = librii::g3d::DumpJson(archive);
  
  // Parse the JSON with nlohmann
  nlohmann::json jsonObj = nlohmann::json::parse(result.jsonData);
  std::string clrJson;
  
  // Extract only the first CLR animation data
  if (jsonObj.contains("clrs") && !jsonObj["clrs"].empty()) {
    clrJson = jsonObj["clrs"][0].dump(2); // Pretty print with 2-space indent
  }
  
  // Display the animation name and JSON
  if (ImGui::BeginTable("UniformAnimData", 2)) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Name");
    ImGui::TableNextColumn();
    ImGui::Text("%s", anim.name.c_str());
    ImGui::EndTable();
  }

  // Display only the CLR0 JSON representation
  ImGui::Text("JSON Representation:");
  ImGui::BeginChild("JSONView", ImVec2(0, 300), true);
  ImGui::TextWrapped("%s", clrJson.c_str());
  ImGui::EndChild();
}

} // namespace riistudio::g3d
