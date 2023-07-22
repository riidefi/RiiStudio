#define UPDATER_INTERNAL

#include "updater.hpp"

#include "GithubManifest.hpp"
#ifdef _WIN32
#include <io.h>
#endif
#include <iostream>
#include <rsl/Download.hpp>
#include <rsl/FsDialog.hpp>
#include <rsl/Launch.hpp>
#include <rsl/Log.hpp>
#include <rsl/Zip.hpp>

#include <array>
#include <core/util/timestamp.hpp>
#include <memory>
#include <thread>

namespace riistudio {

///
/// The following is implementation-defined and will hopefully soon be replaced
/// with Rust code.
///

class GithubManifest;

class Updater {
public:
  Updater();
  ~Updater();

#if defined(UPDATER_INTERNAL)
public:
#else
private:
#endif
  void Calc();
  bool mIsInUpdate = false;
  float mUpdateProgress = 0.0f;
  void StartUpdate() {
    if (!InstallUpdate() && mNeedAdmin) {
      RetryAsAdmin();
      // (Never reached)
    }
  }
  std::optional<std::string> GetChangeLog() const;
  std::string mLatestVer = GIT_TAG;
  bool WasJustUpdated() const { return mHasChangeLog; }
  bool HasPendingUpdate() const { return mHasPendingUpdate; }

private:
  std::unique_ptr<GithubManifest> mJSON;
  bool mNeedAdmin = false;
  std::string mLaunchPath;
  bool mForceUpdate = false;

#ifdef _WIN32
  std::jthread sThread;
#endif

  bool mHasChangeLog = false;
  bool mHasPendingUpdate = false;

  bool InitRepoJSON();
  bool InstallUpdate();
  void RetryAsAdmin();

#if defined(UPDATER_INTERNAL)
public:
#else
private:
#endif
  void LaunchUpdate(const std::string& new_exe);
  void QueueLaunch(const std::string& path) { mLaunchPath = path; }
  void SetProgress(float progress) { mUpdateProgress = progress; }
  void SetForceUpdate(bool update) { mForceUpdate = update; }
  bool IsOnline() const { return mJSON != nullptr; }
};

Updater* Updater_Create() { return new Updater; }
void Updater_Destroy(Updater* updater) { delete updater; }

bool Updater_CanUpdate(Updater& updater) {
#ifdef _WIN32
  return true;
#else
  return false;
#endif
}

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
    rsl::error("Cannot connect to Github");
    return;
  }
  if (auto name = mJSON->name(); name.has_value()) {
    mLatestVer = *name;
  }

  if (mLatestVer.empty()) {
    rsl::error("There is no name field");
    return;
  }

  const auto current_exe = rsl::GetExecutableFilename();
  if (current_exe.empty()) {
    rsl::error("Cannot get the name of the current .exe");
    return;
  }

  mHasPendingUpdate = GIT_TAG != mLatestVer;

  auto temp_dir = GetTempDirectory();
  if (!temp_dir) {
    rsl::error("Cannot get temporary directory: {}", temp_dir.error());
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
    rsl::error("{}", manifest.error());
    return false;
  }
  mJSON = std::make_unique<GithubManifest>(*manifest);
  return true;
}

bool Updater::InstallUpdate() {
#ifndef _WIN32
  return false;
#else
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
    rsl::error("Failed to rename: {}. Retrying with admin perms.",
               error_code.message());
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
#endif
}

void Updater::RetryAsAdmin() {
#ifdef _WIN32
  auto path = rsl::GetExecutableFilename();
  rsl::LaunchAsAdmin(path, "--update");
  exit(0);
#endif
}

void Updater::LaunchUpdate(const std::string& new_exe) {
#ifdef _WIN32
  rsl::info("Launching update {}", new_exe);
  // On slow machines, the thread may not have been joined yet--so wait for it.
  sThread.join();
  assert(!sThread.joinable());
  rsl::LaunchAsUser(new_exe);
  exit(0);
#endif
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
