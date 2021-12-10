#include "CppSupport.hpp"
#include <rsl/versions.hpp>
#include <string>

#include <core/util/timestamp.hpp>
// extern const char GIT_TAG[];
// extern const char VERSION_SHORT[];
// extern const char RII_TIME_STAMP[];

#include <imgui/imgui.h>

std::string_view GetCppSupport() {
  static std::string sVersions;

  if (sVersions.empty()) {
    sVersions = "RiiStudio\n\n";

    sVersions += "GIT_TAG = " + std::string(GIT_TAG) + "\n";
    sVersions += "VERSION_SHORT = " + std::string(VERSION_SHORT) + "\n";
    sVersions += "RII_TIME_STAMP = " + std::string(RII_TIME_STAMP) + "\n";

    sVersions += "\n";

    sVersions += rsl::VersionReport();
  }

  return sVersions.c_str();
}

void DrawCppSupport() {
  static ImGuiTableFlags flags =
      ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersHOuter |
      (0 * ImGuiTableFlags_Resizable) | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable |
      ImGuiTableFlags_SizingPolicyFixedX;

  ImGui::TextUnformatted("Timestamp macros");
  if (ImGui::BeginTable("Macros", 2, flags)) {
    auto DrawMacro = [](const char* k, const char* v) {
      ImGui::TableNextRow();
      ImGui::TextUnformatted(k);
      ImGui::TableNextCell();
      ImGui::TextUnformatted(v);
    };

    DrawMacro("GIT_TAG", GIT_TAG);
    DrawMacro("VERSION_SHORT", VERSION_SHORT);
    DrawMacro("RII_TIME_STAMP", RII_TIME_STAMP);

    ImGui::EndTable();
  }
  ImGui::TextUnformatted("Unsupported language features");
  if (ImGui::BeginTable("Lang", 1, flags)) {
    auto Draw = [](const char* k) {
      ImGui::TableNextRow();
      ImGui::TextUnformatted(k);
    };

    rsl::LanguageFeatures([](const char* feature, int ver) {},
                          [&](const char* feature) { Draw(feature); });

    ImGui::EndTable();
  }
  ImGui::TextUnformatted("Unsupported library features");
  if (ImGui::BeginTable("Lib", 1, flags)) {
    auto Draw = [](const char* k) {
      ImGui::TableNextRow();
      ImGui::TextUnformatted(k);
    };

    rsl::LibraryFeatures([](const char* feature, int ver) {},
                         [&](const char* feature) { Draw(feature); });

    ImGui::EndTable();
  }
}
