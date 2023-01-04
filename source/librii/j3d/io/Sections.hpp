#pragma once

#include "OutputCtx.hpp"
#include <core/common.h>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/node.hxx>

namespace librii::j3d {

Result<void> readINF1(BMDOutputContext& ctx);
Result<void> readVTX1(BMDOutputContext& ctx);
Result<void> readEVP1DRW1(BMDOutputContext& ctx);
Result<void> readJNT1(BMDOutputContext& ctx);
Result<void> readMAT3(BMDOutputContext& ctx);
Result<void> readSHP1(BMDOutputContext& ctx);
Result<void> readTEX1(BMDOutputContext& ctx);

std::unique_ptr<oishii::Node> makeINF1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::Node> makeVTX1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::Node> makeEVP1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::Node> makeDRW1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::Node> makeJNT1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::Node> makeSHP1Node(BMDExportContext& ctx);
std::unique_ptr<oishii::Node> makeMAT3Node(BMDExportContext& ctx);
std::unique_ptr<oishii::Node> makeMDL3Node(BMDExportContext& ctx);
std::unique_ptr<oishii::Node> makeTEX1Node(BMDExportContext& ctx);

} // namespace librii::j3d
