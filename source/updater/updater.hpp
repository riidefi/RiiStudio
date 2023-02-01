#include <array>
#include <core/util/timestamp.hpp>
#include <memory>
#include <string>

namespace riistudio {

class JSON;



class Updater {
public:
  Updater();
  ~Updater();
  void draw();

private:
  std::unique_ptr<JSON> mJSON;
  std::string mLatestVer = GIT_TAG;
  bool mShowUpdateDialog = false;
  bool mShowChangelog = false;
  bool mIsInUpdate = false;
  bool mNeedAdmin = false;
  float mUpdateProgress = 0.0f;
  std::string mLaunchPath;
  bool mForceUpdate = false;
  bool mFirstFrame = true;

  bool InitRepoJSON();
  bool InstallUpdate();
  std::string ExecutableFilename();
  void RetryAsAdmin();

public:
  void LaunchUpdate(const std::string& new_exe);
  void QueueLaunch(const std::string& path) { mLaunchPath = path; }
  void SetProgress(float progress) { mUpdateProgress = progress; }
  void SetForceUpdate(bool update) { mForceUpdate = update; }
  bool IsOnline() const { return mJSON != nullptr; }
};

} // namespace riistudio
