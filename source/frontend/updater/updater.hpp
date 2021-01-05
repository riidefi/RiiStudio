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
  bool mShowUpdateDialog;

  void InitRepoJSON();
  void InstallUpdate();
  std::string ParseJSON(std::string key, std::string defaultValue);
  std::string ExecutableFilename();
};

} // namespace riistudio
