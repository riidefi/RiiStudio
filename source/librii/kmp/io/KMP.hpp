#pragma once

#include <librii/kmp/CourseMap.hpp>
#include <oishii/writer/binary_writer.hxx>

namespace librii::kmp {

void readKMP(CourseMap& map, oishii::ByteView&& data);
void writeKMP(const CourseMap& map, oishii::Writer& writer);

} // namespace librii::kmp
