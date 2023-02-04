#include "updater.hpp"

#ifndef _WIN32
namespace riistudio {
class GithubManifest {};
Updater::Updater() = default;
Updater::~Updater() = default;
void Updater::draw() {}
} // namespace riistudio
#else

#include "GithubManifest.hpp"
#include <frontend/applet.hpp>
#include <frontend/widgets/changelog.hpp>
#include <imcxx/Widgets.hpp>
#include <io.h>
#include <iostream>
#include <rsl/Defer.hpp>
#include <rsl/Download.hpp>
#include <rsl/FsDialog.hpp>
#include <rsl/Launch.hpp>
#include <rsl/Zip.hpp>

namespace riistudio {

Result<std::filesystem::path> GetTempDirectory() {
  std::error_code ec;
  auto temp_dir = std::filesystem::temp_directory_path(ec);

  if (ec) {
    return std::unexpected(ec.message());
  }
  return temp_dir;
}

static const char* GITHUB_REPO = "riidefi/RiiStudio";
static const char* USER_AGENT = "RiiStudio";

Updater::Updater() {
  if (!InitRepoJSON()) {
    rsl::ErrorDialogFmt("Updater Error\n\n"
                        "Cannot connect to Github to check for updates.");
    fprintf(stderr, "Cannot connect to Github\n");
    return;
  }
  if (auto name = mJSON->name(); name.has_value()) {
    mLatestVer = *name;
  }

  if (mLatestVer.empty()) {
    fprintf(stderr, "There is no name field\n");
    return;
  }

  const auto current_exe = rsl::GetExecutableFilename();
  if (current_exe.empty()) {
    fprintf(stderr, "Cannot get the name of the current .exe\n");
    return;
  }

  mShowUpdateDialog = GIT_TAG != mLatestVer;

  auto temp_dir = GetTempDirectory();
  if (!temp_dir) {
    fprintf(stderr, "Cannot get temporary directory: %s\n",
            temp_dir.error().c_str());
    return;
  }

  const auto temp = (*temp_dir) / "RiiStudio_temp.exe";

  if (std::filesystem::exists(temp)) {
    mShowChangelog = true;
    std::filesystem::remove(temp);
  }
}

Updater::~Updater() = default;

void Updater::draw() {
  if (mJSON == nullptr)
    return;

  if (auto body = mJSON->body(); body.has_value()) {
    DrawChangeLog(&mShowChangelog, *body);
  }

  // Hack.. wait one frame for the UI to size properly
  if (mForceUpdate && !mFirstFrame) {
    if (!InstallUpdate()) {
      // Give up.. admin perms didn't solve our problem
    }

    mForceUpdate = false;
  }

  if (!mLaunchPath.empty()) {
    LaunchUpdate(mLaunchPath);
    // (Never reached)
    mShowUpdateDialog = false;
  }

  if (!mShowUpdateDialog)
    return;

  std::optional<float> progress;
  if (mIsInUpdate) {
    progress = mUpdateProgress;
  }
  auto action = DrawUpdaterUI(mLatestVer.c_str(), progress);

  switch (action) {
  case Action::None: {
    break;
  }
  case Action::No: {
    mShowUpdateDialog = false;
    ImGui::CloseCurrentPopup();
    break;
  }
  case Action::Yes: {
    if (!InstallUpdate() && mNeedAdmin) {
      RetryAsAdmin();
      // (Never reached)
    }
    break;
  }
  }
  mFirstFrame = false;
}

Updater::Action Updater::DrawUpdaterUI(const char* version,
                                       std::optional<float> progress) {
  Action action = Action::None;

  const auto wflags =
      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

  ImGui::OpenPopup("RiiStudio Update"_j);

  {
    auto pos = ImGui::GetWindowPos();
    auto avail = ImGui::GetWindowSize();
    auto sz = ImVec2(600, 84);
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

      auto button = ImVec2{75, 0};
      ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - button.x * 2);
      {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 0, 0, 100));
        RSL_DEFER(ImGui::PopStyleColor());
        if (ImGui::Button("No"_j, button)) {
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

    ImGui::EndPopup();
  }

  return action;
}

bool Updater::InitRepoJSON() {
  auto manifest = DownloadLatestRelease(GITHUB_REPO, USER_AGENT);
  if (!manifest) {
    fprintf(stderr, "%s\n", manifest.error().c_str());
    return false;
  }
  mJSON = std::make_unique<GithubManifest>(*manifest);
  return true;
}

bool Updater::InstallUpdate() {
  const auto current_exe = rsl::GetExecutableFilename();
  if (current_exe.empty())
    return false;

  auto assets = mJSON->getAssets();
  if (assets.size() < 1) {
    return false;
  }
  const auto url = assets[0].browser_download_url;

  const auto folder = std::filesystem::path(current_exe).parent_path();
  const auto new_exe = folder / "RiiStudio.exe";
  const auto temp_exe =
      std::filesystem::temp_directory_path() / "RiiStudio_temp.exe";
  const auto download = folder / "download.zip";

  std::error_code error_code;
  std::filesystem::rename(current_exe, temp_exe, error_code);

  if (error_code) {
    std::cout << error_code.message() << std::endl;
    // Request admin perms...
    mNeedAdmin = true;
    return false;
  }

  auto progress_func =
      +[](void* userdata, double total, double current, double, double) {
        Updater* updater = reinterpret_cast<Updater*>(userdata);
        updater->SetProgress(current / total);
        return 0;
      };

  mIsInUpdate = true;
  sThread = std::jthread(
      [=](Updater* updater) {
        rsl::DownloadFile(download.string(), url, USER_AGENT, progress_func,
                          this);
        rsl::ExtractZip(download.string(), folder.string());
        std::filesystem::remove(download);
        updater->QueueLaunch(new_exe.string());
      },
      this);
  return true;
}

void Updater::RetryAsAdmin() {
  auto path = rsl::GetExecutableFilename();
  rsl::LaunchAsAdmin(path, "--update");
  exit(0);
}

void Updater::LaunchUpdate(const std::string& new_exe) {
  printf("Launching update %s\n", new_exe.c_str());
  // On slow machines, the thread may not have been joined yet--so wait for it.
  sThread.join();
  assert(!sThread.joinable());
  rsl::LaunchAsUser(new_exe);
  exit(0);
}

} // namespace riistudio

#endif
