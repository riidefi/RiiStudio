#pragma once

#include <plugins/g3d/collection.hpp>
#include <librii/kcol/Model.hpp>
#include <librii/kmp/CourseMap.hpp>

namespace riistudio::lvl {

enum class NeedResave { Default, AllowUnwritable };

std::unique_ptr<g3d::Collection>
ReadBRRES(const std::vector<u8>& buf, std::string path,
          NeedResave need_resave = NeedResave::AllowUnwritable);

std::unique_ptr<librii::kmp::CourseMap> ReadKMP(const std::vector<u8>& buf,
                                                std::string path);

std::vector<u8> WriteKMP(const librii::kmp::CourseMap& map);

std::unique_ptr<librii::kcol::KCollisionData>
ReadKCL(const std::vector<u8>& buf, std::string path);

} // namespace riistudio::lvl
