/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#include <LibCore/common.h>
#include <LibJ//J3D/Collection.hpp>
#include <LibJ/J3D/Binary/BMD/OutputCtx.hpp>

namespace libcube::jsystem {


class BmdIo : public px::IBinarySerializer
{
	std::unique_ptr<IBinarySerializer> clone() override { return std::make_unique<BmdIo>(*this); }

	bool canRead() override { return true; }
	bool canWrite() override { return true; }

	void read(px::Dynamic& state, oishii::BinaryReader& reader) override;
	void write(px::Dynamic& state, oishii::v2::Writer& writer) override;

	std::string matchFile(const std::string& file, oishii::BinaryReader& reader) override
	{
		u32 j3dMagic = reader.read<u32, oishii::EndianSelect::Big>();
		u32 bmdMagic = reader.read<u32, oishii::EndianSelect::Big>();
		reader.seek<oishii::Whence::Current>(-8);
		return j3dMagic == 'J3D2' && (bmdMagic == 'bmd3' || bmdMagic == 'bdl4') ? std::string(J3DCollection::TypeInfo.namespacedId) : "";
	}
	void lex(BMDOutputContext& ctx, u32 sec_count) noexcept;
	void readBMD(oishii::BinaryReader& reader, BMDOutputContext& ctx);


	bool matchForWrite(const std::string& id) override
	{
		return id == J3DCollection::TypeInfo.namespacedId;
	}
};



} // namespace libcube::jsystem
