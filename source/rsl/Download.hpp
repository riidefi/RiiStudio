#pragma once

#include <core/common.h>

namespace rsl {

Result<std::string> DownloadString(std::string url, std::string user_agent);
void DownloadFile(std::string destPath, std::string url, std::string user_agent,
                  int (*progress_func)(void* userdata, double total,
                                        double current, double, double),
                  void* progress_data);

} // namespace rsl
