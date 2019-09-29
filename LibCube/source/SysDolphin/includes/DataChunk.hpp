#pragma once

#include "essential_functions.hpp"
#include <oishii/writer/binary_writer.hxx>
#include "CmdStream.hpp"

namespace libcube { namespace pikmin1 {

struct DataChunk
{
	constexpr static const char name[] = "DataChunk";

	std::vector<float> m_data;

	DataChunk() = default;
	~DataChunk() = default;

	static void onRead(oishii::BinaryReader& bReader, DataChunk& context)
	{
		context.m_data.resize(bReader.read<u32>());
		for (auto& data : context.m_data)
			data = bReader.read<f32>();
	}

	static void write(oishii::Writer& bWriter, DataChunk& context)
	{
		bWriter.write<u32>(static_cast<u32>(context.m_data.size()));
		for (auto& data : context.m_data)
			bWriter.write<f32>(data);
	}
};

inline void operator<<(DataChunk& context, oishii::BinaryReader& bReader)
{
	bReader.dispatch<DataChunk, oishii::Direct, false>(context);
}

}

}
