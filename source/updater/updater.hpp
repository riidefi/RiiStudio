#include <array>
#include <core/util/timestamp.hpp>
#include <memory>
#include <optional>
#include <string>
#include <thread>

namespace riistudio {

// Opaque updater type
class Updater;

// Check if the Updater is usable
bool Updater_IsOnline(Updater& updater);

// If the caller sees --update in command line args.
void Updater_SetForceUpdate(Updater& updater, bool update);

// Called every frame. Handles --update and such.
void Updater_Calc(Updater& updater);

// Is there a newer version available on GitHub?
bool Updater_HasAvailableUpdate(Updater& updater);

// What is the latest release version?
std::string Updater_LatestVer(Updater& updater);

// Starts the update process.
void Updater_StartUpdate(Updater& updater);

// Is an update in progress?
bool Updater_IsUpdating(Updater& updater);

// What is the % progress of the current download?
// Returns in range [0, 1]
float Updater_Progress(Updater& updater);

// Did an update (and restart) just complete?
bool Updater_WasUpdated(Updater& updater);

// Get the changelog of the latest release.
std::optional<std::string> Updater_GetChangeLog(Updater& updater);

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

} // namespace riistudio
