#include <array>
#include <core/util/timestamp.hpp>
#include <memory>
#include <optional>
#include <string>
#include <thread>

namespace riistudio {

class GithubManifest;

class Updater {
public:
  Updater();
  ~Updater();
  void draw();

private:
  std::unique_ptr<GithubManifest> mJSON;
  std::string mLatestVer = GIT_TAG;
  bool mShowUpdateDialog = false;
  bool mShowChangelog = false;
  bool mIsInUpdate = false;
  bool mNeedAdmin = false;
  float mUpdateProgress = 0.0f;
  std::string mLaunchPath;
  bool mForceUpdate = false;
  bool mFirstFrame = true;

  enum class Action {
    None,
    No,
    Yes,
  };

  Action DrawUpdaterUI(const char* version, std::optional<float> progress);

  std::jthread sThread;

  bool InitRepoJSON();
  bool InstallUpdate();
  void RetryAsAdmin();

public:
  void LaunchUpdate(const std::string& new_exe);
  void QueueLaunch(const std::string& path) { mLaunchPath = path; }
  void SetProgress(float progress) { mUpdateProgress = progress; }
  void SetForceUpdate(bool update) { mForceUpdate = update; }
  bool IsOnline() const { return mJSON != nullptr; }
};

} // namespace riistudio
