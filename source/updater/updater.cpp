#define UPDATER_INTERNAL

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
#include <io.h>
#include <iostream>
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

  mHasPendingUpdate = GIT_TAG != mLatestVer;

  auto temp_dir = GetTempDirectory();
  if (!temp_dir) {
    fprintf(stderr, "Cannot get temporary directory: %s\n",
            temp_dir.error().c_str());
    return;
  }

  const auto temp = (*temp_dir) / "RiiStudio_temp.exe";

  if (std::filesystem::exists(temp)) {
    mHasChangeLog = true;
    std::filesystem::remove(temp);
  }
}

Updater::~Updater() = default;

void Updater::Calc() {
  if (mForceUpdate) {
    if (!InstallUpdate()) {
      // Give up.. admin perms didn't solve our problem
    }

    mForceUpdate = false;
  }

  if (!mLaunchPath.empty()) {
    LaunchUpdate(mLaunchPath);
    // (Never reached)
  }
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
std::optional<std::string> Updater::GetChangeLog() const {
  auto b = mJSON->body();
  if (b.has_value()) {
    return *b;
  }
  return std::nullopt;
}

// Check if the Updater is usable
bool Updater_IsOnline(Updater& updater) { return updater.IsOnline(); }

// If the caller sees --update in command line args.
void Updater_SetForceUpdate(Updater& updater, bool update) {
  updater.SetForceUpdate(update);
}

// Called every frame. Handles --update and such.
void Updater_Calc(Updater& updater) { updater.Calc(); }

// Is there a version version available on GitHub?
bool Updater_HasAvailableUpdate(Updater& updater) {
  return updater.HasPendingUpdate();
}

// What is the latest release version?
std::string Updater_LatestVer(Updater& updater) { return updater.mLatestVer; }

// Starts the update process.
void Updater_StartUpdate(Updater& updater) { updater.StartUpdate(); }

// Is an update in progress?
bool Updater_IsUpdating(Updater& updater) { return updater.mIsInUpdate; }

// What is the % progress of the current download?
// Returns in range [0, 1]
float Updater_Progress(Updater& updater) { return updater.mUpdateProgress; }

// Did an update (and restart) just complete?
bool Updater_WasUpdated(Updater& updater) { return updater.WasJustUpdated(); }

// Get the changelog of the latest release.
std::optional<std::string> Updater_GetChangeLog(Updater& updater) {
  return updater.GetChangeLog();
}

} // namespace riistudio

#endif
