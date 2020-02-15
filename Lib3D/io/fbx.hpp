#pragma once

#include <string>
#include <LibCore/api/Node.hpp>
#include <memory>

namespace lib3d {


inline bool ends_with(const std::string& value, const std::string& ending)
{
	return ending.size() <= value.size() && std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

struct FbxIo : public px::IBinarySerializer
{
	std::unique_ptr<IBinarySerializer> clone() override
	{
		return std::make_unique<FbxIo>(*this);
	}

	bool canRead() override { return true; }
	bool canWrite() override { return false; }

	void read(px::Dynamic& state, oishii::BinaryReader& reader) override;
	void write(px::Dynamic& state, oishii::v2::Writer& writer) override;

	std::string matchFile(const std::string& file, oishii::BinaryReader& reader) override
	{
		return ends_with(file, ".fbx") || ends_with(file, ".dae") || ends_with(file, ".obj") ? "3D::Scene" : "";
	}
	// TODO: Might merge this with canRead/canWrite
	bool matchForWrite(const std::string& id) override { return false; }
};

} // namespace lib3d
