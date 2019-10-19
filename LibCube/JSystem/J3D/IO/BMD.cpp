/*!
 * @file
 * @brief FIXME: Document
 */

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include "BMD.hpp"
#include "../Model.hpp"
#include <map>
#include <type_traits>
#include <algorithm>
#include <tuple>

namespace libcube::jsystem {

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

#pragma region INF1
struct SceneGraph
{
	static constexpr const char name[] = "Scenegraph";

	enum class ByteCodeOp : s16
	{
		Terminate,
		Open,
		Close,

		Joint = 0x10,
		Material,
		Shape,

		Unitialized = -1
	};

	static_assert(sizeof(ByteCodeOp) == 2, "Invalid enum size.");

	struct ByteCodeCmd
	{
		ByteCodeOp op;
		s16 idx;

		ByteCodeCmd(oishii::BinaryReader& reader)
		{
			transfer(reader);
		}
		ByteCodeCmd()
			: op(ByteCodeOp::Unitialized), idx(-1)
		{}

		template<typename T>
		void transfer(T& stream)
		{
			stream.transfer<ByteCodeOp>(op);
			stream.transfer<s16>(idx);
		}
	};

	static void onRead(oishii::BinaryReader& reader, BMDImporter::BMDOutputContext& ctx)
	{
		// FIXME: Algorithm can be significantly improved

		u16 mat, joint = 0;
		auto lastType = ByteCodeOp::Unitialized;

		std::vector<ByteCodeOp> hierarchy_stack;
		std::vector<u16> joint_stack;

		for (ByteCodeCmd cmd(reader); cmd.op != ByteCodeOp::Terminate; cmd.transfer(reader))
		{
			switch (cmd.op)
			{
			case ByteCodeOp::Terminate:
				return;
			case ByteCodeOp::Open:
				if (lastType == ByteCodeOp::Joint)
					joint_stack.push_back(joint);
				hierarchy_stack.emplace_back(lastType);
				break;
			case ByteCodeOp::Close:
				if (hierarchy_stack.back() == ByteCodeOp::Joint)
					joint_stack.pop_back();
				hierarchy_stack.pop_back();
				break;
			case ByteCodeOp::Joint: {
				const auto newId = cmd.idx;

				if (!joint_stack.empty())
				{
					ctx.mdl.mJoints[ctx.jointIdLut[joint_stack.back()]].children.emplace_back(ctx.mdl.mJoints[ctx.jointIdLut[newId]].id);
					ctx.mdl.mJoints[ctx.jointIdLut[newId]].parent = ctx.mdl.mJoints[ctx.jointIdLut[joint_stack.back()]].id;
				}
				joint = newId;
				break;
			}
			case ByteCodeOp::Material:
				mat = cmd.idx;
				break;
			case ByteCodeOp::Shape:
				ctx.mdl.mJoints[ctx.jointIdLut[joint]].displays.emplace_back(ctx.mdl.mMaterials[mat].id, "Shapes TODO");
				break;
			}

			if (cmd.op != ByteCodeOp::Open && cmd.op != ByteCodeOp::Close)
				lastType = cmd.op;
		}
	}
};
#pragma endregion

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

		u32 flag = reader.read<u32>();

		// TODO -- Use these for validation
		u32 nPacket = reader.read<u32>();
		u32 nVertex = reader.read<u32>();

		ctx.mdl.info.mScalingRule = static_cast<J3DModel::Information::ScalingRule>(flag & 0xf);

		reader.dispatch<SceneGraph, oishii::Indirection<0, s32, oishii::Whence::At>>(ctx, g.start);
	}
}
#pragma endregion

#pragma region EVP1/DRW1
void BMDImporter::readDrawMatrices(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{

	// We infer DRW1 -- one bone -> singlebound, else create envelope
	std::vector<J3DModel::DrawMatrix> envelopes;

	// First read our envelope data
	if (enterSection(reader, 'EVP1'))
	{
		ScopedSection g(reader, "Envelopes");

		u16 size = reader.read<u16>();
		envelopes.resize(size);
		reader.read<u16>();

		const auto[ofsMatrixSize, ofsMatrixIndex, ofsMatrixWeight, ofsMatrixInvBind] = reader.readX<s32, 4>();

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
			// TODO: Read inverse bind matrices
		}
	}

	// Now construct vertex draw matrices.
	if (enterSection(reader, 'DRW1'))
	{
		ScopedSection g(reader, "Vertex Draw Matrix");

		ctx.mdl.mDrawMatrices.clear();
		ctx.mdl.mDrawMatrices.resize(reader.read<u16>());
		reader.read<u16>();

		const auto[ofsPartialWeighting, ofsIndex] = reader.readX<s32, 2>();

		reader.seekSet(g.start);

		int i = 0;
		for (auto& mtx : ctx.mdl.mDrawMatrices)
		{
			ScopedInc inc(i);

			const auto multipleInfluences = reader.peekAt<u8>(ofsPartialWeighting + i);
			const auto index = reader.peekAt<u16>(ofsIndex + i * 2);

			mtx = multipleInfluences ? envelopes[index] : J3DModel::DrawMatrix{ std::vector<J3DModel::DrawMatrix::MatrixWeight>{index} };
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

		const auto[ofsJointData, ofsRemapTable, ofsStringTable] = reader.readX<s32, 3>();
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
			DebugReport("Joint IDS will be remapped on save and incompatible with animations.\n");


		// FIXME: unnecessary allocation of a vector.
		reader.seekSet(ofsStringTable + g.start);
		const auto nameTable = readNameTable(reader);

		for (int i = 0; i < size; ++i)
		{
			auto& joint = ctx.mdl.mJoints[i];
			reader.seekSet(g.start + ofsJointData + ctx.jointIdLut[i] * 0x40);
			joint.id = nameTable[i];
			const u16 flag = reader.read<u16>();
			joint.flag = flag & 0xf;
			joint.bbMtxType = static_cast<J3DModel::Joint::MatrixType>(flag >> 4);
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

		TRaw raw()
		{
			if (!ofs)
				throw "Invalid read.";

			return reader.getAt<TRaw>(ofs);
		}
		template<typename T>
		T as()
		{
			return static_cast<T>(raw());
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
		c.compRight= static_cast<gx::Comparison>(reader.read<u8>());
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
            gx::AttenuationFunction::Spotlight};
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
struct io_wrapper<J3DModel::Material::NBTScale>
{
	static void onRead(oishii::BinaryReader& reader, J3DModel::Material::NBTScale& c)
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
struct io_wrapper<J3DModel::Material::TexMatrix>
{
	static void onRead(oishii::BinaryReader& reader, J3DModel::Material::TexMatrix& c)
	{
		c.projection = static_cast<gx::TexGenType>(reader.read<u8>());
		assert(c.projection == gx::TexGenType::Matrix2x4 || c.projection == gx::TexGenType::Matrix3x4);

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

void readMatEntry(J3DModel::Material& mat, MatLoader& loader, oishii::BinaryReader& reader)
{
	oishii::DebugExpectSized dbg(reader, 332);
	oishii::BinaryReader::ScopedRegion g(reader, "Material");

	assert(reader.tell() % 4 == 0);
	mat.flag				= reader.read<u8>();
	mat.cullMode			= loader.indexed<u8>(MatSec::CullModeInfo).as<gx::CullMode>();
	mat.info.nColorChannel	= loader.indexed<u8>(MatSec::NumTexGens).raw();
	mat.info.nTexGen		= loader.indexed<u8>(MatSec::NumTexGens).raw();
	mat.info.nTevStage		= loader.indexed<u8>(MatSec::NumTevStages).raw();
	mat.earlyZComparison	= loader.indexed<u8>(MatSec::ZCompareInfo).as<bool>();
	loader.indexedContained<u8>(mat.ZMode, MatSec::ZModeInfo, 4);
	mat.dither				= loader.indexed<u8>(MatSec::DitherInfo).as<bool>();

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
		J3DModel::array_vector<TevOrder, 16> tevOrderInfos;
		loader.indexedContainer<u16>(tevOrderInfos, MatSec::TevOrderInfo, 4);


		loader.indexedContainer<u16>(mat.tevColors, MatSec::TevColors, 4);

		// FIXME: Directly read into material
		J3DModel::array_vector<gx::TevStage, 16> tevStageInfos;
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
		J3DModel::array_vector<SwapSel, 16> swapSels;
		loader.indexedContainer<u16>(swapSels, MatSec::TevSwapModeInfo, 4);
		for (int i = 0; i < mat.shader.mStages.size(); ++i)
		{
			mat.shader.mStages[i].texMapSwap = swapSels[i].texSel;
			mat.shader.mStages[i].rasSwap = swapSels[i].colorChanSel;
		}

		// FIXME
		J3DModel::array_vector<gx::Shader::SwapTableEntry, 16> swapTables;
		loader.indexedContainer<u16>(swapTables, MatSec::TevSwapModeTableInfo, 4);
		assert(swapTables.size() == 4);
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

		const auto[ofsMatData, ofsRemapTable, ofsStringTable] = reader.readX<s32, 3>();
		MatLoader loader { reader.readX<s32, static_cast<u32>(MatSec::Max)>(), g.start, reader };

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
			mat.id = nameTable[i];

			readMatEntry(mat, loader, reader);
		}
	}
}
#pragma endregion


#pragma region VTX1
void BMDImporter::readVertexBuffers(oishii::BinaryReader& reader, BMDOutputContext& ctx) noexcept
{
	if (enterSection(reader, 'VTX1'))
	{
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
				(gen_data == gx::VertexBufferType::Generic::u16 || gen_data == gx::VertexBufferType::Generic::u16) ? 2 :
				1;
			const auto shift = reader.read<u8>();
			reader.seek(3);

			assert(0 <= shift && shift <= 31);

			auto estride = 0;
			// FIXME: type punning
			void* buf = nullptr;

			J3DModel::VBufferKind bufkind = J3DModel::VBufferKind::undefined;

			switch (type)
			{
			case gx::VertexBufferAttribute::Position:
				buf = &ctx.mdl.mBufs.pos;
				bufkind = J3DModel::VBufferKind::position;
				ctx.mdl.mBufs.pos.mQuant = J3DModel::VQuantization(
					gx::VertexComponentCount(static_cast<gx::VertexComponentCount::Position>(comp)),
					gx::VertexBufferType(gen_data),
					gen_data != gx::VertexBufferType::Generic::f32 ? shift : 0,
					(comp + 2) * gen_comp_size);
				estride = ctx.mdl.mBufs.pos.mQuant.stride;
				break;
			case gx::VertexBufferAttribute::Normal:
				buf = &ctx.mdl.mBufs.norm;
				bufkind = J3DModel::VBufferKind::normal;
				ctx.mdl.mBufs.norm.mQuant = J3DModel::VQuantization(
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
				bufkind = J3DModel::VBufferKind::color;
				clr.mQuant = J3DModel::VQuantization(
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
					}(static_cast<gx::VertexBufferType::Color>(data))
				);
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
				bufkind = J3DModel::VBufferKind::textureCoordinate;
				uv.mQuant = J3DModel::VQuantization(
					gx::VertexComponentCount(static_cast<gx::VertexComponentCount::TextureCoordinate>(comp)),
					gx::VertexBufferType(gen_data),
					gen_data != gx::VertexBufferType::Generic::f32 ? shift : 0,
					(comp + 1) * gen_comp_size
				);
				estride = ctx.mdl.mBufs.pos.mQuant.stride;
				break;
			}
			default:
				assert(false);
			}

			assert(estride);
			assert(bufkind != J3DModel::VBufferKind::undefined);

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
				oishii::Jump g_bufdata(reader, ofs);
				size_t size = 0;
				
				bool found = false;
				for (int i = idx + 1; !size && i < ofsData.size(); ++i)
					if (ofsData[i])
						size = static_cast<size_t>(ofsData[i]) - ofs;
				
				if (!size)
					size = g.size - ofs;

				assert(size > 0);
				const auto ensize = (size + estride) / estride;

				switch (bufkind)
				{
				case J3DModel::VBufferKind::position: {
					auto pos = reinterpret_cast<decltype(ctx.mdl.mBufs.pos)*>(buf);

					pos->mData.resize(ensize);
					for (int i = 0; reader.tell() < size; ++i)
						pos->readBufferEntryGeneric(reader, pos->mData[i]);
				}
				case J3DModel::VBufferKind::normal: {
					auto nrm = reinterpret_cast<decltype(ctx.mdl.mBufs.norm)*>(buf);

					nrm->mData.resize(ensize);
					for (int i = 0; reader.tell() < size; ++i)
						nrm->readBufferEntryGeneric(reader, nrm->mData[i]);
				}
				case J3DModel::VBufferKind::color: {
					auto clr = reinterpret_cast<decltype(ctx.mdl.mBufs.color)::value_type*>(buf);

					clr->mData.resize(ensize);
					for (int i = 0; reader.tell() < size; ++i)
						clr->readBufferEntryColor(reader, clr->mData[i]);
				}
				case J3DModel::VBufferKind::textureCoordinate: {
					auto uv = reinterpret_cast<decltype(ctx.mdl.mBufs.uv)::value_type*>(buf);

					uv->mData.resize(ensize);
					for (int i = 0; reader.tell() < size; ++i)
						uv->readBufferEntryGeneric(reader, uv->mData[i]);
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

	// Read EVP1 and DRW1
	readDrawMatrices(reader, ctx);

	// Read JNT1
	readJoints(reader, ctx);

	// Read SHP1

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
	readBMD(reader, BMDOutputContext{ static_cast<J3DCollection&>(state).mModel });
	return !error;
}

} // namespace libcube::jsystem
