#pragma once

#include <LibRiiEditor/common.hpp>
#include <oishii/reader/binary_reader.hxx>
#include <LibCube/JSystem/J3D/Binary/BMD/OutputCtx.hpp>

namespace libcube::jsystem {
    
void readINF1(BMDOutputContext& ctx);
void readEVP1DRW1(BMDOutputContext& ctx);
void readJNT1(BMDOutputContext& ctx);
void readMAT3(BMDOutputContext& ctx);
void readVTX1(BMDOutputContext& ctx);
void readSHP1(BMDOutputContext& ctx);

}