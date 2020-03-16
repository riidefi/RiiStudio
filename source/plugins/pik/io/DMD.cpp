#include "DMD.hpp"

#include <core/common.h>
#include <core/kpi/Node.hpp>

#include <oishii/v2/writer/binary_writer.hxx>
#include <oishii/reader/binary_reader.hxx>

#include <string>

#include <plugins/pik/Scene.hpp>

#include <plugins/pik/util/Parser.hpp>

#include <core/util/crc.hpp>

namespace riistudio::pik {

inline bool ends_with(const std::string& value, const std::string& ending) {
	return ending.size() <= value.size() && std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


class DMD {
public:
	std::string canRead(const std::string& file, oishii::BinaryReader& reader) const {
		return ends_with(file, "dmd") || ends_with(file, "mod") ? typeid(PikminCollection).name() : "";
	}
	bool canWrite(kpi::IDocumentNode& node) const {
		return dynamic_cast<PikminCollection*>(&node) != nullptr;
	}

	// Read a DMD file
	void readModelAscii(PikminModelAccessor model, Parser& parser) const {
		while (!parser.atEnd()) {
			const auto block_name = parser.readToken();
			switch (crc32(block_name)) {
			case crc32("<INFORMATION>"):
				assert(parser.getToken() == "{");
				parser.readToken();
				while (parser.getToken() != "}") {
					switch (crc32(parser.readToken())) {
					case crc32("version"):
						break;
					case crc32("filename"):
						break;
					case crc32("toolname"):
						break;
					case crc32("source"):
						break;
						// date, time, host, magnify, suitable_joint
					case crc32("numjoints"):
						model.get().nJoints = std::stoi(parser.readToken());
						break;
						// scalingrule, primitive, embossbump, compress_mat, vtxclranm
					default:
						break;
					}
				}
				break;
			default:
				return;
			}
		}
	}
	// Read a MOD file
	void readModelBinary(PikminModelAccessor model, oishii::BinaryReader& reader) const {
		model.get().nJoints = 3;
	}

	// Write MOD/DMD file
	void write(kpi::IDocumentNode& node, oishii::v2::Writer& writer) const {

	}

	void read(kpi::IDocumentNode& node, oishii::BinaryReader& reader) const {
		assert(dynamic_cast<PikminCollection*>(&node) != nullptr);
		PikminCollectionAccessor collection(&node);

		if (ends_with(reader.getFile(), "dmd")) {
			// Awful code but not a real issue...
			std::string data;
			data.resize(reader.endpos());
			memcpy(data.data(), reader.getStreamStart(), data.size());
			std::stringstream stream{ data };
			Parser parser{ stream };

			readModelAscii(collection.addPikminModel(), parser);
		} else if (ends_with(reader.getFile(), "mod")) {
			readModelBinary(collection.addPikminModel(), reader);
		}
	}
};

void InstallDMD(kpi::ApplicationPlugins& installer) {
	installer.addDeserializer<DMD>();
	installer.addSerializer<DMD>();
}

} // namespace riistudio::pik
