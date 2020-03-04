#include <core/common.h>
#include <core/kpi/Node.hpp>

#include <oishii/v2/writer/binary_writer.hxx>
#include <oishii/reader/binary_reader.hxx>

#include <plugins/g3d/collection.hpp>

#include <plugins/g3d/util/Dictionary.hpp>
#include <plugins/g3d/util/NameTable.hpp>

#include <string>

namespace riistudio::g3d {

inline bool ends_with(const std::string& value, const std::string& ending) {
	return ending.size() <= value.size() && std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

static void readModel(G3DModelAccessor& mdl, oishii::BinaryReader& reader) {
	const auto start = reader.tell();

	reader.expectMagic<'MDL0', false>();

	const u32 fileSize = reader.read<u32>();
	(void)fileSize;
	const u32 revision = reader.read<u32>();
	assert(revision == 11);
	if (revision != 11) return;

	reader.read<s32>(); // ofsBRRES

	union {
		struct {
			s32 ofsRenderTree;
			s32 ofsBones;
			struct {
				s32 position;
				s32 normal;
				s32 color;
				s32 uv;
				s32 furVec;
				s32 furPos;
			} ofsBuffers;
			s32 ofsMaterials;
			s32 ofsShaders;
			s32 ofsMeshes;
			s32 ofsTextureLinks;
			s32 ofsPaletteLinks;
			s32 ofsUserData;
		} secOfs;
		std::array<s32, 14> secOfsArr;
	};
	for (auto& ofs : secOfsArr)
		ofs = reader.read<s32>();

	mdl.node().setName(readName(reader, start));

	const auto infoPos = reader.tell();
	reader.seek(8); // Ignore size, ofsMode
	mdl.get().mScalingRule = static_cast<ScalingRule>(reader.read<u32>());
	mdl.get().mTexMtxMode = static_cast<TextureMatrixMode>(reader.read<u32>());

	const auto [nVtx, nTri] = reader.readX<u32, 2>();
	mdl.get().sourceLocation = readName(reader, infoPos);
	const auto nViewMtx = reader.read<u32>();

	const auto [bMtxArray, bTexMtxArray, bBoundVolume] = reader.readX<u8, 3>();
	mdl.get().mEvpMtxMode = static_cast<EnvelopeMatrixMode>(reader.read<u8>());

	const s32 ofsBoneTable = reader.read<s32>();

	mdl.get().aabb.min.x = reader.read<f32>();
	mdl.get().aabb.min.y = reader.read<f32>();
	mdl.get().aabb.min.z = reader.read<f32>();
	mdl.get().aabb.max.x = reader.read<f32>();
	mdl.get().aabb.max.y = reader.read<f32>();
	mdl.get().aabb.max.z = reader.read<f32>();
	
}
static void readTexture(kpi::NodeAccessor<Texture>& tex, oishii::BinaryReader& reader) {
	const auto start = reader.tell();

	auto& data = tex.get();

	reader.expectMagic<'TEX0', false>();
	reader.read<u32>(); // size
	const u32 revision = reader.read<u32>();
	assert(revision == 1 || revision == 3);
	reader.read<s32>(); // BRRES offset
	const s32 ofsTex = reader.read<s32>();
	data.name = readName(reader, start);
	const u32 flag = reader.read<u32>(); // TODO: Paletted textures
	data.dimensions.width = reader.read<u16>();
	data.dimensions.height = reader.read<u16>();
	data.format = reader.read<u32>();
	data.mipLevel = reader.read<u32>();
	data.minLod = reader.read<f32>();
	data.maxLod = reader.read<f32>();
	data.sourcePath = readName(reader, start);
	// Skip user data
	reader.seekSet(start + ofsTex);
	data.resizeData();
	assert(reader.tell() + data.getEncodedSize(true) < reader.endpos());
	memcpy(data.data.data(), reader.getStreamStart() + reader.tell(), data.getEncodedSize(true));
	reader.seek(data.getEncodedSize(true));
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

			// TODO
			if (cnode.mName == "3DModels(NW4R)") {
				for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
					const auto& sub = cdic.mNodes[j];

					reader.seekSet(sub.mDataDestination);
					auto&& mdl = collection.addG3DModel();
					readModel(mdl, reader);
				}
			} else if (cnode.mName == "Textures(NW4R)") {
				for (std::size_t j = 1; j < cdic.mNodes.size(); ++j) {
					const auto& sub = cdic.mNodes[j];

					reader.seekSet(sub.mDataDestination);
					auto&& tex = collection.addTexture();
					readTexture(tex, reader);
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
