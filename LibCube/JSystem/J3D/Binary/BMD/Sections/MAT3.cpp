#include <LibCube/JSystem/J3D/Binary/BMD/Sections.hpp>

namespace libcube::jsystem {

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

template<typename T>
struct io_wrapper
{
	template<typename C>
	static void onRead(oishii::BinaryReader&, C&)
	{
		DebugReport("Unimplemented IO wrapper called.\n");
	}
};

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
		template<typename T, typename TRaw>
		T as()
		{
			return static_cast<T>(raw<TRaw>());
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

	template<typename TIdx, typename T>
	void indexedContained(T& out, MatSec sec, u32 stride)
	{
		if (const auto ofs = indexed<TIdx>(sec, stride).ofs)
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
				io_wrapper<T::value_type>::onRead(reader, it);

				++out.nElements;
			}
		}
	}
};
template<>
struct io_wrapper<gx::ZMode>
{
	static void onRead(oishii::BinaryReader& reader, gx::ZMode& out)
	{
		out.compare = reader.read<u8>();
		out.function = static_cast<gx::Comparison>(reader.read<u8>());
		out.update = reader.read<u8>();
		reader.read<u8>();
	}
};

template<>
struct io_wrapper<gx::AlphaComparison>
{
	static void onRead(oishii::BinaryReader& reader, gx::AlphaComparison& c)
	{
		c.compLeft = static_cast<gx::Comparison>(reader.read<u8>());
		c.refLeft = reader.read<u8>();
		c.op = static_cast<gx::AlphaOp>(reader.read<u8>());
		c.compRight = static_cast<gx::Comparison>(reader.read<u8>());
		c.refRight = reader.read<u8>();
		reader.seek(3);
	}
};

template<>
struct io_wrapper<gx::BlendMode>
{
	static void onRead(oishii::BinaryReader& reader, gx::BlendMode& c)
	{
		c.type = static_cast<gx::BlendModeType>(reader.read<u8>());
		c.source = static_cast<gx::BlendModeFactor>(reader.read<u8>());
		c.dest = static_cast<gx::BlendModeFactor>(reader.read<u8>());
		c.logic = static_cast<gx::LogicOp>(reader.read<u8>());
	}
};

template<>
struct io_wrapper<gx::Color>
{
	static void onRead(oishii::BinaryReader& reader, gx::Color& out)
	{
		out.r = reader.read<u8>();
		out.g = reader.read<u8>();
		out.b = reader.read<u8>();
		out.a = reader.read<u8>();
	}
};

template<>
struct io_wrapper<gx::ChannelControl>
{
	static void onRead(oishii::BinaryReader& reader, gx::ChannelControl& out)
	{
		out.enabled = reader.read<u8>();
		out.Material = static_cast<gx::ColorSource>(reader.read<u8>());
		out.lightMask = static_cast<gx::LightID>(reader.read<u8>());
		out.diffuseFn = static_cast<gx::DiffuseFunction>(reader.read<u8>());
		constexpr const std::array<gx::AttenuationFunction, 4> cvt = {
			gx::AttenuationFunction::None,
			gx::AttenuationFunction::Specular,
			gx::AttenuationFunction::None,
			gx::AttenuationFunction::Spotlight };
		out.attenuationFn = cvt[reader.read<u8>()];
		out.Ambient = static_cast<gx::ColorSource>(reader.read<u8>());

		reader.read<u16>();
	}
};
template<>
struct io_wrapper<gx::ColorS10>
{
	static void onRead(oishii::BinaryReader& reader, gx::ColorS10& out)
	{
		out.r = reader.read<s16>();
		out.g = reader.read<s16>();
		out.b = reader.read<s16>();
		out.a = reader.read<s16>();
	}
};

template<>
struct io_wrapper<Material::NBTScale>
{
	static void onRead(oishii::BinaryReader& reader, Material::NBTScale& c)
	{
		c.enable = static_cast<bool>(reader.read<u8>());
		reader.seek(3);
		c.scale << reader;
	}
};

// J3D specific
template<>
struct io_wrapper<gx::TexCoordGen>
{
	static void onRead(oishii::BinaryReader& reader, gx::TexCoordGen& ctx)
	{
		ctx.func = static_cast<gx::TexGenType>(reader.read<u8>());
		ctx.sourceParam = static_cast<gx::TexGenSrc>(reader.read<u8>());

		ctx.matrix = static_cast<gx::TexMatrix>(reader.read<u8>());
		reader.read<u8>();
		// Assume no postmatrix
		ctx.normalize = false;
		ctx.postMatrix = gx::PostTexMatrix::Identity;
	}
};

template<>
struct io_wrapper<Material::TexMatrix>
{
	static void onRead(oishii::BinaryReader& reader, Material::TexMatrix& c)
	{
		c.projection = static_cast<gx::TexGenType>(reader.read<u8>());
		// FIXME:
		//	assert(c.projection == gx::TexGenType::Matrix2x4 || c.projection == gx::TexGenType::Matrix3x4);

		c.mappingMethod = reader.read<u8>();
		c.maya = c.mappingMethod & 0x80;
		c.mappingMethod &= ~0x80;

		reader.read<u16>();

		c.origin << reader;
		c.scale << reader;
		c.rotate = static_cast<f32>(reader.read<s16>()) * 180.f / (f32)0x7FFF;
		reader.read<u16>();
		c.translate << reader;
		// TODO: effect matrix
	}
};

struct SwapSel
{
	u8 colorChanSel, texSel;
};
template<>
struct io_wrapper<SwapSel>
{
	static void onRead(oishii::BinaryReader& reader, SwapSel& c)
	{
		c.colorChanSel = reader.read<u8>();
		c.texSel = reader.read<u8>();
		reader.read<u16>();
	}
};
template<>
struct io_wrapper<gx::Shader::SwapTableEntry>
{
	static void onRead(oishii::BinaryReader& reader, gx::Shader::SwapTableEntry& c)
	{
		c.r = static_cast<gx::ColorComponent>(reader.read<u8>());
		c.g = static_cast<gx::ColorComponent>(reader.read<u8>());
		c.b = static_cast<gx::ColorComponent>(reader.read<u8>());
		c.a = static_cast<gx::ColorComponent>(reader.read<u8>());
	}
};

struct TevOrder
{
	gx::ColorSelChanApi rasOrder;
	u8 texMap, texCoord;
};

template<>
struct io_wrapper<TevOrder>
{
	static void onRead(oishii::BinaryReader& reader, TevOrder& c)
	{
		c.texCoord = reader.read<u8>();
		c.texMap = reader.read<u8>();
		c.rasOrder = static_cast<gx::ColorSelChanApi>(reader.read<u8>());
		reader.read<u8>();
	}
};

template<>
struct io_wrapper<gx::TevStage>
{
	static void onRead(oishii::BinaryReader& reader, gx::TevStage& c)
	{
		const auto unk1 = reader.read<u8>();
		c.colorStage.a = static_cast<gx::TevColorArg>(reader.read<u8>());
		c.colorStage.b = static_cast<gx::TevColorArg>(reader.read<u8>());
		c.colorStage.c = static_cast<gx::TevColorArg>(reader.read<u8>());
		c.colorStage.d = static_cast<gx::TevColorArg>(reader.read<u8>());

		c.colorStage.formula = static_cast<gx::TevColorOp>(reader.read<u8>());
		c.colorStage.bias = static_cast<gx::TevBias>(reader.read<u8>());
		c.colorStage.scale = static_cast<gx::TevScale>(reader.read<u8>());
		c.colorStage.clamp = reader.read<u8>();
		c.colorStage.out = static_cast<gx::TevReg>(reader.read<u8>());

		c.alphaStage.a = static_cast<gx::TevAlphaArg>(reader.read<u8>());
		c.alphaStage.b = static_cast<gx::TevAlphaArg>(reader.read<u8>());
		c.alphaStage.c = static_cast<gx::TevAlphaArg>(reader.read<u8>());
		c.alphaStage.d = static_cast<gx::TevAlphaArg>(reader.read<u8>());

		c.alphaStage.formula = static_cast<gx::TevAlphaOp>(reader.read<u8>());
		c.alphaStage.bias = static_cast<gx::TevBias>(reader.read<u8>());
		c.alphaStage.scale = static_cast<gx::TevScale>(reader.read<u8>());
		c.alphaStage.clamp = reader.read<u8>();
		c.alphaStage.out = static_cast<gx::TevReg>(reader.read<u8>());

		const auto unk2 = reader.read<u8>();
	}
};

template<>
struct io_wrapper<MaterialData::Fog>
{
	static void onRead(oishii::BinaryReader& reader, MaterialData::Fog& f)
	{
		f.type = static_cast<MaterialData::Fog::Type>(reader.read<u8>());
		f.enabled = reader.read<u8>();
		f.center = reader.read<u16>();
		f.startZ = reader.read<f32>();
		f.endZ = reader.read<f32>();
		f.nearZ = reader.read<f32>();
		f.farZ = reader.read<f32>();
		io_wrapper<gx::Color>::onRead(reader, f.color);
		f.rangeAdjTable = reader.readX<u16, 10>();
	}
};
void readMatEntry(Material& mat, MatLoader& loader, oishii::BinaryReader& reader)
{
	oishii::DebugExpectSized dbg(reader, 332);
	oishii::BinaryReader::ScopedRegion g(reader, "Material");

	assert(reader.tell() % 4 == 0);
	mat.flag = reader.read<u8>();
	mat.cullMode = loader.indexed<u8>(MatSec::CullModeInfo).as<gx::CullMode, u32>();
	mat.info.nColorChannel = loader.indexed<u8>(MatSec::NumTexGens).raw();
	mat.info.nTexGen = loader.indexed<u8>(MatSec::NumTexGens).raw();
	mat.info.nTevStage = loader.indexed<u8>(MatSec::NumTevStages).raw();
	mat.earlyZComparison = loader.indexed<u8>(MatSec::ZCompareInfo).as<bool, u8>();
	loader.indexedContained<u8>(mat.ZMode, MatSec::ZModeInfo, 4);
	mat.dither = loader.indexed<u8>(MatSec::DitherInfo).as<bool, u8>();

	dbg.assertSince(8);
	loader.indexedContainer<u16>(mat.matColors, MatSec::MaterialColors, 4);
	dbg.assertSince(0xc);

	loader.indexedContainer<u16>(mat.colorChanControls, MatSec::ColorChannelInfo, 8);
	loader.indexedContainer<u16>(mat.ambColors, MatSec::AmbientColors, 4);
	loader.indexedContainer<u16>(mat.lightColors, MatSec::LightInfo, 4);

	dbg.assertSince(0x28);
	loader.indexedContainer<u16>(mat.texGenInfos, MatSec::TexGenInfo, 4);
	const auto post_tg = reader.readX<u16, 8>();

	dbg.assertSince(0x48);
	loader.indexedContainer<u16>(mat.texMatrices, MatSec::TexMatrixInfo, 100);
	loader.indexedContainer<u16>(mat.postTexMatrices, MatSec::PostTexMatrixInfo, 100);

	dbg.assertSince(0x84);

	const auto TextureIndexes = reader.readX<u16, 8>();

	{
		dbg.assertSince(0x094);
		loader.indexedContainer<u16>(mat.tevKonstColors, MatSec::TevKonstColors, 4);
		// TEV/Ind: Read specially
		// ind block stride: 312

		//
		dbg.assertSince(0x09C);

		const auto kc_sels = reader.readX<u8, 16>();
		const auto ka_sels = reader.readX<u8, 16>();

		const auto get_kc = [&](size_t i) { return static_cast<gx::TevKColorSel>(kc_sels[i]); };
		const auto get_ka = [&](size_t i) { return static_cast<gx::TevKAlphaSel>(ka_sels[i]); };

		dbg.assertSince(0x0BC);
		array_vector<TevOrder, 16> tevOrderInfos;
		loader.indexedContainer<u16>(tevOrderInfos, MatSec::TevOrderInfo, 4);


		loader.indexedContainer<u16>(mat.tevColors, MatSec::TevColors, 4);

		// FIXME: Directly read into material
		array_vector<gx::TevStage, 16> tevStageInfos;
		loader.indexedContainer<u16>(tevStageInfos, MatSec::TevStageInfo, 20);
		for (int i = 0; i < tevStageInfos.size(); ++i)
		{
			mat.shader.mStages.push_back(tevStageInfos[i]);
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

		// FIXME
		array_vector<gx::Shader::SwapTableEntry, 16> swapTables;
		loader.indexedContainer<u16>(swapTables, MatSec::TevSwapModeTableInfo, 4);
		//	assert(swapTables.size() == 4);
		for (int i = 0; i < 4; ++i)
			mat.shader.mSwapTable[i] = swapTables[i];
	}
	dbg.assertSince(0x144);
	loader.indexedContained<u16>(mat.fogInfo, MatSec::FogInfo, 44);
	loader.indexedContained<u16>(mat.alphaCompare, MatSec::AlphaCompareInfo, 8);
	loader.indexedContained<u16>(mat.blendMode, MatSec::BlendModeInfo, 4);
	loader.indexedContained<u16>(mat.nbtScale, MatSec::NBTScaleInfo, 16);
}


void readMAT3(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
	if (enterSection(ctx, 'MAT3'))
	{
		ScopedSection g(reader, "Materials");

		u16 size = reader.read<u16>();
		ctx.mdl.mMaterials.resize(size);
		ctx.materialIdLut.resize(size);
		reader.read<u16>();

		const auto [ofsMatData, ofsRemapTable, ofsStringTable] = reader.readX<s32, 3>();
		MatLoader loader{ reader.readX<s32, static_cast<u32>(MatSec::Max)>(), g.start, reader };

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
			auto& mat = ctx.mdl.mMaterials[i];
			reader.seekSet(g.start + ofsMatData + ctx.materialIdLut[i] * 0x14c);
			mat.id = ctx.materialIdLut[i];
			mat.name = nameTable[i];

			readMatEntry(mat, loader, reader);
		}
	}
}

}
