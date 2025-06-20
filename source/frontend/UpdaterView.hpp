#include <updater/updater.hpp>

#include <memory>

namespace riistudio {

class UpdaterView {
public:
  UpdaterView();

  void draw();

  // If the caller sees --update in command line args.
  void SetForceUpdate(bool update) {
    Updater_SetForceUpdate(*mUpdater, update);
  }

  // Check if the Updater is usable
  bool IsOnline() { return Updater_IsOnline(*mUpdater); }

private:
  enum class Action {
    None,
    No,
    Yes,
  };

  Action DrawUpdaterUI(const char* version, std::optional<float> progress);

  std::unique_ptr<Updater, void (*)(Updater*)> mUpdater;

public:
  bool mShowChangelog = false;

private:
  bool mShowUpdateDialog = false;
  bool mHasHandledPostUpdateUITasks = false;
};

} // namespace riistudio
