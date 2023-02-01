#pragma once

#include <string>

namespace rsl {

void LaunchAsAdmin(std::string path, std::string arg);
void LaunchAsUser(std::string path);

} // namespace rsl
