#pragma once

#include <string>

namespace rsl {

// Launches an executable with admin permissions
void LaunchAsAdmin(std::string path, std::string arg);

// Launches an executable with user permissions.
void LaunchAsUser(std::string path);

// Wraps GetModuleFileNameA()
std::string GetExecutableFilename();

} // namespace rsl
