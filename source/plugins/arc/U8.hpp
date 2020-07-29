#pragma once

#include <oishii/reader/binary_reader.hxx>
#include <plugins/arc/Arc.hpp>


namespace riistudio::arc::u8 {

void readArchive(riistudio::arc::Archive& dst, oishii::BinaryReader& reader);

} // namespace riistudio::arc::u8
