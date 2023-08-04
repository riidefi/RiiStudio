#include <optional>
#include <string>

namespace riistudio {

// Opaque updater type
class Updater;

Updater* Updater_Create();
void Updater_Destroy(Updater* updater);

// Only Windows builds are published
bool Updater_CanUpdate(Updater& updater);

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

// Are we requesting an update because our current install is bad?
bool Updater_InRecoveryMode(Updater& updater);

// Get the changelog of the latest release.
std::optional<std::string> Updater_GetChangeLog(Updater& updater);

} // namespace riistudio
