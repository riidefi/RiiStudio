#include <array>
#include <memory>
#include <string>

namespace riistudio {

class JSON;

inline const char* VERSION = "Alpha 3.0";

class Updater {

public:
  Updater();
  ~Updater();
  void draw();

private:
  std::unique_ptr<JSON> mJSON;
  std::string mLatestVer = VERSION;
  bool mShowUpdateDialog = false;
  bool mShowChangelog = false;
  bool mIsInUpdate = false;
  bool mNeedAdmin = false;
  float mUpdateProgress = 0.0f;
  std::string mLaunchPath;
  bool mForceUpdate = false;
  bool mFirstFrame = true;

  void InitRepoJSON();
  bool InstallUpdate();
  std::string ExecutableFilename();
  void RetryAsAdmin();

public:
  void LaunchUpdate(const std::string& new_exe);
  void QueueLaunch(const std::string& path) { mLaunchPath = path; }
  void SetProgress(float progress) { mUpdateProgress = progress; }
  void SetForceUpdate(bool update) { mForceUpdate = update; }
};

} // namespace riistudio
