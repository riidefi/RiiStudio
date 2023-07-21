#pragma once

#include <core/common.h>
#include <span>
#include <string_view>

namespace librii::wbz {

Result<std::vector<u8>> decodeWBZ(std::span<const u8> buf,
                                   std::string_view autoadd_path);

Result<void> decodeWU8Inplace(std::span<u8> buf, std::string_view autoadd_path);

} // namespace librii::wbz
