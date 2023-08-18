// @file PreciseBMDDump.hpp
//
// @brief For generating KCL testing suite data, we dump "Jmp2Bmd" files to
// recover original triangle data -- kcl files, on their own, are quite lossy.
//
#pragma once

#include <rsl/Expected.hpp>
#include <string>
#include <string_view>

namespace librii::j3d {

struct J3dModel;

std::expected<std::string, std::string>
PreciseBMDDump(const J3dModel& bmd, std::string_view debug_name);

} // namespace librii::j3d
