/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include "BMD.hpp"
#include <LibCube/JSystem/J3D/Model.hpp>
#include <map>
#include <type_traits>
#include <algorithm>
#include <tuple>

#include <oishii/writer/node.hxx>
#include <oishii/writer/linker.hxx>

#include "SceneGraph.hpp"


namespace libcube::jsystem {

template<typename T, u32 r, u32 c, typename TM, glm::qualifier qM>
void transferMatrix(glm::mat<r, c, TM, qM>& mat, T& stream)
{
	for (int i = 0; i < r; ++i)
		for (int j = 0; j < c; ++j)
			stream.transfer(mat[i][j]);
}

std::vector<std::string> readNameTable(oishii::BinaryReader& reader)
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

#pragma region Sectioning
bool BMDImporter::enterSection(oishii::BinaryReader& reader, u32 id)
{
	const auto sec = mSections.find(id);
	if (sec == mSections.end())
		return false;

	reader.seekSet(sec->second.streamPos - 8);
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
#pragma endregion

#pragma region INF1
void BMDImporter::readInformation(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{
	if (enterSection(reader, 'INF1'))
	{
		ScopedSection g(reader, "Information");

		u16 flag = reader.read<u16>();
		reader.signalInvalidityLast<u16, oishii::UncommonInvalidity>("Flag");
		reader.read<u16>();

		// TODO -- Use these for validation
		u32 nPacket = reader.read<u32>();
		u32 nVertex = reader.read<u32>();

		ctx.mdl.info.mScalingRule = static_cast<J3DModel::Information::ScalingRule>(flag & 0xf);
		// FIXME
		//assert((flag & ~0xf) == 0);

		reader.dispatch<SceneGraph, oishii::Indirection<0, s32, oishii::Whence::At>>(ctx, g.start);
	}
}
#pragma endregion

#pragma region EVP1/DRW1
void BMDImporter::readDrawMatrices(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{

	// We infer DRW1 -- one bone -> singlebound, else create envelope
	std::vector<DrawMatrix> envelopes;

	// First read our envelope data
	if (enterSection(reader, 'EVP1'))
	{
		ScopedSection g(reader, "Envelopes");

		u16 size = reader.read<u16>();
		envelopes.resize(size);
		reader.read<u16>();

		const auto [ofsMatrixSize, ofsMatrixIndex, ofsMatrixWeight, ofsMatrixInvBind] = reader.readX<s32, 4>();

		int mtxId = 0;
		int maxJointIndex = -1;

		reader.seekSet(g.start);

		for (int i = 0; i < size; ++i)
		{
			const auto num = reader.peekAt<u8>(ofsMatrixSize + i);

			for (int j = 0; j < num; ++j)
			{
				const auto index = reader.peekAt<u16>(ofsMatrixIndex + mtxId * 2);
				const auto influence = reader.peekAt<f32>(ofsMatrixWeight + mtxId * 4);

				envelopes[i].mWeights.emplace_back(index, influence);

				if (index > maxJointIndex)
					maxJointIndex = index;

				++mtxId;
			}
		}

		for (int i = 0; i < maxJointIndex + 1; ++i)
		{
			// TODO: Fix
			oishii::Jump<oishii::Whence::Set> gInv(reader, g.start + ofsMatrixInvBind + i * 0x30);

			transferMatrix(ctx.mdl.mJoints[i].inverseBindPoseMtx, reader);
		}
	}

	// Now construct vertex draw matrices.
	if (enterSection(reader, 'DRW1'))
	{
		ScopedSection g(reader, "Vertex Draw Matrix");

		ctx.mdl.mDrawMatrices.clear();
		ctx.mdl.mDrawMatrices.resize(reader.read<u16>());
		reader.read<u16>();

		const auto [ofsPartialWeighting, ofsIndex] = reader.readX<s32, 2>();

		reader.seekSet(g.start);

		int i = 0;
		for (auto& mtx : ctx.mdl.mDrawMatrices)
		{

			const auto multipleInfluences = reader.peekAt<u8>(ofsPartialWeighting + i);
			const auto index = reader.peekAt<u16>(ofsIndex + i * 2);
			if (multipleInfluences)
				printf("multiple influences: %u\n", index);

			mtx = multipleInfluences ? envelopes[index] : DrawMatrix{ std::vector<DrawMatrix::MatrixWeight>{DrawMatrix::MatrixWeight(index, 1.0f)} };
			i++;
		}
	}
}
#pragma endregion

#pragma region JNT1
void BMDImporter::readJoints(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{
	if (enterSection(reader, 'JNT1'))
	{
		ScopedSection g(reader, "Joints");

		u16 size = reader.read<u16>();
		ctx.mdl.mJoints.resize(size);
		ctx.jointIdLut.resize(size);
		reader.read<u16>();

		const auto [ofsJointData, ofsRemapTable, ofsStringTable] = reader.readX<s32, 3>();
		reader.seekSet(g.start);

		// Compressible resources in J3D have a relocation table (necessary for interop with other animations that access by index)
		// FIXME: Note and support saving identically
		reader.seekSet(g.start + ofsRemapTable);

		bool sorted = true;
		for (int i = 0; i < size; ++i)
		{
			ctx.jointIdLut[i] = reader.read<u16>();

			if (ctx.jointIdLut[i] != i)
				sorted = false;
		}

		if (!sorted)
			DebugReport("Joint IDS are remapped.\n");


		// FIXME: unnecessary allocation of a vector.
		reader.seekSet(ofsStringTable + g.start);
		const auto nameTable = readNameTable(reader);

		for (int i = 0; i < size; ++i)
		{
			auto& joint = ctx.mdl.mJoints[i];
			reader.seekSet(g.start + ofsJointData + ctx.jointIdLut[i] * 0x40);
			joint.id = ctx.jointIdLut[i]; // TODO
			joint.name = nameTable[i];
			const u16 flag = reader.read<u16>();
			joint.flag = flag & 0xf;
			joint.bbMtxType = static_cast<Joint::MatrixType>(flag >> 4);
			const u8 mayaSSC = reader.read<u8>();
			joint.mayaSSC = mayaSSC == 0xff ? false : mayaSSC;
			reader.read<u8>();
			joint.scale << reader;
			joint.rotate.x = static_cast<f32>(reader.read<s16>()) / f32(0x7ff) * M_PI;
			joint.rotate.y = static_cast<f32>(reader.read<s16>()) / f32(0x7ff) * M_PI;
			joint.rotate.z = static_cast<f32>(reader.read<s16>()) / f32(0x7ff) * M_PI;
			reader.read<u16>();
			joint.translate << reader;
			joint.boundingSphereRadius = reader.read<f32>();
			joint.boundingBox << reader;
		}
	}
}
#pragma endregion

#pragma region MAT3
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

void BMDImporter::readMaterials(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{
	if (enterSection(reader, 'MAT3'))
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
#pragma endregion


#pragma region VTX1
void BMDImporter::readVertexBuffers(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{
	if (!enterSection(reader, 'VTX1'))
		return;

	ScopedSection g(reader, "Vertex Buffers");

	const auto ofsFmt = reader.read<s32>();
	const auto ofsData = reader.readX<s32, 13>();

	reader.seekSet(g.start + ofsFmt);
	for (gx::VertexBufferAttribute type = reader.read<gx::VertexBufferAttribute>();
		type != gx::VertexBufferAttribute::Terminate;
		type = reader.read<gx::VertexBufferAttribute>())
	{
		const auto comp = reader.read<u32>();
		const auto data = reader.read<u32>();
		const auto gen_data = static_cast<gx::VertexBufferType::Generic>(data);
		const auto gen_comp_size = (gen_data == gx::VertexBufferType::Generic::f32) ? 4 :
			(gen_data == gx::VertexBufferType::Generic::u16 || gen_data == gx::VertexBufferType::Generic::s16) ? 2 :
			1;
		const auto shift = reader.read<u8>();
		reader.seek(3);

		assert(0 <= shift && shift <= 31);

		auto estride = 0;
		// FIXME: type punning
		void* buf = nullptr;

		VBufferKind bufkind = VBufferKind::undefined;

		switch (type)
		{
		case gx::VertexBufferAttribute::Position:
			buf = &ctx.mdl.mBufs.pos;
			bufkind = VBufferKind::position;
			ctx.mdl.mBufs.pos.mQuant = VQuantization(
				gx::VertexComponentCount(static_cast<gx::VertexComponentCount::Position>(comp)),
				gx::VertexBufferType(gen_data),
				gen_data != gx::VertexBufferType::Generic::f32 ? shift : 0,
				(comp + 2) * gen_comp_size);
			estride = ctx.mdl.mBufs.pos.mQuant.stride;
			break;
		case gx::VertexBufferAttribute::Normal:
			buf = &ctx.mdl.mBufs.norm;
			bufkind = VBufferKind::normal;
			ctx.mdl.mBufs.norm.mQuant = VQuantization(
				gx::VertexComponentCount(static_cast<gx::VertexComponentCount::Normal>(comp)),
				gx::VertexBufferType(gen_data),
				gen_data == gx::VertexBufferType::Generic::s8 ? 6 :
				gen_data == gx::VertexBufferType::Generic::s16 ? 14 : 0,
				3 * gen_comp_size
			);
			estride = ctx.mdl.mBufs.norm.mQuant.stride;
			break;
		case gx::VertexBufferAttribute::Color0:
		case gx::VertexBufferAttribute::Color1:
		{
			auto& clr = ctx.mdl.mBufs.color[static_cast<size_t>(type) - static_cast<size_t>(gx::VertexBufferAttribute::Color0)];
			buf = &clr;
			bufkind = VBufferKind::color;
			clr.mQuant = VQuantization(
				gx::VertexComponentCount(static_cast<gx::VertexComponentCount::Color>(comp)),
				gx::VertexBufferType(static_cast<gx::VertexBufferType::Color>(data)),
				0,
				(comp + 3)* [](gx::VertexBufferType::Color c) {
					using ct = gx::VertexBufferType::Color;
					switch (c)
					{
					case ct::FORMAT_16B_4444:
					case ct::FORMAT_16B_565:
						return 2;
					case ct::FORMAT_24B_6666:
					case ct::FORMAT_24B_888:
						return 3;
					case ct::FORMAT_32B_8888:
					case ct::FORMAT_32B_888x:
						return 4;
					default:
						throw "Invalid color data type.";
					}
				}(static_cast<gx::VertexBufferType::Color>(data)));
			estride = clr.mQuant.stride;
			break;
		}
		case gx::VertexBufferAttribute::TexCoord0:
		case gx::VertexBufferAttribute::TexCoord1:
		case gx::VertexBufferAttribute::TexCoord2:
		case gx::VertexBufferAttribute::TexCoord3:
		case gx::VertexBufferAttribute::TexCoord4:
		case gx::VertexBufferAttribute::TexCoord5:
		case gx::VertexBufferAttribute::TexCoord6:
		case gx::VertexBufferAttribute::TexCoord7: {
			auto& uv = ctx.mdl.mBufs.uv[static_cast<size_t>(type) - static_cast<size_t>(gx::VertexBufferAttribute::TexCoord0)];
			buf = &uv;
			bufkind = VBufferKind::textureCoordinate;
			uv.mQuant = VQuantization(
				gx::VertexComponentCount(static_cast<gx::VertexComponentCount::TextureCoordinate>(comp)),
				gx::VertexBufferType(gen_data),
				gen_data != gx::VertexBufferType::Generic::f32 ? shift : 0,
				(comp + 1)* gen_comp_size
			);
			estride = ctx.mdl.mBufs.pos.mQuant.stride;
			break;
		}
		default:
			assert(false);
		}

		assert(estride);
		assert(bufkind != VBufferKind::undefined);

		const auto getDataIndex = [&](gx::VertexBufferAttribute attr)
		{
			static const constexpr std::array<s8, static_cast<size_t>(gx::VertexBufferAttribute::NormalBinormalTangent) + 1> lut = {
				-1, -1, -1, -1, -1, -1, -1, -1, -1,
				0, // Position
				1, // Normal
				3, 4, // Color
				5, 6, 7, 8, // UV
				9, 10, 11, 12,
				-1, -1, -1, -1, 2
			};

			static_assert(lut[static_cast<size_t>(gx::VertexBufferAttribute::NormalBinormalTangent)] == 2, "NBT");

			const auto attr_i = static_cast<size_t>(attr);

			assert(attr_i < lut.size());

			return attr_i < lut.size() ? lut[attr_i] : -1;
		};

		const auto idx = getDataIndex(type);
		const auto ofs = g.start + ofsData[idx];
		{
			oishii::Jump<oishii::Whence::Set> g_bufdata(reader, ofs);
			s32 size = 0;

			for (int i = idx + 1; i < ofsData.size(); ++i)
			{
				if (ofsData[i])
				{
					size = static_cast<s32>(ofsData[i]) - ofsData[idx];
					break;
				}
			}

			if (!size)
				size = g.size - ofsData[idx];

			assert(size > 0);


			// Desirable to round down
			const auto ensize = (size /*+ estride*/) / estride;
			//	assert(size % estride == 0);
			assert(ensize < u32(u16(-1)) * 3);

			// FIXME: Alignment padding trim

			switch (bufkind)
			{
			case VBufferKind::position: {
				auto pos = reinterpret_cast<decltype(ctx.mdl.mBufs.pos)*>(buf);
					
				pos->mData.resize(ensize);
				for (int i = 0; i < ensize; ++i)
					pos->readBufferEntryGeneric(reader, pos->mData[i]);
				break;
			}
			case VBufferKind::normal: {
				auto nrm = reinterpret_cast<decltype(ctx.mdl.mBufs.norm)*>(buf);

				nrm->mData.resize(ensize);
				for (int i = 0; i < ensize; ++i)
					nrm->readBufferEntryGeneric(reader, nrm->mData[i]);
				break;
			}
			case VBufferKind::color: {
				auto clr = reinterpret_cast<decltype(ctx.mdl.mBufs.color)::value_type*>(buf);

				clr->mData.resize(ensize);
				for (int i = 0; i < ensize; ++i)
					clr->readBufferEntryColor(reader, clr->mData[i]);
				break;
			}
			case VBufferKind::textureCoordinate: {
				auto uv = reinterpret_cast<decltype(ctx.mdl.mBufs.uv)::value_type*>(buf);

				uv->mData.resize(ensize);
				for (int i = 0; i < ensize; ++i)
					uv->readBufferEntryGeneric(reader, uv->mData[i]);
				break;
			}
			}
		}
	}
}
#pragma endregion

#pragma region SHP1

void BMDImporter::readShapes(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{
	if (enterSection(reader, 'SHP1'))
	{
		ScopedSection g(reader, "Shapes");

		u16 size = reader.read<u16>();
		ctx.mdl.mShapes.resize(size);
		ctx.shapeIdLut.resize(size);
		reader.read<u16>();

		const auto [ofsShapeData, ofsShapeLut, ofsStringTable,
			// Describes the vertex buffers: GX_VA_POS, GX_INDEX16
			// (We can get this from VTX1)
			ofsVcdList, // "attr table"

			// DRW indices
			ofsDrwIndices,

			// Just a display list
			ofsDL, // "prim data"

			// struct MtxData {
			// s16 current_matrix; // -1 for singlebound
			// u16 matrix_list_size; // Vector of matrix indices; limited by hardware
			// int matrix_list_start;
			// }
			// Each sampled by matrix primitives
			ofsMtxData, // "mtx data"

			// size + offset?
			// matrix primitive splits
			ofsMtxPrimAccessor // "mtx grp"
		] = reader.readX<s32, 8>();
		reader.seekSet(g.start);

		// Compressible resources in J3D have a relocation table (necessary for interop with other animations that access by index)
		reader.seekSet(g.start + ofsShapeLut);

		bool sorted = true;
		for (int i = 0; i < size; ++i)
		{
			ctx.shapeIdLut[i] = reader.read<u16>();

			if (ctx.shapeIdLut[i] != i)
				sorted = false;
		}

		if (!sorted)
			DebugReport("Shape IDS are remapped.\n");


		// Unused
		// reader.seekSet(ofsStringTable + g.start);
		// const auto nameTable = readNameTable(reader);

		for (int si = 0; si < size; ++si)
		{
			auto& shape = ctx.mdl.mShapes[si];
			reader.seekSet(g.start + ofsShapeData + ctx.shapeIdLut[si] * 0x28);
			shape.id = ctx.shapeIdLut[si];
			// shape.name = nameTable[si];
			shape.mode = static_cast<ShapeData::Mode>(reader.read<u8>());
			assert(shape.mode < ShapeData::Mode::Max);
			reader.read<u8>();
			// Number of matrix primitives (mtxGrpCnt)
			auto num_matrix_prims = reader.read<u16>();
			auto ofs_vcd_list = reader.read<u16>();
			// current mtx/mtx list
			auto first_matrix_list = reader.read<u16>();
			// "Packet" or mtx prim summary (accessor) idx
			auto first_mtx_prim_idx = reader.read<u16>();
			reader.read<u16>();
			shape.bsphere = reader.read<f32>();
			shape.bbox << reader;

			// Read VCD attributes
			reader.seekSet(g.start + ofsVcdList + ofs_vcd_list);
			gx::VertexAttribute attr = gx::VertexAttribute::Undefined;
			while ((attr = reader.read<gx::VertexAttribute>()) != gx::VertexAttribute::Terminate)
				shape.mVertexDescriptor.mAttributes[attr] = reader.read<gx::VertexAttributeType>();

			// Calculate the VCD bitfield
			shape.mVertexDescriptor.calcVertexDescriptorFromAttributeList();
			const u32 vcd_size = shape.mVertexDescriptor.getVcdSize();

			// Read the two-layer primitives

			
			for (u16 i = 0; i < num_matrix_prims; ++i)
			{
				const u32 prim_idx = first_mtx_prim_idx + i;
				reader.seekSet(g.start + ofsMtxPrimAccessor + prim_idx * 8);
				const auto [dlSz, dlOfs] = reader.readX<u32, 2>();

				struct MatrixData
				{
					s16 current_matrix;
					std::vector<s16> matrixList; // DRW1
				};
				auto readMatrixData = [&]()
				{
					oishii::Jump<oishii::Whence::At> j(reader, g.start + ofsMtxData + first_matrix_list * 8);
					MatrixData out;
					out.current_matrix = reader.read<s16>();
					u16 list_size = reader.read<u16>();
					s32 list_start = reader.read<s32>();
					out.matrixList.resize(list_size);
					{
						oishii::Jump<oishii::Whence::At> d(reader, g.start + ofsDrwIndices + list_start * 2);
						for (u16 i = 0; i < list_size; ++i)
						{
							out.matrixList[i] = reader.read<s16>();
						}
					}
					return out;
				};

				// Mtx Prim Data
				MatrixData mtxPrimHdr = readMatrixData();
				MatrixPrimitive& mprim = shape.mMatrixPrimitives.emplace_back(mtxPrimHdr.current_matrix, mtxPrimHdr.matrixList);
				
				// Now read prim data..
				// (Stripped down display list interpreter function)
				
				reader.seekSet(g.start + ofsDL + dlOfs);
				while (reader.tell() < g.start + ofsDL + dlOfs + dlSz)
				{
					const u8 tag = reader.read<u8, oishii::EndianSelect::Current, true>();
					if (tag == 0) break;
					assert(tag & 0x80 && "Unexpected GPU command in display list.");
					const gx::PrimitiveType type = gx::DecodeDrawPrimitiveCommand(tag);
					u16 nVerts = reader.read<u16, oishii::EndianSelect::Current, true>();
					IndexedPrimitive& prim = mprim.mPrimitives.emplace_back(type, nVerts);

					for (u16 vi = 0; vi < nVerts; ++vi)
					{
						for (int a = 0; a < (int)gx::VertexAttribute::Max; ++a)
						{
							if (shape.mVertexDescriptor[(gx::VertexAttribute)a])
							{
								u16 val = 0;

								switch (shape.mVertexDescriptor.mAttributes[(gx::VertexAttribute)a])
								{
								case gx::VertexAttributeType::None:
									break;
								case gx::VertexAttributeType::Byte:
									val = reader.read<u8, oishii::EndianSelect::Current, true>();
									break;
								case gx::VertexAttributeType::Short:
									val = reader.read<u16, oishii::EndianSelect::Current, true>();
									break;
								case gx::VertexAttributeType::Direct:
									if (((gx::VertexAttribute)a) != gx::VertexAttribute::PositionNormalMatrixIndex)
									{
										assert(!"Direct vertex data is unsupported.");
										throw "";
									}
									val = reader.read<u8, oishii::EndianSelect::Current, true>(); // As PNM indices are always direct, we still use them in an all-indexed vertex
									break;
								default:
									assert("!Unknown vertex attribute format.");
									throw "";
								}

								prim.mVertices[vi][(gx::VertexAttribute)a] = val;
							}
						}
					}
				}
			}
		}
	}
}
#pragma endregion
void BMDImporter::lex(oishii::BinaryReader& reader, u32 sec_count) noexcept
{
	mSections.clear();
	for (u32 i = 0; i < sec_count; ++i)
	{
		const u32 secType = reader.read<u32>();
		const u32 secSize = reader.read<u32>();

		{
			oishii::JumpOut g(reader, secSize - 8);
			switch (secType)
			{
			case 'INF1':
			case 'VTX1':
			case 'EVP1':
			case 'DRW1':
			case 'JNT1':
			case 'SHP1':
			case 'MAT3':
			case 'MDL3':
			case 'TEX1':
				mSections[secType] = { reader.tell(), secSize };
				break;
			default:
				reader.warnAt("Unexpected section type.", reader.tell() - 8, reader.tell() - 4);
				break;
			}
		}
	}
}
void BMDImporter::readBMD(oishii::BinaryReader& reader, BMDOutputContext& ctx)
{
	reader.setEndian(true);
	reader.expectMagic<'J3D2'>();


	u32 bmdVer = reader.read<u32>();
	if (bmdVer != 'bmd3' && bmdVer != 'bdl4')
	{
		reader.signalInvalidityLast<u32, oishii::MagicInvalidity<'bmd3'>>();
		error = true;
		return;
	}

	// TODO: Validate file size.
	const auto fileSize = reader.read<u32>();
	const auto sec_count = reader.read<u32>();

	// Skip SVR3 header
	reader.seek<oishii::Whence::Current>(16);

	// Skim through sections
	lex(reader, sec_count);

	// Read VTX1
	readVertexBuffers(reader, ctx);

	// Read JNT1
	readJoints(reader, ctx);

	// Read EVP1 and DRW1
	readDrawMatrices(reader, ctx);

	

	// Read SHP1
	readShapes(reader, ctx);
	// Read TEX1
	// Read MAT3
	readMaterials(reader, ctx);

	// Read INF1
	readInformation(reader, ctx);

	// FIXME: MDL3
}
bool BMDImporter::tryRead(oishii::BinaryReader& reader, pl::FileState& state)
{
	//oishii::BinaryReader::ScopedRegion g(reader, "J3D Binary Model Data (.bmd)");
	auto& j3dc = static_cast<J3DCollection&>(state);
	readBMD(reader, BMDOutputContext{ j3dc.mModel });
	return !error;
}


template<typename T>
struct LinkNode final : public oishii::Node, T
{
	LinkNode(bool b=false)
		: T(), Node(T::getNameId())
	{
		transferOwnershipToLinker(b);
	}
	template<typename S>
	LinkNode(S& arg)
		: T(arg), Node(T::getNameId())
	{
		transferOwnershipToLinker(true);
	}
	LinkNode* asLeaf()
	{
		getLinkingRestriction().options |= oishii::LinkingRestriction::Leaf;
		return this;
	}
	eResult write(oishii::Writer& writer) const noexcept
	{
		T::write(writer);
		return eResult::Success;
	}

	eResult gatherChildren(std::vector<const oishii::Node*>& out) const noexcept override
	{
		for (auto ptr : T::gatherChildren())
			out.push_back(ptr.get());

		return eResult::Success;
	}
	const oishii::Node& getSelf() const override
	{
		return *this;
	}
};
template<typename T>
union linker_ptr
{
	const T* get() const { return data; }
	T* get() { return data; }

	T* operator->() { return get(); }
	const T* operator->() const { return get(); }


	linker_ptr(T* ptr)
		: data(ptr)
	{}
	linker_ptr(const linker_ptr& other)
	{
		this->data = other.data;
	}

private:
	T* data;
};

template<typename T>
auto make_tempnode = []()
{
	// Deleted by linker
	return linker_ptr<LinkNode<T>>(new LinkNode<T>(true));
};

struct INF1Node
{
	static const char* getNameId() { return "INF1 InFormation"; }
	virtual const oishii::Node& getSelf() const = 0;

	void write(oishii::Writer& writer) const
	{
		if (!mdl) return;

		writer.write<u32, oishii::EndianSelect::Big>('INF1');
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("VTX1"/*getSelf(), oishii::Hook::EndOfChildren*/) });

		writer.write<u16>(static_cast<u16>(mdl->info.mScalingRule) & 0xf);
		writer.write<u16>(-1);
		// Packet count
		writer.write<u32>(-1); // TODO
		// Vertex position count
		writer.write<u32>(mdl->mBufs.pos.mData.size());

		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("SceneGraph")
			});
	}


	std::vector<linker_ptr<oishii::Node>> gatherChildren() const
	{
		return { SceneGraph::getLinkerNode(*mdl, true) };
	}

	J3DModel* mdl = nullptr;
};

struct VTX1Node
{
	static const char* getNameId() { return "VTX1"; }
	virtual const oishii::Node& getSelf() const = 0;

	int computeNumOfs() const
	{
		int numOfs = 0;
		if (mdl)
		{
			if (!mdl->mBufs.pos.mData.empty()) ++numOfs;
			if (!mdl->mBufs.norm.mData.empty()) ++numOfs;
			for (int i = 0; i < 2; ++i)
				if (!mdl->mBufs.color[i].mData.empty()) ++numOfs;
			for (int i = 0; i < 8; ++i)
				if (!mdl->mBufs.uv[i].mData.empty()) ++numOfs;
		}
		return numOfs;
	}

	void write(oishii::Writer& writer) const
	{
		if (!mdl) return;

		writer.write<u32, oishii::EndianSelect::Big>('VTX1');
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook(getSelf(), oishii::Hook::EndOfChildren) });

		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("Format") });

		const auto numOfs = computeNumOfs();
		int i = 0;
		auto writeBufLink = [&]()
		{
			writer.writeLink<s32>(oishii::Link{
				oishii::Hook(getSelf()),
				oishii::Hook("Buf" + std::to_string(i++)) });
		};
		auto writeOptBufLink = [&](const auto& b) {
			if (b.mData.empty())
				writer.write<s32>(0);
			else
				writeBufLink();
		};

		writeOptBufLink(mdl->mBufs.pos);
		writeOptBufLink(mdl->mBufs.norm);
		writer.write<s32>(0); // NBT

		for (const auto& c : mdl->mBufs.color)
			writeOptBufLink(c);

		for (const auto& u : mdl->mBufs.uv)
			writeOptBufLink(u);
	}

	struct FormatDecl
	{
		static const char* getNameId() { return "Format"; }
		virtual const oishii::Node& getSelf() const = 0;

		struct Entry
		{
			gx::VertexBufferAttribute attrib;
			u32 cnt = 1;
			u32 data = 0;
			u8 shift = 0;

			void write(oishii::Writer& writer)
			{
				writer.write<u32>(static_cast<u32>(attrib));
				writer.write<u32>(static_cast<u32>(cnt));
				writer.write<u32>(static_cast<u32>(data));
				writer.write<u8>(static_cast<u8>(shift));
				for (int i = 0; i < 3; ++i)
					writer.write<u8>(-1);
			}
		};

		void write(oishii::Writer& writer) const
		{
			// Positions
			if (!mdl->mBufs.pos.mData.empty())
			{
				const auto& q = mdl->mBufs.pos.mQuant;
				Entry{
					gx::VertexBufferAttribute::Position,
					static_cast<u32>(q.comp.position),
					static_cast<u32>(q.type.generic),
					q.divisor
				}.write(writer);
			}
			// Normals
			if (!mdl->mBufs.norm.mData.empty())
			{
				const auto& q = mdl->mBufs.norm.mQuant;
				Entry{
					gx::VertexBufferAttribute::Normal,
					static_cast<u32>(q.comp.normal),
					static_cast<u32>(q.type.generic),
					q.divisor
				}.write(writer);
			}
			// Colors
			{
				int i = 0;
				for (const auto& buf : mdl->mBufs.color)
				{
					if (!buf.mData.empty())
					{
						const auto& q = buf.mQuant;
						Entry{
							gx::VertexBufferAttribute((int)gx::VertexBufferAttribute::Color0 + i),
							static_cast<u32>(q.comp.color),
							static_cast<u32>(q.type.color),
							q.divisor
						}.write(writer);
					}
					++i;
				}
			}
			// UVs
			{
				int i = 0;
				for (const auto& buf : mdl->mBufs.uv)
				{
					if (!buf.mData.empty())
					{
						const auto& q = buf.mQuant;
						Entry{
							gx::VertexBufferAttribute((int)gx::VertexBufferAttribute::TexCoord0 + i),
							static_cast<u32>(q.comp.texcoord),
							static_cast<u32>(q.type.generic),
							q.divisor
						}.write(writer);
					}
					++i;
				}
			}
			Entry{ gx::VertexBufferAttribute::Terminate }.write(writer);
		}
		std::vector<linker_ptr<oishii::Node>> gatherChildren() const
		{
			return {};
		}

		J3DModel* mdl;
	};

	template<typename T>
	struct VertexAttribBuf : public oishii::Node
	{
		VertexAttribBuf(const J3DModel& m, const std::string& id, const T& data, bool linkerOwned = true)
			: Node(id), mdl(m), mData(data)
		{
			transferOwnershipToLinker(linkerOwned);
			getLinkingRestriction().options |= oishii::LinkingRestriction::Leaf;
		}

		eResult write(oishii::Writer& writer) const noexcept
		{
			mData.writeData(writer);
			return eResult::Success;
		}

		const J3DModel& mdl;
		const T& mData;
	};

	std::vector<linker_ptr<oishii::Node>> gatherChildren() const
	{
		std::vector<linker_ptr<oishii::Node>> tmp;

		if (!mdl)
			return tmp;

		auto fmt = new LinkNode<FormatDecl>();
		fmt->mdl = mdl;
		tmp.emplace_back(fmt);

		int i = 0;

		auto push_buf = [&](auto& buf) {
			auto b = new VertexAttribBuf(*mdl, "Buf" + std::to_string(i++), buf);
			b->getLinkingRestriction().alignment = 32;
			tmp.emplace_back(b);
		};

		// Positions
		if (!mdl->mBufs.pos.mData.empty())
		{
			push_buf(mdl->mBufs.pos);
			// tmp.emplace_back(new VertexAttribBuf(*mdl, "Buf" + std::to_string(i++), mdl->mBufs.pos));
		}
		// Normals
		if (!mdl->mBufs.norm.mData.empty())
			push_buf(mdl->mBufs.norm);
		// Colors
		for (auto& c : mdl->mBufs.color)
			if (!c.mData.empty())
				push_buf(c);
		// UV
		for (auto& c : mdl->mBufs.uv)
			if (!c.mData.empty())
				push_buf(c);


		return tmp;
	}

	J3DModel* mdl = nullptr;
};

struct EVP1Node
{
	static const char* getNameId() { return "EVP1 EnVeloPe"; }
	virtual const oishii::Node& getSelf() const = 0;

	void write(oishii::Writer& writer) const
	{
		writer.write<u32, oishii::EndianSelect::Big>('EVP1');
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("VTX1"/*getSelf(), oishii::Hook::EndOfChildren*/) });

		writer.write<u16>(envelopesToWrite.size());
		writer.write<u16>(-1);
		// ofsMatrixSize, ofsMatrixIndex, ofsMatrixWeight, ofsMatrixInvBind
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("MatrixSizeTable")
		});
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("MatrixIndexTable")
		});
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("MatrixWeightTable")
		});
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook("MatrixInvBindTable")
		});
	}
	struct SimpleEvpNode
	{
		virtual const oishii::Node& getSelf() const = 0;
		std::vector<linker_ptr<oishii::Node>> gatherChildren() const { return{}; }

		SimpleEvpNode(const std::vector<DrawMatrix>& from, const std::vector<int>& toWrite, const std::vector<float>& weightPool)
			: mFrom(from), mToWrite(toWrite), mWeightPool(weightPool)
		{}
		const std::vector<DrawMatrix>& mFrom;
		const std::vector<int>& mToWrite;
		const std::vector<float>& mWeightPool;
	};
	struct MatrixSizeTable : public SimpleEvpNode
	{
		static const char* getNameId() { return "MatrixSizeTable"; }
		MatrixSizeTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{}
		void write(oishii::Writer& writer) const
		{
			for (int i : mToWrite)
				writer.write<u8>(mFrom[i].mWeights.size());
		}
	};
	struct MatrixIndexTable : public SimpleEvpNode
	{
		static const char* getNameId() { return "MatrixIndexTable"; }
		MatrixIndexTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{}
		void write(oishii::Writer& writer) const
		{
			for (int i : mToWrite)
				writer.write<u8>(i);
		}
	};
	struct MatrixWeightTable : public SimpleEvpNode
	{
		static const char* getNameId() { return "MatrixWeightTable"; }
		MatrixWeightTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{}
		void write(oishii::Writer& writer) const
		{
			for (f32 f: mWeightPool)
				writer.write<f32>(f);
		}
	};
	struct MatrixInvBindTable : public SimpleEvpNode
	{
		static const char* getNameId() { return "MatrixInvBindTable"; }
		MatrixInvBindTable(const EVP1Node& node)
			: SimpleEvpNode(node.mdl.mDrawMatrices, node.envelopesToWrite, node.weightPool)
		{}
		void write(oishii::Writer& writer) const
		{
			// TODO
		}
	};
	std::vector<linker_ptr<oishii::Node>> gatherChildren() const
	{

		return {
			// Matrix size table
			(new LinkNode<MatrixSizeTable>(*this))->asLeaf(),
			// Matrix index table
			(new LinkNode<MatrixIndexTable>(*this))->asLeaf(),
			// matrix weight table
			(new LinkNode<MatrixWeightTable>(*this))->asLeaf(),
			// matrix inv bind table
			(new LinkNode<MatrixInvBindTable>(*this))->asLeaf()
		};
	}

	EVP1Node(J3DModel& jmdl)
		: mdl(jmdl)
	{
		for (int i = 0; i < mdl.mDrawMatrices.size(); ++i)
		{
			if (mdl.mDrawMatrices[i].mWeights.size() <= 1)
			{
				assert(mdl.mDrawMatrices[i].mWeights[0].weight == 1.0f);
			}
			else
			{
				envelopesToWrite.push_back(i);
				for (const auto& it : mdl.mDrawMatrices[i].mWeights)
				{
					if (std::find(weightPool.begin(), weightPool.end(), it.weight) == weightPool.end())
						weightPool.push_back(it.weight);
				}
			}
		}
	}
	std::vector<f32> weightPool;
	std::vector<int> envelopesToWrite;
	J3DModel& mdl;
};
struct BMDFile
{
	static const char* getNameId() { return "JSystem Binary Model Data"; }

	virtual const oishii::Node& getSelf() const = 0;

	void write(oishii::Writer& writer) const
	{
		// Hack
		writer.write<u32, oishii::EndianSelect::Big>('J3D2');
		writer.write<u32, oishii::EndianSelect::Big>(bBDL ? 'bdl4' : 'bmd3');

		// Filesize
		writer.writeLink<s32>(oishii::Link{
			oishii::Hook(getSelf()),
			oishii::Hook(getSelf(), oishii::Hook::EndOfChildren) });

		// 8 sections
		writer.write<u32>(bBDL ? 9 : 8);

		// SubVeRsion
		writer.write<u32, oishii::EndianSelect::Big>('SVR3');
		for (int i = 0; i < 3; ++i)
			writer.write<u32>(-1);
	}
	std::vector<linker_ptr<oishii::Node>> gatherChildren() const
	{
		auto* inf1 = new LinkNode<INF1Node>();
		inf1->getLinkingRestriction().alignment = 32;
		inf1->mdl = &mCollection->mModel;

		auto* vtx1 = new LinkNode<VTX1Node>();
		vtx1->getLinkingRestriction().alignment = 32;
		vtx1->mdl = &mCollection->mModel;

		// evp1
		auto* evp1 = new LinkNode<EVP1Node>(mCollection->mModel);
		evp1->getLinkingRestriction().alignment = 32;
		// drw1
		// jnt1
		// shp1
		// mat3
		// tex1

		return { inf1, vtx1, evp1 };
	}

	J3DCollection* mCollection;
	bool bBDL = true;
	bool bMimic = true;
};

void exportBMD(oishii::Writer& writer, J3DCollection& collection)
{
	oishii::Linker linker;

	LinkNode<BMDFile> bmd(false);
	bmd.bBDL = collection.bdl;
	bmd.bMimic = true;
	bmd.mCollection = &collection;

	linker.gather(bmd, "");
	linker.write(writer);
}

bool BMDExporter::write(oishii::Writer& writer, pl::FileState& state)
{
	exportBMD(writer, static_cast<J3DCollection&>(state));

	return true;
}

} // namespace libcube::jsystem
