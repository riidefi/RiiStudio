#include <array>
#include <string>

namespace riistudio {

class Updater {
public:
  Updater();
  void draw();

private:
  std::string mJSON;
  std::string mLatestVer;
  bool mShowUpdateDialog = false;
  bool mIsInUpdate = false;
  float mUpdateProgress = 0.0f;
  std::string mLaunchPath;

  void InitRepoJSON();
  void InstallUpdate();
  std::string ParseJSON(std::string key, std::string defaultValue);
  std::string ExecutableFilename();

public:
  void LaunchUpdate(const std::string& new_exe);
  void QueueLaunch(const std::string& path) { mLaunchPath = path; }
  void SetProgress(float progress) { mUpdateProgress = progress; }
};

} // namespace riistudio
