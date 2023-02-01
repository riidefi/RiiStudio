#include "updater.hpp"

#ifndef _WIN32
namespace riistudio {
Updater::Updater() = default;
Updater::~Updater() = default;
void Updater::draw() {}
class JSON {};
} // namespace riistudio
#else

#include <frontend/applet.hpp>
#include <frontend/widgets/changelog.hpp>
#include <imcxx/Widgets.hpp>
#include <io.h>
#include <rsl/Download.hpp>
#include <rsl/Launch.hpp>
#include <rsl/Zip.hpp>

#include "GithubManifest.hpp"

IMPORT_STD;
#include <iostream>

namespace riistudio {

static const char* GITHUB_REPO = "riidefi/RiiStudio";
static const char* USER_AGENT = "RiiStudio";

Updater::Updater() {
  if (!InitRepoJSON()) {
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

  std::error_code ec;
  auto temp_dir = std::filesystem::temp_directory_path(ec);

  if (ec) {
    std::cout << ec.message() << std::endl;
    return;
  }

  const auto temp = temp_dir / "RiiStudio_temp.exe";

  if (std::filesystem::exists(temp)) {
    mShowChangelog = true;
    remove(temp);
  }
}

Updater::~Updater() {}

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
    printf("Launching update %s\n", mLaunchPath.c_str());
    LaunchUpdate(mLaunchPath);
    // (Never reached)
    mShowUpdateDialog = false;
  }

  if (!mShowUpdateDialog)
    return;

  const auto wflags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

  ImGui::OpenPopup("RiiStudio Update"_j);
  if (ImGui::BeginPopupModal("RiiStudio Update"_j, nullptr, wflags)) {
    if (mIsInUpdate) {
      ImGui::Text("Installing Riistudio %s.."_j, mLatestVer.c_str());
      ImGui::ProgressBar(mUpdateProgress);
    } else {
      ImGui::Text("A new version of RiiStudio (%s) was found. Would you like "
                  "to update?"_j,
                  mLatestVer.c_str());
      if (ImGui::Button("Yes"_j, ImVec2(170, 0))) {
        if (!InstallUpdate() && mNeedAdmin) {
          RetryAsAdmin();
          // (Never reached)
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("No"_j, ImVec2(170, 0))) {
        mShowUpdateDialog = false;
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::EndPopup();
  }
  mFirstFrame = false;
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

static std::jthread sThread;

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
  // On slow machines, the thread may not have been joined yet--so wait for it.
  sThread.join();
  assert(!sThread.joinable());
  rsl::LaunchAsUser(new_exe);
  exit(0);
}

} // namespace riistudio

#endif
