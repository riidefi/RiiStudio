#pragma once

#include <vector>
#include <map>
#include <LibCube/JSystem/J3D/Model.hpp>
#include <oishii/v2/writer/binary_writer.hxx>
#include <oishii/v2/writer/node.hxx>
#include <LibRiiEditor/common.hpp>
namespace libcube::jsystem {

struct BMDOutputContext
{
    J3DModel& mdl;
    oishii::BinaryReader& reader;

    // Compression ID LUT (remap table)
    std::vector<u16> jointIdLut;
    std::vector<u16> materialIdLut;
    std::vector<u16> shapeIdLut;

    
	// Associate section magics with file positions and size
	struct SectionEntry
	{
		std::size_t streamPos;
		u32 size;
	};
	std::map<u32, SectionEntry> mSections;
};

template<typename T, u32 r, u32 c, typename TM, glm::qualifier qM>
inline void transferMatrix(glm::mat<r, c, TM, qM>& mat, T& stream)
{
	for (int i = 0; i < r; ++i)
		for (int j = 0; j < c; ++j)
			stream.transfer(mat[i][j]);
}

inline std::vector<std::string> readNameTable(oishii::BinaryReader& reader)
{
	const auto start = reader.tell();
	std::vector<std::string> collected(reader.read<u16>());
	reader.read<u16>();

	for (auto& e : collected)
	{
		const auto [hash, ofs] = reader.readX<u16, 2>();
		{
			oishii::Jump<oishii::Whence::Set> g(reader, start + ofs);

			for (char c = reader.read<s8>(); c; c = reader.read<s8>())
				e.push_back(c);
		}
	}

	return collected;
}


inline bool enterSection(BMDOutputContext& ctx, u32 id)
{
	const auto sec = ctx.mSections.find(id);
	if (sec == ctx.mSections.end())
		return false;

	ctx.reader.seekSet(sec->second.streamPos - 8);
	return true;
}

struct ScopedSection : private oishii::BinaryReader::ScopedRegion
{
	ScopedSection(oishii::BinaryReader& reader, const char* name)
		: oishii::BinaryReader::ScopedRegion(reader, name)
	{
		start = reader.tell();
		reader.seek(4);
		size = reader.read<u32>();
	}
	u32 start;
	u32 size;
};

template<typename T, bool leaf=false>
struct LinkNode final : public oishii::v2::Node, public T
{

	template<typename... S>
	LinkNode(S... arg)
		: T(arg...), Node(T::getNameId())
	{
		if (leaf) mLinkingRestriction.setFlag(oishii::v2::LinkingRestriction::Leaf);
	}
	oishii::v2::Node::Result write(oishii::v2::Writer& writer) const noexcept override
	{
		T::write(writer);
		return eResult::Success;
	}

	oishii::v2::Node::Result gatherChildren(oishii::v2::Node::NodeDelegate& out) const noexcept override
	{
		T::gatherChildren(out);

		return {};
	}
	const oishii::v2::Node& getSelf() const override
	{
		return *this;
	}
};

struct BMDExportContext
{
	J3DModel& mdl;
};

}
