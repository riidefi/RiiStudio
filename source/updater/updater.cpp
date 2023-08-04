#include "updater.hpp"

#include "GithubManifest.hpp"
#ifdef _WIN32
#include <io.h>
#endif
#include <iostream>
#include <rsl/Download.hpp>
#include <rsl/Filesystem.hpp>
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

  Result<void> Init();

  void Calc();
  bool mIsInUpdate = false;
  float mUpdateProgress = 0.0f;
  void StartUpdate() {
    auto installOk = InstallUpdate();
    if (!installOk) {
      rsl::error(installOk.error());
      rsl::ErrorDialogFmt("Updater Error\n\n{}", installOk.error());
    }

    if (!installOk && mNeedAdmin) {
      RetryAsAdmin();
      // (Never reached)
    }
  }
  std::optional<std::string> GetChangeLog() const;
  std::string mLatestVer = GIT_TAG;
  bool WasJustUpdated() const { return mHasChangeLog; }
  bool HasPendingUpdate() const { return mHasPendingUpdate; }
  bool InRecoveryMode() const {
    return mInCurrentlyCorruptedStateFromPastUpdate;
  }

private:
  std::unique_ptr<GithubManifest> mJSON;
  bool mNeedAdmin = false;
  std::string mLaunchPath;
  bool mForceUpdate = false;

#ifdef _WIN32
  std::jthread sThread;
#endif

  bool mInCurrentlyCorruptedStateFromPastUpdate = false;
  bool mHasChangeLog = false;
  bool mHasPendingUpdate = false;

  Result<void> InitRepoJSON();
  Result<void> InstallUpdate();
  void RetryAsAdmin();

public:
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

#define FS_TRY(expr)                                                           \
  TRY(expr.transform_error([](const std::error_code& ec) -> std::string {      \
    return std::format("Filesystem Error: {} ({}:{})", ec.message(), __FILE__, \
                       __LINE__);                                              \
  }))

static const char* GITHUB_REPO = "riidefi/RiiStudio";
static const char* USER_AGENT = "RiiStudio";

Result<bool> FilesFromSameUpdateRun_Impl(const std::filesystem::path& file1,
                                         const std::filesystem::path& file2) {
  if (!FS_TRY(rsl::filesystem::exists(file1)) ||
      !FS_TRY(rsl::filesystem::exists(file2))) {
    return false;
  }

  auto ftime1 = FS_TRY(rsl::filesystem::last_write_time(file1));
  auto ftime2 = FS_TRY(rsl::filesystem::last_write_time(file2));

  auto sctime1 =
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(
          ftime1 - std::filesystem::file_time_type::clock::now() +
          std::chrono::system_clock::now());
  auto sctime2 =
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(
          ftime2 - std::filesystem::file_time_type::clock::now() +
          std::chrono::system_clock::now());

  auto diff = sctime1 - sctime2;
  auto diff_minutes = std::chrono::duration_cast<std::chrono::minutes>(diff);

  u64 minute_difference = static_cast<u64>(std::abs(diff_minutes.count()));
  rsl::info("FilesFromSameUpdateRun_Impl: Minute difference: {}; Stale: {}\n",
            minute_difference, minute_difference > 5);

  // Determines if two files are more than five minutes apart in last replaced
  // a good indication if we have a partial/stale update
  return minute_difference <= 5;
}
bool FilesFromSameUpdateRun(const std::filesystem::path& file1,
                            const std::filesystem::path& file2) {
  return FilesFromSameUpdateRun_Impl(file1, file2).value_or(false);
}

extern "C" bool RII_IS_DIST_BUILD;

bool IsUpdateStale(Updater& updater, std::filesystem::path root) {
  if (!Updater_CanUpdate(updater)) {
    // Don't enter into recovery mode if recovery is not possible
    return false;
  }
  if (!RII_IS_DIST_BUILD) {
    // Desync'd files are common in dev builds
    return false;
  }
#ifdef _WIN32
  // These filepaths are Windows-specific
  return !FilesFromSameUpdateRun(root / "RiiStudio.exe", root / "gctex.dll");
#else
  return false;
#endif
}

Updater::Updater() {
  auto ok = Init();
  if (!ok) {
    rsl::error(ok.error());
    rsl::ErrorDialogFmt("Updater Error\n\n{}", ok.error());
  }
}

Result<void> Updater::Init() {
  if (auto ok = InitRepoJSON(); !ok) {
    return std::unexpected("Cannot connect to Github to check for updates: " +
                           ok.error());
  }
  if (auto name = mJSON->name(); name.has_value()) {
    mLatestVer = *name;
  }

  if (mLatestVer.empty()) {
    return std::unexpected("There is no name field");
  }

  const auto current_exe = rsl::GetExecutableFilename();
  if (current_exe.empty()) {
    return std::unexpected("Cannot get the name of the current .exe");
  }

  if (IsUpdateStale(*this, std::filesystem::path(current_exe).parent_path())) {
    std::cout << "Update is stale!\n" << std::endl;
    mInCurrentlyCorruptedStateFromPastUpdate = true;
  }

  mHasPendingUpdate =
      GIT_TAG != mLatestVer || mInCurrentlyCorruptedStateFromPastUpdate;

  auto temp_dir = rsl::filesystem::temp_directory_path();
  if (!temp_dir) {
    return std::unexpected("Cannot get temporary directory: " +
                           temp_dir.error().message());
  }

  const auto temp = (*temp_dir) / "RiiStudio_temp.exe";

  if (FS_TRY(rsl::filesystem::exists(temp))) {
    mHasChangeLog = true;
    FS_TRY(rsl::filesystem::remove(temp));
  }
  return {};
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

Result<void> Updater::InitRepoJSON() {
  auto manifest = TRY(DownloadLatestRelease(GITHUB_REPO, USER_AGENT));
  mJSON = std::make_unique<GithubManifest>(manifest);
  return {};
}

Result<void> Updater::InstallUpdate() {
#ifndef _WIN32
  return std::unexpected(
      "Updating from a prebuilt binary is not supported on Linux/MacOS");
#else
  const auto current_exe = rsl::GetExecutableFilename();
  if (current_exe.empty())
    return std::unexpected("Failed to get path of .exe file");

  auto assets = mJSON->getAssets();
  if (assets.size() < 1) {
    return std::unexpected("Release has no prebuilt binaries on GitHub");
  }
  const auto url = assets[0].browser_download_url;

  const auto folder = std::filesystem::path(current_exe).parent_path();
  const auto new_exe = folder / "RiiStudio.exe";
  const auto temp_exe =
      FS_TRY(rsl::filesystem::temp_directory_path()) / "RiiStudio_temp.exe";
  const auto download = folder / "download.zip";

  auto rename_ok = rsl::filesystem::rename(current_exe, temp_exe);

  if (!rename_ok) {
    // Request admin perms...
    mNeedAdmin = true;
    return std::unexpected(
        std::format("Failed to rename: {}. Retrying with admin perms.",
                    rename_ok.error().message()));
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
        auto ok = rsl::filesystem::remove(download);
        if (!ok) {
          rsl::ErrorDialogFmt("Failed to remove {}", download.string());
        }
        updater->QueueLaunch(new_exe.string());
      },
      this);
  return {};
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

// Are we requesting an update because our current install is bad?
bool Updater_InRecoveryMode(Updater& updater) {
  return updater.InRecoveryMode();
}

// Get the changelog of the latest release.
std::optional<std::string> Updater_GetChangeLog(Updater& updater) {
  return updater.GetChangeLog();
}

} // namespace riistudio
