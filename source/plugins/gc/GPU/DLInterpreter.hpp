#pragma once

#include <core/common.h>

#include "GPUCommand.hpp"
#include "GPUAddressSpace.hpp"

#include <oishii/reader/binary_reader.hxx>

namespace libcube::gpu {

struct QBPCommand {
    BPAddress reg;
    u32 val;
};

// TODO: Not variable-length
struct QXFCommand {
	u16 reg;
	u32 val;
};
struct QCPCommand
{
	u8 reg;
	u32 val;
};

class QDisplayListHandler
{
public:
	virtual ~QDisplayListHandler() {}
	virtual void onCommandBP(const QBPCommand& token) {}
	virtual void onCommandCP(const QCPCommand& token) {}
	virtual void onCommandXF(const QXFCommand& token) {}

	virtual void onStreamBegin() {}
	virtual void onStreamEnd() {}
};

void RunDisplayList(oishii::BinaryReader& reader, QDisplayListHandler& handler, u32 dlSize);


} // namespace libcube::gpu
