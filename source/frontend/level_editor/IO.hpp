#pragma once

#include <librii/kcol/Model.hpp>
#include <librii/kmp/CourseMap.hpp>
#include <plugins/g3d/collection.hpp>

namespace riistudio::lvl {

enum class NeedResave { Default, AllowUnwritable };

[[nodiscard]] Result<std::unique_ptr<g3d::Collection>>
ReadBRRES(const std::vector<u8>& buf, std::string path,
          NeedResave need_resave = NeedResave::AllowUnwritable);

[[nodiscard]] Result<std::unique_ptr<librii::kmp::CourseMap>>
ReadKMP(const std::vector<u8>& buf, std::string path);

[[nodiscard]] std::vector<u8> WriteKMP(const librii::kmp::CourseMap& map);

[[nodiscard]] Result<std::unique_ptr<librii::kcol::KCollisionData>>
ReadKCL(const std::vector<u8>& buf, std::string path);

} // namespace riistudio::lvl
