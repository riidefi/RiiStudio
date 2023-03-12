#pragma once

#include <librii/kmp/CourseMap.hpp>
#include <oishii/writer/binary_writer.hxx>

namespace librii::kmp {

Result<CourseMap> readKMP(std::span<const u8> data);
void writeKMP(const CourseMap& map, oishii::Writer& writer);

} // namespace librii::kmp
