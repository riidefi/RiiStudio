#pragma once

#include <core/common.h>

namespace rsl {

Result<std::string> DownloadString(std::string url, std::string user_agent);
void DownloadFile(std::string destPath, std::string url, std::string user_agent,
                  void* progress_func, void* progress_data);

} // namespace rsl
