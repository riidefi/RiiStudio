
#include "MaterialData.hpp"

namespace riistudio::j3d {

using namespace libcube;

enum class MatSec
{
	// Material entries, lut, and name table handled separately
	IndirectTexturingInfo,
	CullModeInfo,
	MaterialColors,
	NumColorChannels,
	ColorChannelInfo,
	AmbientColors,
	LightInfo,
	NumTexGens,
	TexGenInfo,
	PostTexGenInfo,
	TexMatrixInfo,
	PostTexMatrixInfo,
	TextureRemapTable,
	TevOrderInfo,
	TevColors,
	TevKonstColors,
	NumTevStages,
	TevStageInfo,
	TevSwapModeInfo,
	TevSwapModeTableInfo,
	FogInfo,
	AlphaCompareInfo,
	BlendModeInfo,
	ZModeInfo,
	ZCompareInfo,
	DitherInfo,
	NBTScaleInfo,

	Max,
	Min = 0
};

#pragma endregion
struct MatLoader
{
	std::array<s32, static_cast<u32>(MatSec::Max)> mSections;
	u32 start;
	oishii::BinaryReader& reader;

	template<typename TRaw>
	struct SecOfsEntry
	{
		oishii::BinaryReader& reader;
		u32 ofs;

		template<typename TGet = TRaw>
		TGet raw()
		{
			if (!ofs)
				throw "Invalid read.";

			return reader.getAt<TGet>(ofs);
		}
		template<typename T, typename TRaw_>
		T as()
		{
			return static_cast<T>(raw<TRaw_>());
		}
	};
private:
	u32 secOfsStart(MatSec sec) const
	{
		return mSections[static_cast<size_t>(sec)] + start;
	}
public:
	u32 secOfsEntryRaw(MatSec sec, u32 idx, u32 stride) const noexcept
	{
		return secOfsStart(sec) + idx * stride;
	}

	template<typename T>
	SecOfsEntry<T> indexed(MatSec sec)
	{
		return SecOfsEntry<T> { reader, secOfsEntryRaw(sec, reader.read<T>(), sizeof(T)) };
	}

	template<typename T>
	SecOfsEntry<u8> indexed(MatSec sec, u32 stride)
	{
		const auto idx = reader.read<T>();

		if (idx == static_cast<T>(-1))
		{
			// reader.warnAt("Null index", reader.tell() - sizeof(T), reader.tell());
			return SecOfsEntry<u8> { reader, 0 };
		}

		return SecOfsEntry<u8>{ reader, secOfsEntryRaw(sec, idx, stride) };
	}
	SecOfsEntry<u8> indexed(MatSec sec, u32 stride, u32 idx)
	{
		if (idx == -1)
		{
			// reader.warnAt("Null index", reader.tell() - sizeof(T), reader.tell());
			return SecOfsEntry<u8> { reader, 0 };
		}

		return SecOfsEntry<u8>{ reader, secOfsEntryRaw(sec, idx, stride) };
	}

	template<typename TIdx, typename T>
	void indexedContained(T& out, MatSec sec, u32 stride)
	{
		if (const auto ofs = indexed<TIdx>(sec, stride).ofs)
		{
			oishii::Jump<oishii::Whence::Set> g(reader, ofs);

			io_wrapper<T>::onRead(reader, out);
		}
	}
	template<typename T>
	void indexedContained(T& out, MatSec sec, u32 stride, u32 idx)
	{
		if (const auto ofs = indexed(sec, stride, idx).ofs)
		{
			oishii::Jump<oishii::Whence::Set> g(reader, ofs);

			io_wrapper<T>::onRead(reader, out);
		}
	}

	template<typename TIdx, typename T>
	void indexedContainer(T& out, MatSec sec, u32 stride)
	{
		// Big assumption: Entries are contiguous

		oishii::JumpOut g(reader, out.max_size() * sizeof(TIdx));

		for (auto& it : out)
		{
			if (const auto ofs = indexed<TIdx>(sec, stride).ofs)
			{
				oishii::Jump<oishii::Whence::Set> g(reader, ofs);
				// oishii::BinaryReader::ScopedRegion n(reader, io_wrapper<T::value_type>::getName());
				io_wrapper<typename T::value_type>::onRead(reader, it);

				++out.nElements;
			}
			// Assume entries are contiguous
			else break;
		}
	}
};
void readMatEntry(Material& mat, MatLoader& loader, oishii::BinaryReader& reader, u32 ofsStringTable, u32 idx)
{
	oishii::DebugExpectSized dbg(reader, 332);
	oishii::BinaryReader::ScopedRegion g(reader, "Material");

	assert(reader.tell() % 4 == 0);
	mat.flag = reader.read<u8>();
	mat.cullMode = loader.indexed<u8>(MatSec::CullModeInfo).as<gx::CullMode, u32>();
	// TODO:
	mat.cullMode = libcube::gx::CullMode::Back;
	mat.info.nColorChan = loader.indexed<u8>(MatSec::NumTexGens).raw();
	mat.info.nTexGen = loader.indexed<u8>(MatSec::NumTexGens).raw();
	mat.info.nTevStage = loader.indexed<u8>(MatSec::NumTevStages).raw();
	mat.earlyZComparison = loader.indexed<u8>(MatSec::ZCompareInfo).as<bool, u8>();
	loader.indexedContained<u8>(mat.zMode, MatSec::ZModeInfo, 4);
	mat.dither = loader.indexed<u8>(MatSec::DitherInfo).as<bool, u8>();

	dbg.assertSince(8);
	array_vector<gx::Color, 2> matColors;
	loader.indexedContainer<u16>(matColors, MatSec::MaterialColors, 4);
	dbg.assertSince(0xc);

	loader.indexedContainer<u16>(mat.colorChanControls, MatSec::ColorChannelInfo, 8);
	array_vector<gx::Color, 2> ambColors;

	loader.indexedContainer<u16>(ambColors, MatSec::AmbientColors, 4);
	for (int i = 0; i < matColors.size(); ++i)
		mat.chanData.push_back({ matColors[i], ambColors[i] });

	loader.indexedContainer<u16>(mat.lightColors, MatSec::LightInfo, 4);

	dbg.assertSince(0x28);
	loader.indexedContainer<u16>(mat.texGens, MatSec::TexGenInfo, 4);
	const auto post_tg = reader.readX<u16, 8>();
	// TODO: Validate assumptions here

	dbg.assertSince(0x48);
	array_vector<Material::TexMatrix, 10> texMatrices;
	loader.indexedContainer<u16>(texMatrices, MatSec::TexMatrixInfo, 100);
	loader.indexedContainer<u16>(mat.postTexMatrices, MatSec::PostTexMatrixInfo, 100);

	mat.texMatrices.nElements = texMatrices.size();
	for (int i = 0; i < mat.texMatrices.nElements; ++i)
		mat.texMatrices[i] = std::make_unique<Material::TexMatrix>(texMatrices[i]);
	dbg.assertSince(0x84);

	array_vector<Material::J3DSamplerData, 8> samplers;
	loader.indexedContainer<u16>(samplers, MatSec::TextureRemapTable, 2);
	mat.samplers.nElements = samplers.size();
	for (int i = 0; i < samplers.size(); ++i)
		mat.samplers[i] = std::make_unique<Material::J3DSamplerData>(samplers[i]);

	{
		dbg.assertSince(0x094);
		loader.indexedContainer<u16>(mat.tevKonstColors, MatSec::TevKonstColors, 4);
		dbg.assertSince(0x09C);

		const auto kc_sels = reader.readX<u8, 16>();
		const auto ka_sels = reader.readX<u8, 16>();

		const auto get_kc = [&](size_t i) { return static_cast<gx::TevKColorSel>(kc_sels[i]); };
		const auto get_ka = [&](size_t i) { return static_cast<gx::TevKAlphaSel>(ka_sels[i]); };

		dbg.assertSince(0x0BC);
		array_vector<TevOrder, 16> tevOrderInfos;
		loader.indexedContainer<u16>(tevOrderInfos, MatSec::TevOrderInfo, 4);


		loader.indexedContainer<u16>(mat.tevColors, MatSec::TevColors, 8);

		// FIXME: Directly read into material
		array_vector<gx::TevStage, 16> tevStageInfos;
		loader.indexedContainer<u16>(tevStageInfos, MatSec::TevStageInfo, 20);

		mat.shader.mStages.resize(tevStageInfos.size());
		for (int i = 0; i < tevStageInfos.size(); ++i)
		{
			mat.shader.mStages[i] = tevStageInfos[i];
			mat.shader.mStages[i].texCoord = tevOrderInfos[i].texCoord;
			mat.shader.mStages[i].texMap = tevOrderInfos[i].texMap;
			mat.shader.mStages[i].rasOrder = tevOrderInfos[i].rasOrder;
			mat.shader.mStages[i].colorStage.constantSelection = get_kc(i);
			mat.shader.mStages[i].alphaStage.constantSelection = get_ka(i);
		}

		dbg.assertSince(0x104);
		array_vector<SwapSel, 16> swapSels;
		loader.indexedContainer<u16>(swapSels, MatSec::TevSwapModeInfo, 4);
		for (int i = 0; i < mat.shader.mStages.size(); ++i)
		{
			mat.shader.mStages[i].texMapSwap = swapSels[i].texSel;
			mat.shader.mStages[i].rasSwap = swapSels[i].colorChanSel;
		}

		// TODO: We can't use a std::array as indexedContainer relies on nEntries
		array_vector<gx::SwapTableEntry, 4> swap;
		loader.indexedContainer<u16>(swap, MatSec::TevSwapModeTableInfo, 4);
		for (int i = 0; i < 4; ++i)
			mat.shader.mSwapTable[i] = swap[i];

		for (auto& e : mat.stackTrash)
			e = reader.read<u8>();
	}
	dbg.assertSince(0x144);
	loader.indexedContained<u16>(mat.fogInfo, MatSec::FogInfo, 44);
	dbg.assertSince(0x146);
	loader.indexedContained<u16>(mat.alphaCompare, MatSec::AlphaCompareInfo, 8);
	dbg.assertSince(0x148);
	loader.indexedContained<u16>(mat.blendMode, MatSec::BlendModeInfo, 4);
	dbg.assertSince(0x14a);
	loader.indexedContained<u16>(mat.nbtScale, MatSec::NBTScaleInfo, 16);

	if (loader.mSections[(u32)MatSec::IndirectTexturingInfo] && loader.mSections[(u32)MatSec::IndirectTexturingInfo] != ofsStringTable)
	{
		Model::Indirect ind{};
		loader.indexedContained<Model::Indirect>(ind, MatSec::IndirectTexturingInfo, 0x138, idx);

		mat.indEnabled = ind.enabled;
		mat.info.nIndStage = ind.nIndStage;

		for (int i = 0; i < ind.nIndStage; ++i)
			mat.mIndScales.emplace_back(ind.texScale[i]);
		// TODO: Assumes one indmtx / indstage
		for (int i = 0; i < 3 && i < ind.nIndStage; ++i)
			mat.mIndMatrices.emplace_back(ind.texMtx[i]);
		for (int i = 0; i < 4; ++i)
			mat.shader.mIndirectOrders[i] = ind.tevOrder[i];
		for (int i = 0; i < mat.shader.mStages.size(); ++i)
			mat.shader.mStages[i].indirectStage = ind.tevStage[i];
	}
}

template<typename T>
void operator<<(T& lhs, oishii::BinaryReader& rhs) {
	io_wrapper<T>::onRead(rhs, lhs);
}

void readMAT3(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
	if (!enterSection(ctx, 'MAT3')) return;
	
	ScopedSection g(reader, "Materials");

	u16 size = reader.read<u16>();

	for (u32 i = 0; i < size; ++i)
		ctx.mdl.addMaterial();
	ctx.materialIdLut.resize(size);
	reader.read<u16>();

	const auto [ofsMatData, ofsRemapTable, ofsStringTable] = reader.readX<s32, 3>();
	MatLoader loader{ reader.readX<s32, static_cast<u32>(MatSec::Max)>(), g.start, reader };

	{
		// Read cache
		auto& cache = ctx.mdl.get().mMatCache;

		for (int i = 0; i < (int)MatSec::Max; ++i) {
			const auto begin = loader.mSections[i];
			if (!begin) continue;
			// NBT scale hack..
			auto end = 0;
			if (i + 1 == (int)MatSec::Max) {
				end = -1;
			}
			else {
				bool found = false;
				for (int j = i+1; !found && j < (int)MatSec::Max; ++j) {
					if (loader.mSections[j]) {
						end = loader.mSections[j];
						found = true;
					}
				}
				assert(found);
			}
			assert(i + 1 == (int)MatSec::Max || end == loader.mSections[i + 1] || loader.mSections[i + 1] == 0);
			auto size = begin > 0 ? end - begin : 0;
			if (end == -1) size = 16; // NBT hack
			if (size <= 0) continue;

			auto readCacheEntry = [&](auto& out, std::size_t entry_size) {
				const auto nInferred = size / entry_size;
				out.resize(nInferred);

				auto pad_check = [](oishii::BinaryReader& reader, std::size_t search_size) {
					std::string_view pstring = "This is padding data to align";

					for (int i = 0; i < std::min(search_size, pstring.length()); ++i) {
						if (reader.read<u8>() != pstring[i]) return false;
					}
					return true;
				};

				std::size_t _it = 0;
				for (auto& e : out) {
					reader.seekSet(begin + g.start + _it * entry_size);
					if (pad_check(reader, entry_size)) {
						break;
					}
					reader.seekSet(begin + g.start + _it * entry_size);
					e << reader;
					++_it;
				}
				out.resize(_it);
			};

			switch ((MatSec)i) {
			case MatSec::IndirectTexturingInfo:
				readCacheEntry(cache.indirectInfos, 312);
				break;
			case MatSec::CullModeInfo:
				readCacheEntry(cache.cullModes, 4);
				break;
			case MatSec::MaterialColors:
				readCacheEntry(cache.matColors, 4);
				break;
			case MatSec::NumColorChannels:
				readCacheEntry(cache.nColorChan, 1);
				break;
			case MatSec::ColorChannelInfo:
				readCacheEntry(cache.colorChans, 8);
				break;
			case MatSec::AmbientColors:
				readCacheEntry(cache.ambColors, 4);
				break;
			case MatSec::LightInfo:
				readCacheEntry(cache.lightColors, 4);
				break;
			case MatSec::NumTexGens:
				readCacheEntry(cache.nTexGens, 1);
				break;
			case MatSec::TexGenInfo:
				readCacheEntry(cache.texGens, 4);
				break;
			case MatSec::PostTexGenInfo:
				readCacheEntry(cache.posTexGens, 4);
				break;
			case MatSec::TexMatrixInfo:
				readCacheEntry(cache.texMatrices, 100);
				break;
			case MatSec::PostTexMatrixInfo:
				readCacheEntry(cache.postTexMatrices, 100);
				break;
			case MatSec::TextureRemapTable:
				readCacheEntry(cache.samplers, 2);
				break;
			case MatSec::TevOrderInfo:
				readCacheEntry(cache.orders, 4);
				break;
			case MatSec::TevColors:
				readCacheEntry(cache.tevColors, 8);
				break;
			case MatSec::TevKonstColors:
				readCacheEntry(cache.konstColors, 4);
				break;
			case MatSec::NumTevStages:
				readCacheEntry(cache.nTevStages, 1);
				break;
			case MatSec::TevStageInfo:
				readCacheEntry(cache.tevStages, 20);
				break;
			case MatSec::TevSwapModeInfo:
				readCacheEntry(cache.swapModes, 4);
				break;
			case MatSec::TevSwapModeTableInfo:
				readCacheEntry(cache.swapTables, 4);
				break;
			case MatSec::FogInfo:
				readCacheEntry(cache.fogs, 44);
				break;
			case MatSec::AlphaCompareInfo:
				readCacheEntry(cache.alphaComparisons, 8);
				break;
			case MatSec::BlendModeInfo:
				readCacheEntry(cache.blendModes, 4);
				break;
			case MatSec::ZModeInfo:
				readCacheEntry(cache.zModes, 4);
				break;
			case MatSec::ZCompareInfo:
				readCacheEntry(cache.zCompLocs, 1);
				break;
			case MatSec::DitherInfo:
				readCacheEntry(cache.dithers, 1);
				break;
			case MatSec::NBTScaleInfo:
				readCacheEntry(cache.nbtScales, 16);
				break;
			}
		}

	}

	reader.seekSet(g.start);

	// FIXME: Generalize this code
	reader.seekSet(g.start + ofsRemapTable);

	bool sorted = true;
	for (int i = 0; i < size; ++i)
	{
		ctx.materialIdLut[i] = reader.read<u16>();

		if (ctx.materialIdLut[i] != i)
			sorted = false;
	}

	if (!sorted)
		DebugReport("Material IDS will be remapped on save and incompatible with animations.\n");

	reader.seekSet(ofsStringTable + g.start);
	const auto nameTable = readNameTable(reader);

	for (int i = 0; i < size; ++i)
	{
		Material& mat = ctx.mdl.getMaterial(i).get();
		reader.seekSet(g.start + ofsMatData + ctx.materialIdLut[i] * 0x14c);
		mat.id = ctx.materialIdLut[i];
		mat.name = nameTable[i];

		readMatEntry(mat, loader, reader, ofsStringTable, i);
	}
}
template<typename T, u32 bodyAlign = 1, u32 entryAlign = 1, bool compress = true>
class MCompressableVector : public oishii::v2::Node
{
	struct Child : public oishii::v2::Node
	{
		Child(const MCompressableVector& parent, u32 index)
			: mParent(parent), mIndex(index)
		{
			mId = std::to_string(index);
			getLinkingRestriction().alignment = entryAlign;
			//getLinkingRestriction().setFlag(oishii::v2::LinkingRestriction::PadEnd);
			getLinkingRestriction().setLeaf();
		}
		Result write(oishii::v2::Writer& writer) const noexcept override
		{
			io_wrapper<T>::onWrite(writer, mParent.getEntry(mIndex));
			return {};
		}
		const MCompressableVector& mParent;
		const u32 mIndex;
	};
	struct ContainerNode : public oishii::v2::Node
	{
		ContainerNode(const MCompressableVector& parent, const std::string& id)
			: mParent(parent)
		{
			mId = id;
			getLinkingRestriction().alignment = bodyAlign;
		}
		Result gatherChildren(NodeDelegate& d) const noexcept override
		{
			for (u32 i = 0; i < mParent.getNumEntries(); ++i)
				d.addNode(std::make_unique<Child>(mParent, i));
			return {};
		}
		const MCompressableVector& mParent;
	};
public:

	std::unique_ptr<oishii::v2::Node> spawnNode(const std::string& id) const
	{
		return std::make_unique<ContainerNode>(*this, id);
	}

	u32 append(const T& entry)
	{
		const auto found = std::find(mEntries.begin(), mEntries.end(), entry);
		if (found == mEntries.end() || !compress)
		{
			mEntries.push_back(entry);
			return mEntries.size() - 1;
		}
		return found - mEntries.begin();
	}
	int find(const T& entry) const
	{
		int outIdx = -1;
		for (int i = 0; i < mEntries.size(); ++i)
			if (entry == mEntries[i])
			{
				outIdx = i;
				break;
				
			}
		// Intentionally complete
		// TODO: rfind
		return outIdx;

	}
	u32 getNumEntries() const
	{
		return mEntries.size();
	}
	const T& getEntry(u32 idx) const
	{
		assert(idx < mEntries.size());
		return mEntries[idx];
	}
	T& getEntry(u32 idx)
	{
		assert(idx < mEntries.size());
		return mEntries[idx];
	}

public:
	std::vector<T> mEntries;
};
struct MAT3Node;
struct SerializableMaterial
{
	SerializableMaterial(const MAT3Node& mat3, int idx)
		: mMAT3(mat3), mIdx(idx)
	{}

	const MAT3Node& mMAT3;
	const int mIdx;

	bool operator==(const SerializableMaterial& rhs) const noexcept;
};
auto find = [](const auto& buf, const auto x) {
	auto found = std::find(buf.begin(), buf.end(), x);
	// if (found == buf.end()) {
	// 	buf.push_back(x);
	// }
	assert(found != buf.end());
	if (found == buf.end()) {
		printf("Invalid data entry not cached.\n");
	}
	return found == buf.end() ? -1 : found - buf.begin();
};
template<typename TIdx, typename T, typename TPool>
void write_array_vec(oishii::v2::Writer& writer, const T& vec, TPool& pool)
{
	for (int i = 0; i < vec.nElements; ++i)
		writer.write<TIdx>(find(pool, vec[i]));
	for (int i = vec.nElements; i < vec.max_size(); ++i)
		writer.write<TIdx>(-1);
}
template<typename T>
int write_cache(oishii::v2::Writer& writer, const std::vector<T>& cache) {
	// while (writer.tell() % io_wrapper<T>::SizeOf) writer.write(0xff);
	const auto start = writer.tell();
	for (auto& x : cache) {
		io_wrapper<T>::onWrite(writer, x);
	}
	return start;
}

template<>
struct io_wrapper<SerializableMaterial>
{
	static void onWrite(oishii::v2::Writer& writer, const SerializableMaterial& smat);
};
struct MAT3Node : public oishii::v2::Node
{
	template<typename T, MatSec s>
	struct Section : MCompressableVector<T, 4, 0, true>
	{
	};

	struct EntrySection final : public Section<SerializableMaterial, MatSec::Max>
	{
	

		EntrySection(const ModelAccessor mdl, const MAT3Node& mat3)
			: mMdl(mdl)
		{
			for (int i = 0; i < mMdl.getMaterials().size(); ++i)
				mLut.push_back(append(SerializableMaterial{ mat3, i }));
		}

		const ModelAccessor mMdl;
		std::vector<u16> mLut;
	};


	const ModelAccessor mMdl;
	bool hasIndirect = false;
	EntrySection mEntries;
	const Model::MatCache& mCache;

	gx::TexCoordGen postTexGen(const gx::TexCoordGen& gen) const noexcept
	{
		return gx::TexCoordGen{ // gen.id,
			gen.func, gen.sourceParam, static_cast<gx::TexMatrix>(gen.postMatrix),
					false, gen.postMatrix };
	}

	MAT3Node(const BMDExportContext& ctx)
		: mMdl(ctx.mdl), mEntries(ctx.mdl, *this), mCache(ctx.mdl.get().mMatCache)
	{
		mId = "MAT3";
		getLinkingRestriction().alignment = 32;

	}

	Result write(oishii::v2::Writer& writer) const noexcept override
	{
		const auto start = writer.tell();
		writer.write<u32, oishii::EndianSelect::Big>('MAT3');
		writer.writeLink<s32>({ *this }, { *this, oishii::v2::Hook::EndOfChildren });

		writer.write<u16>(mMdl.getMaterials().size());
		writer.write<u16>(-1);

		auto writeSecLinkS = [&](const std::string& lnk)
		{
			writer.writeLink<s32>({ *this }, { lnk });
		};

		const auto ofsEntries = writer.tell();
		writer.write<s32>(-1);
		const auto ofsLut = writer.tell();
		writer.write<s32>(-1);
		const auto ofsNameTab = writer.tell();
		writer.write<s32>(-1);
		auto tableCursor = writer.tell();
		// For now, write padding
		for (int i = 3; i <= 29; ++i)
			writer.write<s32>(-1);

	
		auto entryStart = writer.tell();
		for (auto& entry : mEntries.mEntries) {
			io_wrapper<SerializableMaterial>::onWrite(writer, entry);
		}

		{
			oishii::Jump<oishii::Whence::At, oishii::v2::Writer> g(writer, ofsEntries);
			writer.write<s32>(entryStart - start);
		}


		{

			while (writer.tell() % 2) writer.write<u8>(0);
			auto lutStart = writer.tell();

			for (const auto e : mEntries.mLut)
				writer.write<u16>(e);

			oishii::Jump<oishii::Whence::At, oishii::v2::Writer> g(writer, ofsLut);
			writer.write<s32>(lutStart - start);
		}
		{
			while (writer.tell() % 4) writer.write<u8>(0);

			auto nameTabStart = writer.tell();

			std::vector<std::string> names(mMdl.getMaterials().size());
			int i = 0;
			for (int i = 0; i < mMdl.getMaterials().size(); ++i)
				names[i] = mMdl.getMaterial(i).get().name;
			writeNameTable(writer, names);
			writer.alignTo(4);

			oishii::Jump<oishii::Whence::At, oishii::v2::Writer> g(writer, ofsNameTab);
			writer.write<s32>(nameTabStart - start);
		}

		auto dataPos = writer.tell();
		auto writeBuf = [&](const auto& buf) {
			int slide = 0;

			if (!buf.empty()) {
				while (writer.tell() % 4) writer.write<u8>(0);
				const int ofs = write_cache(writer, buf);
				slide = ofs - start;
			}

			oishii::Jump<oishii::Whence::Set, oishii::v2::Writer> g(writer, tableCursor);
			writer.write<s32>(slide);
			
			tableCursor += 4;
		};

		// 3 indirect
		writeBuf(mCache.indirectInfos);
		// mCullModes, 4
		writeBuf(mCache.cullModes);
		// mMaterialColors, 5
		writeBuf(mCache.matColors);
		// mNumColorChannels, 6
		writeBuf(mCache.nColorChan);
		// mChannelControls, 7
		writeBuf(mCache.colorChans);
		// mAmbColors, 8
		writeBuf(mCache.ambColors);
		// mLightColors, 9
		writeBuf(mCache.lightColors);
		// mNumTexGens, 10
		writeBuf(mCache.nTexGens);
		// mTexGens, 11
		writeBuf(mCache.texGens);
		// mPostTexGens, 12
		writeBuf(mCache.posTexGens);
		// mTexMatrices, 13
		writeBuf(mCache.texMatrices);
		// mPostTexMatrices, 14
		writeBuf(mCache.postTexMatrices);
		// mTextureTable, 15
		writeBuf(mCache.samplers);
		// mOrders, 16
		writeBuf(mCache.orders);
		// mTevColors, 17
		writeBuf(mCache.tevColors);
		// mKonstColors, 18
		writeBuf(mCache.konstColors);
		// mNumTevStages, 19
		writeBuf(mCache.nTevStages);
		// mTevStages, 20
		writeBuf(mCache.tevStages);
		// mSwapModes, 21
		writeBuf(mCache.swapModes);
		// mSwapTables, 22
		writeBuf(mCache.swapTables);
		// mFogs, 23
		writeBuf(mCache.fogs);
		// mAlphaComparisons, 24
		writeBuf(mCache.alphaComparisons);
		// mBlendModes, 25
		writeBuf(mCache.blendModes);
		// mZModes, 26
		writeBuf(mCache.zModes);
		// mZCompLocs, 27
		writeBuf(mCache.zCompLocs);
		// mDithers, 28
		writeBuf(mCache.dithers);
		// mNBTScales, 29
		writeBuf(mCache.nbtScales);

		

		return {};
	}

	Result gatherChildren(NodeDelegate& d) const noexcept override
	{
		return {};
	}
};
bool SerializableMaterial::operator==(const SerializableMaterial& rhs) const noexcept
{
	return mMAT3.mMdl.getMaterial(mIdx).get() == rhs.mMAT3.mMdl.getMaterial(rhs.mIdx).get();
}
void io_wrapper<SerializableMaterial>::onWrite(oishii::v2::Writer& writer, const SerializableMaterial& smat)
{
	const Material& m = smat.mMAT3.mMdl.getMaterial(smat.mIdx).get();
		
	oishii::DebugExpectSized dbg(writer, 332);

	

	writer.write<u8>(m.flag);
	writer.write<u8>(find(smat.mMAT3.mCache.cullModes, m.cullMode));
	writer.write<u8>(find(smat.mMAT3.mCache.nColorChan, m.info.nColorChan));
	writer.write<u8>(find(smat.mMAT3.mCache.nTexGens, m.info.nTexGen));
	writer.write<u8>(find(smat.mMAT3.mCache.nTevStages, m.info.nTevStage));
	writer.write<u8>(find(smat.mMAT3.mCache.zCompLocs, m.earlyZComparison));
	writer.write<u8>(find(smat.mMAT3.mCache.zModes, m.zMode));
	writer.write<u8>(find(smat.mMAT3.mCache.dithers, m.dither));

	dbg.assertSince(0x008);
	const auto& m3 = smat.mMAT3;
	const auto& cache = m3.mCache;

	array_vector<gx::Color, 2> matColors, ambColors;
	for (int i = 0; i < m.chanData.size(); ++i)
	{
		matColors.push_back(m.chanData[i].matColor);
		ambColors.push_back(m.chanData[i].ambColor);
	}
	if (m.chanData.size() == 1) {
		matColors.push_back(m.chanData[0].matColor);
		ambColors.push_back(m.chanData[0].ambColor);
	}
	write_array_vec<u16>(writer, matColors, cache.matColors);
	write_array_vec<u16>(writer, m.colorChanControls, cache.colorChans);
	write_array_vec<u16>(writer, ambColors, cache.ambColors);
	write_array_vec<u16>(writer, m.lightColors, cache.lightColors);
		
	for (int i = 0; i < m.texGens.size(); ++i)
		writer.write<u16>(find(cache.texGens, m.texGens[i]));
	for (int i = m.texGens.size(); i < m.texGens.max_size(); ++i)
		writer.write<u16>(-1);

	for (int i = 0; i < m.texGens.size(); ++i)
		writer.write<u16>(m.texGens[i].postMatrix == gx::PostTexMatrix::Identity ?
			-1 : find(cache.posTexGens, m3.postTexGen(m.texGens[i])));
	for (int i = m.texGens.size(); i < m.texGens.max_size(); ++i)
		writer.write<u16>(-1);

	dbg.assertSince(0x048);
	array_vector<Material::TexMatrix, 10> texMatrices;
	for (int i = 0; i < m.texMatrices.size(); ++i) {
		texMatrices.push_back(*m.texMatrices[i].get());

		assert(texMatrices[i] == texMatrices[i]);
	}
	write_array_vec<u16>(writer, texMatrices, cache.texMatrices);
	
	write_array_vec<u16>(writer, m.postTexMatrices, cache.postTexMatrices);
	//	write_array_vec<u16>(writer, texMatrices, m3.mTexMatrices);
	//	for (int i = 0; i < 10; ++i)
	//		writer.write<s16>(-1);

	dbg.assertSince(0x084);
	array_vector<Material::J3DSamplerData, 8> samplers;
	samplers.nElements = m.samplers.size();
	for (int i = 0; i < m.samplers.size(); ++i)
		samplers[i] = (Material::J3DSamplerData&)*m.samplers[i].get();
	dbg.assertSince(0x084);
	write_array_vec<u16>(writer, samplers, cache.samplers);
	dbg.assertSince(0x094);
	assert(m.tevKonstColors.nElements == 4);
	write_array_vec<u16>(writer, m.tevKonstColors, cache.konstColors);

	dbg.assertSince(0x09C);
	// TODO -- comparison logic might need to account for ksels being here
	for (int i = 0; i < m.shader.mStages.size(); ++i)
		writer.write<u8>(static_cast<u8>(m.shader.mStages[i].colorStage.constantSelection));
	for (int i = m.shader.mStages.size(); i < 16; ++i)
		writer.write<u8>(0xc); // Default

	dbg.assertSince(0x0AC);
	for (int i = 0; i < m.shader.mStages.size(); ++i)
		writer.write<u8>(static_cast<u8>(m.shader.mStages[i].alphaStage.constantSelection));
	for (int i = m.shader.mStages.size(); i < 16; ++i)
		writer.write<u8>(0x1c); // Default

	dbg.assertSince(0x0bc);
	for (int i = 0; i < m.shader.mStages.size(); ++i)
		writer.write<u16>(find(cache.orders, TevOrder{ m.shader.mStages[i].rasOrder, m.shader.mStages[i].texMap, m.shader.mStages[i].texCoord }));
	for (int i = m.shader.mStages.size(); i < 16; ++i)
		writer.write<u16>(-1);

	dbg.assertSince(0x0dc);
	write_array_vec<u16>(writer, m.tevColors, cache.tevColors);

	dbg.assertSince(0x0e4);
	for (int i = 0; i < m.shader.mStages.size(); ++i) {
		libcube::gx::TevStage tmp;
		tmp.colorStage = m.shader.mStages[i].colorStage;
		tmp.colorStage.constantSelection = gx::TevKColorSel::k0;
		tmp.alphaStage = m.shader.mStages[i].alphaStage;
		tmp.alphaStage.constantSelection = gx::TevKAlphaSel::k0_a;
		writer.write<u16>(find(cache.tevStages, tmp));
	}
	for (int i = m.shader.mStages.size(); i < 16; ++i)
		writer.write<u16>(-1);

	dbg.assertSince(0x104);
	for (int i = 0; i < m.shader.mStages.size(); ++i)
		writer.write<u16>(find(cache.swapModes, SwapSel{ m.shader.mStages[i].rasSwap, m.shader.mStages[i].texMapSwap }));
	for (int i = m.shader.mStages.size(); i < 16; ++i)
		writer.write<u16>(-1);

	dbg.assertSince(0x124);
	for (const auto& table : m.shader.mSwapTable)
		writer.write<u16>(find(cache.swapTables, table));

	for (const u8 s : m.stackTrash)
		writer.write<u8>(s);

	dbg.assertSince(0x144);
	writer.write<u16>(find(cache.fogs, m.fogInfo));
	writer.write<u16>(find(cache.alphaComparisons , m.alphaCompare));
	writer.write<u16>(find(cache.blendModes, m.blendMode));
	writer.write<u16>(find(cache.nbtScales, m.nbtScale));
}
std::unique_ptr<oishii::v2::Node> makeMAT3Node(BMDExportContext& ctx)
{
	return std::make_unique<MAT3Node>(ctx);
}

}
