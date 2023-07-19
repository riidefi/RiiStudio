#include "UpdaterView.hpp"
#include <frontend/widgets/changelog.hpp>
#include <imcxx/Widgets.hpp>
#include <rsl/Defer.hpp>

namespace riistudio {

UpdaterView::UpdaterView() {
  mShowUpdateDialog = Updater_HasAvailableUpdate(mUpdater);
}
void UpdaterView::draw() {
  if (!Updater_IsOnline(mUpdater))
    return;

  if (Updater_WasUpdated(mUpdater)) {
    if (auto body = Updater_GetChangeLog(mUpdater); body.has_value()) {
      DrawChangeLog(&mShowChangelog, *body);
    }
  }

  Updater_Calc(mUpdater);

  if (!mShowUpdateDialog)
    return;

  std::optional<float> progress;
  if (Updater_IsUpdating(mUpdater)) {
    progress = Updater_Progress(mUpdater);
  }
  auto action = DrawUpdaterUI(Updater_LatestVer(mUpdater).c_str(), progress);
  switch (action) {
  case Action::None: {
    break;
  }
  case Action::No: {
    mShowUpdateDialog = false;
    break;
  }
  case Action::Yes: {
    Updater_StartUpdate(mUpdater);
    break;
  }
  }
}

UpdaterView::Action UpdaterView::DrawUpdaterUI(const char* version,
                                               std::optional<float> progress) {
  Action action = Action::None;

  const auto wflags =
      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

  ImGui::OpenPopup("RiiStudio Update"_j);

  {
    auto pos = ImGui::GetWindowPos();
    auto avail = ImGui::GetWindowSize();
    auto sz = ImVec2(600, 84);
    if (!Updater_CanUpdate(mUpdater)) {
      // Long explanation text
      sz.y = 176;
    }
    ImGui::SetNextWindowPos(
        ImVec2(pos.x + avail.x / 2 - sz.x / 2, pos.y + avail.y / 2 - sz.y));
    ImGui::SetNextWindowSize(sz);
  }
  if (ImGui::BeginPopupModal("RiiStudio Update"_j, nullptr, wflags)) {
    if (progress.has_value()) {
      ImGui::Text("Installing Riistudio %s.."_j, version);
      ImGui::Separator();
      ImGui::ProgressBar(*progress);
    } else {
      ImGui::Text("A new version of RiiStudio (%s) was found. Would you like "
                  "to update?"_j,
                  version);

      ImGui::Separator();

      if (!Updater_CanUpdate(mUpdater)) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error:");
        ImGui::TextWrapped(
            "Unfortunately, only Windows builds are published "
            "currently. You can run `git pull origin master` to update the "
            "source code, and then `cmake --build . --config Release "
            "--parallel` in your `build` directory to continue with your last "
            "settings. In a differential build, only new code will be "
            "compiled, resulting in a much faster process.");
        ImGui::Separator();
        if (ImGui::Button("OK"_j)) {
          ImGui::CloseCurrentPopup();
          action = Action::No;
        }
      }

      if (Updater_CanUpdate(mUpdater)) {
        auto button = ImVec2{75, 0};
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - button.x * 2);
        {
          ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 0, 0, 100));
          RSL_DEFER(ImGui::PopStyleColor());
          if (ImGui::Button("No"_j, button)) {
            ImGui::CloseCurrentPopup();
            action = Action::No;
          }
        }
        ImGui::SameLine();
        {
          ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 255, 0, 100));
          RSL_DEFER(ImGui::PopStyleColor());
          if (ImGui::Button("Yes"_j, button)) {
            action = Action::Yes;
          }
        }
      }
    }

    ImGui::EndPopup();
  }

  return action;
}

} // namespace riistudio
