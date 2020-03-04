#include <core/common.h>
#include <core/kpi/Node.hpp>

#include <oishii/v2/writer/binary_writer.hxx>
#include <oishii/reader/binary_reader.hxx>

#include <plugins/g3d/collection.hpp>

#include <plugins/g3d/util/Dictionary.hpp>

#include <string>

namespace riistudio::g3d {

inline bool ends_with(const std::string& value, const std::string& ending) {
	return ending.size() <= value.size() && std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

static void readModel(G3DModelAccessor& mdl, oishii::BinaryReader& reader) {
	reader.expectMagic<'MDL0', false>();
}

class ArchiveDeserializer {
public:
	std::string canRead(const std::string& file, oishii::BinaryReader& reader) const {
		return ends_with(file, "brres") ? typeid(G3DCollection).name() : "";
    }
	void read(kpi::IDocumentNode& node, oishii::BinaryReader& reader) const {
		assert(dynamic_cast<G3DCollection*>(&node) != nullptr);
		G3DCollectionAccessor collection(&node);

		// Magic
		reader.read<u32>();
		// byte order
		reader.read<u16>();
		// revision
		reader.read<u16>();
		// filesize
		reader.read<u32>();
		// ofs
		reader.read<u16>();
		// section size
		const u16 nSec = reader.read<u16>();

		// 'root'
		reader.read<u32>();
		u32 secLen = reader.read<u32>();
		Dictionary rootDict(reader);

		for (std::size_t i = 1; i < rootDict.mNodes.size(); ++i)
		{
			const auto& cnode = rootDict.mNodes[i];

			reader.seekSet(cnode.mDataDestination);
			Dictionary cdic(reader);

			if (cnode.mName == "3DModels(NW4R)") {
				for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
					const auto& sub = cdic.mNodes[j];

					reader.seekSet(sub.mDataDestination);
					auto&& mdl = collection.addG3DModel();
					readModel(mdl, reader);
				}
			} else {
				printf("Unsupported folder: %s\n", cnode.mName.c_str());
			}
		}
	}

	static void Install(kpi::ApplicationPlugins& installer) {
		installer.addDeserializer<ArchiveDeserializer>();
	}
};

void InstallBRRES(kpi::ApplicationPlugins& installer) {
	ArchiveDeserializer::Install(installer);
}

} // namespace riistudio::g3d
