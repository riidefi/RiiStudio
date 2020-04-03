#pragma once

#include <core/common.h>
#include <oishii/reader/binary_reader.hxx>
#include "OutputCtx.hpp"
#include <oishii/v2/writer/node.hxx>

namespace riistudio::j3d {
    
void readINF1(BMDOutputContext& ctx);

void readVTX1(BMDOutputContext& ctx);
void readEVP1DRW1(BMDOutputContext& ctx);
void readJNT1(BMDOutputContext& ctx);
void readMAT3(BMDOutputContext& ctx);
void readSHP1(BMDOutputContext& ctx);
void readTEX1(BMDOutputContext& ctx);


std::unique_ptr<oishii::v2::Node> makeINF1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::v2::Node> makeVTX1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::v2::Node> makeEVP1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::v2::Node> makeDRW1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::v2::Node> makeJNT1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::v2::Node> makeSHP1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::v2::Node> makeMAT3Node(BMDExportContext& ctx);
std::unique_ptr<oishii::v2::Node> makeTEX1Node(BMDExportContext& ctx);

}
