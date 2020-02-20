#define GLM_ENABLE_EXPERIMENTAL
#include <ThirdParty/glm/gtx/matrix_decompose.hpp>

#include <LibJ/J3D/Binary/BMD/Sections.hpp>
#include <oishii/v2/writer/binary_writer.hxx>

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
	//	template<typename C>
	//	static void onRead(oishii::BinaryReader&, C&)
	//	{
	//		DebugReport("Unimplemented IO wrapper called.\n");
	//	}
	//	template<typename C>
	//	static void onWrite(oishii::v2::Writer&, const C&)
	//	{
	//		DebugReport("Unimplemented IO writer wrapper called.\n");
	//	}

	template<typename C = T, typename _ = typename std::enable_if<sizeof(T) <= 4 && std::is_integral<T>::value>::type>
	static void onRead(oishii::BinaryReader& reader, T& c)
	{
		switch (sizeof(T))
		{
		case 1:
			c = static_cast<T>(reader.read<u8>());
			break;
		case 2:
			c = static_cast<T>(reader.read<u16>());
			break;
		case 4:
			c = static_cast<T>(reader.read<u32>());
			break;
		default:
			break;
		}
	}
	template<typename C = T, typename _ = typename std::enable_if<sizeof(T) <= 4 && std::is_integral<T>::value>::type>
	static void onWrite(oishii::v2::Writer& writer, const T& c)
	{
		switch (sizeof(T))
		{
		case 1:
			writer.write<u8>(static_cast<u8>(c));
			break;
		case 2:
			writer.write<u16>(static_cast<u16>(c));
			break;
		case 4:
			writer.write<u32>(static_cast<u32>(c));
			break;
		default:
			break;
		}
	}
};

#pragma region IO
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
	static void onWrite(oishii::v2::Writer& writer, const gx::ZMode& in)
	{
		writer.write<u8>(in.compare);
		writer.write<u8>(static_cast<u8>(in.function));
		writer.write<u8>(in.update);
		writer.write<u8>(0xff);
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
	static void onWrite(oishii::v2::Writer& writer, const gx::AlphaComparison& in)
	{
		writer.write<u8>(static_cast<u8>(in.compLeft));
		writer.write<u8>(in.refLeft);
		writer.write<u8>(static_cast<u8>(in.op));
		writer.write<u8>(static_cast<u8>(in.compRight));
		writer.write<u8>(static_cast<u8>(in.refRight));
		for (int i = 0; i < 3; ++i)
			writer.write<u8>(0xff);
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
	static void onWrite(oishii::v2::Writer& writer, const gx::BlendMode& in)
	{
		writer.write<u8>(static_cast<u8>(in.type));
		writer.write<u8>(static_cast<u8>(in.source));
		writer.write<u8>(static_cast<u8>(in.dest));
		writer.write<u8>(static_cast<u8>(in.logic));
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
	static void onWrite(oishii::v2::Writer& writer, const gx::Color& in)
	{
		writer.write<u8>(in.r);
		writer.write<u8>(in.g);
		writer.write<u8>(in.b);
		writer.write<u8>(in.a);
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
		// TODO Data loss?
		constexpr const std::array<gx::AttenuationFunction, 4> cvt = {
			gx::AttenuationFunction::None,
			gx::AttenuationFunction::Specular,
			gx::AttenuationFunction::None2,
			gx::AttenuationFunction::Spotlight };
		out.attenuationFn = cvt[reader.read<u8>()];
		out.Ambient = static_cast<gx::ColorSource>(reader.read<u8>());

		reader.read<u16>();
	}
	static void onWrite(oishii::v2::Writer& writer, const gx::ChannelControl& in)
	{
		writer.write<u8>(in.enabled);
		writer.write<u8>(static_cast<u8>(in.Material));
		writer.write<u8>(static_cast<u8>(in.lightMask));
		writer.write<u8>(static_cast<u8>(in.diffuseFn));
		assert(static_cast<u8>(in.attenuationFn) <= static_cast<u8>(gx::AttenuationFunction::None2));
		constexpr const std::array<u8, 4> cvt {
			1, // spec
			3, // spot
			0, // none
			2 // none
		};
		writer.write<u8>(cvt[static_cast<u8>(in.attenuationFn)]);
		writer.write<u8>(static_cast<u8>(in.Ambient));
		writer.write<u16>(-1);
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
	static void onWrite(oishii::v2::Writer& writer, const gx::ColorS10& in)
	{
		writer.write<s16>(in.r);
		writer.write<s16>(in.g);
		writer.write<s16>(in.b);
		writer.write<s16>(in.a);
	}
};

template<>
struct io_wrapper<NBTScale>
{
	static void onRead(oishii::BinaryReader& reader, NBTScale& c)
	{
		c.enable = static_cast<bool>(reader.read<u8>());
		reader.seek(3);
		c.scale << reader;
	}
	static void onWrite(oishii::v2::Writer& writer, const NBTScale& in)
	{
		writer.write<u8>(in.enable);
		for (int i = 0; i < 3; ++i)
			writer.write<u8>(-1);
		writer.write<f32>(in.scale.x);
		writer.write<f32>(in.scale.y);
		writer.write<f32>(in.scale.z);
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
	static void onWrite(oishii::v2::Writer& writer, const gx::TexCoordGen& in)
	{
		writer.write<u8>(static_cast<u8>(in.func));
		writer.write<u8>(static_cast<u8>(in.sourceParam));
		writer.write<u8>(static_cast<u8>(in.matrix));
		writer.write<u8>(-1);
	}
};
struct J3DMappingMethodDecl
{
	using Method = Material::CommonMappingMethod;
	enum class Class { Standard, Basic, Old };

	Method _method;
	Class _class;

	bool operator==(const J3DMappingMethodDecl& rhs) const
	{
		return _method == rhs._method && _class == rhs._class;
	}
};

static std::array<J3DMappingMethodDecl, 12> J3DMappingMethods 
{
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::Standard, J3DMappingMethodDecl::Class::Standard }, // 0
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::EnvironmentMapping, J3DMappingMethodDecl::Class::Basic }, // 1
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::ProjectionMapping, J3DMappingMethodDecl::Class::Basic }, // 2
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::ViewProjectionMapping, J3DMappingMethodDecl::Class::Basic }, // 3
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::Standard, J3DMappingMethodDecl::Class::Standard }, // 4 Not supported
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::Standard, J3DMappingMethodDecl::Class::Standard }, // 5 Not supported
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::EnvironmentMapping, J3DMappingMethodDecl::Class::Old }, // 6
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::EnvironmentMapping, J3DMappingMethodDecl::Class::Standard }, // 7
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::ProjectionMapping, J3DMappingMethodDecl::Class::Standard }, // 8
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::ViewProjectionMapping, J3DMappingMethodDecl::Class::Standard }, // 9
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::ManualEnvironmentMapping, J3DMappingMethodDecl::Class::Old }, // a
	J3DMappingMethodDecl{ J3DMappingMethodDecl::Method::ManualEnvironmentMapping, J3DMappingMethodDecl::Class::Standard }  // b
};

template<>
struct io_wrapper<Material::TexMatrix>
{
	static void onRead(oishii::BinaryReader& reader, Material::TexMatrix& c)
	{
		c.projection = static_cast<gx::TexGenType>(reader.read<u8>());
		// FIXME:
		//	assert(c.projection == gx::TexGenType::Matrix2x4 || c.projection == gx::TexGenType::Matrix3x4);

		u8 mappingMethod = reader.read<u8>();
		bool maya = mappingMethod & 0x80;
		mappingMethod &= ~0x80;

		c.transformModel = maya ? Material::CommonTransformModel::Maya : Material::CommonTransformModel::Default;

		if (mappingMethod > J3DMappingMethods.size())
		{
			reader.warnAt("Invalid mapping method: Enumeration ends at 0xB.", reader.tell() - 1, reader.tell());
			mappingMethod = 0;
		}
		else if (mappingMethod == 4 || mappingMethod == 5)
		{
			reader.warnAt("This mapping method has yet to be seen in the wild: Enumeration ends at 0xB.", reader.tell() - 1, reader.tell());
		}

		const auto& method_decl = J3DMappingMethods[mappingMethod];
		switch (method_decl._class)
		{
		case J3DMappingMethodDecl::Class::Basic:
			c.option = Material::CommonMappingOption::DontRemapTextureSpace;
			break;
		case J3DMappingMethodDecl::Class::Old:
			c.option = Material::CommonMappingOption::KeepTranslation;
			break;
		default:
			break;
		}

		c.method = method_decl._method;

		reader.read<u16>();

		glm::vec3 origin;
		origin << reader;

		// TODO -- Assert

		c.scale << reader;
		c.rotate = static_cast<f32>(reader.read<s16>()) * 180.f / (f32)0x7FFF;
		reader.read<u16>();
		c.translate << reader;
		for (auto& f : c.effectMatrix)
			f = reader.read<f32>();
	}
	static void onWrite(oishii::v2::Writer& writer, const Material::TexMatrix& in)
	{
		writer.write<u8>(static_cast<u8>(in.projection));

		u8 mappingMethod = 0;
		bool maya = in.transformModel == Material::CommonTransformModel::Maya;

		J3DMappingMethodDecl::Class _class = J3DMappingMethodDecl::Class::Standard;

		switch (in.option)
		{
		case Material::CommonMappingOption::DontRemapTextureSpace:
			_class = J3DMappingMethodDecl::Class::Basic;
			break;
		case Material::CommonMappingOption::KeepTranslation:
			_class = J3DMappingMethodDecl::Class::Old;
			break;
		default:
			break;
		}

		const auto mapIdx = std::find(J3DMappingMethods.begin(), J3DMappingMethods.end(), J3DMappingMethodDecl{
			in.method,
			_class
			});

		if (mapIdx != J3DMappingMethods.end())
			mappingMethod = mapIdx - J3DMappingMethods.begin();

		writer.write<u8>(static_cast<u8>(mappingMethod) | (maya ? 0x80 : 0));
		writer.write<u16>(-1);
		writer.write<f32>(0.5f); // TODO
		writer.write<f32>(0.5f);
		writer.write<f32>(0.5f);
		writer.write<f32>(in.scale.x);
		writer.write<f32>(in.scale.y);
		writer.write<u16>((u16)roundf(static_cast<f32>(in.rotate * (f32)0x7FFF / 180.0f)));
		writer.write<u16>(-1);
		writer.write<f32>(in.translate.x);
		writer.write<f32>(in.translate.y);
		for (const auto f : in.effectMatrix)
			writer.write<f32>(f);
	}
};

struct SwapSel
{
	u8 colorChanSel, texSel;

	bool operator==(const SwapSel& rhs) const noexcept
	{
		return colorChanSel == rhs.colorChanSel && texSel == rhs.texSel;
	}
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
	static void onWrite(oishii::v2::Writer& writer, const SwapSel& in)
	{
		writer.write<u8>(in.colorChanSel);
		writer.write<u8>(in.texSel);
		writer.write<u16>(-1);
	}
};
template<>
struct io_wrapper<gx::SwapTableEntry>
{
	static void onRead(oishii::BinaryReader& reader, gx::SwapTableEntry& c)
	{
		c.r = static_cast<gx::ColorComponent>(reader.read<u8>());
		c.g = static_cast<gx::ColorComponent>(reader.read<u8>());
		c.b = static_cast<gx::ColorComponent>(reader.read<u8>());
		c.a = static_cast<gx::ColorComponent>(reader.read<u8>());
	}
	static void onWrite(oishii::v2::Writer& writer, const gx::SwapTableEntry& in)
	{
		writer.write<u8>(static_cast<u8>(in.r));
		writer.write<u8>(static_cast<u8>(in.g));
		writer.write<u8>(static_cast<u8>(in.b));
		writer.write<u8>(static_cast<u8>(in.a));
	}
};

struct TevOrder
{
	gx::ColorSelChanApi rasOrder;
	u8 texMap, texCoord;

	bool operator==(const TevOrder& rhs) const noexcept
	{
		return rasOrder == rhs.rasOrder && texMap == rhs.texMap && texCoord == rhs.texCoord;
	}
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
	static void onWrite(oishii::v2::Writer& writer, const TevOrder& in)
	{
		writer.write<u8>(in.texCoord);
		writer.write<u8>(in.texMap);
		writer.write<u8>(static_cast<u8>(in.rasOrder));
		writer.write<u8>(-1);
	}
};

template<>
struct io_wrapper<gx::TevStage>
{
	static void onRead(oishii::BinaryReader& reader, gx::TevStage& c)
	{
		const auto unk1 = reader.read<u8>();
		assert(unk1 == 0xff);
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
		assert(unk2 == 0xff);
	}
	static void onWrite(oishii::v2::Writer& writer, const gx::TevStage& in)
	{
		writer.write<u8>(0xff);
		writer.write<u8>(static_cast<u8>(in.colorStage.a));
		writer.write<u8>(static_cast<u8>(in.colorStage.b));
		writer.write<u8>(static_cast<u8>(in.colorStage.c));
		writer.write<u8>(static_cast<u8>(in.colorStage.d));

		writer.write<u8>(static_cast<u8>(in.colorStage.formula));
		writer.write<u8>(static_cast<u8>(in.colorStage.bias));
		writer.write<u8>(static_cast<u8>(in.colorStage.scale));
		writer.write<u8>(static_cast<u8>(in.colorStage.clamp));
		writer.write<u8>(static_cast<u8>(in.colorStage.out));

		writer.write<u8>(static_cast<u8>(in.alphaStage.a));
		writer.write<u8>(static_cast<u8>(in.alphaStage.b));
		writer.write<u8>(static_cast<u8>(in.alphaStage.c));
		writer.write<u8>(static_cast<u8>(in.alphaStage.d));

		writer.write<u8>(static_cast<u8>(in.alphaStage.formula));
		writer.write<u8>(static_cast<u8>(in.alphaStage.bias));
		writer.write<u8>(static_cast<u8>(in.alphaStage.scale));
		writer.write<u8>(static_cast<u8>(in.alphaStage.clamp));
		writer.write<u8>(static_cast<u8>(in.alphaStage.out));
		writer.write<u8>(0xff);
	}
};

template<>
struct io_wrapper<Fog>
{
	static void onRead(oishii::BinaryReader& reader, Fog& f)
	{
		f.type = static_cast<Fog::Type>(reader.read<u8>());
		f.enabled = reader.read<u8>();
		f.center = reader.read<u16>();
		f.startZ = reader.read<f32>();
		f.endZ = reader.read<f32>();
		f.nearZ = reader.read<f32>();
		f.farZ = reader.read<f32>();
		io_wrapper<gx::Color>::onRead(reader, f.color);
		f.rangeAdjTable = reader.readX<u16, 10>();
	}
	static void onWrite(oishii::v2::Writer& writer, const Fog& in)
	{
		writer.write<u8>(static_cast<u8>(in.type));
		writer.write<u8>(in.enabled);
		writer.write<u16>(in.center);
		writer.write<f32>(in.startZ);
		writer.write<f32>(in.endZ);
		writer.write<f32>(in.nearZ);
		writer.write<f32>(in.farZ);
		io_wrapper<gx::Color>::onWrite(writer, in.color);
		for (const auto x : in.rangeAdjTable)
			writer.write<u16>(x);
	}
};
#pragma endregion
template<>
struct io_wrapper<gx::CullMode>
{
	static void onWrite(oishii::v2::Writer& writer, gx::CullMode cm)
	{
		writer.write<u32>(static_cast<u32>(cm));
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
template<>
struct io_wrapper<Material::J3DSamplerData>
{
	static void onRead(oishii::BinaryReader& reader, GCMaterialData::SamplerData& sampler)
	{
		reinterpret_cast<Material::J3DSamplerData&>(sampler).btiId = reader.read<u16>();
	}
	static void onWrite(oishii::v2::Writer& writer, const Material::J3DSamplerData& sampler)
	{
		writer.write<u16>(sampler.btiId);
	}
};
struct Indirect
{	
	bool enabled;
	u8 nIndStage;

	std::array<gx::IndOrder, 4> tevOrder;
	std::array<gx::IndirectMatrix, 3> texMtx;
	std::array<gx::IndirectTextureScalePair, 4> texScale;
	std::array<gx::TevStage::IndirectStage, 16> tevStage;

	Indirect() = default;
	Indirect(const MaterialData& mat)
	{
		enabled = mat.indEnabled;
		nIndStage = mat.info.nIndStage;

		tevOrder = mat.shader.mIndirectOrders;
		for (int i = 0; i < nIndStage; ++i)
			texScale[i] = mat.mIndScales[i];
		for (int i = 0; i < 3 && i < mat.mIndMatrices.size(); ++i)
			texMtx[i] = mat.mIndMatrices[i];
		for (int i = 0; i < mat.shader.mStages.size(); ++i)
			tevStage[i] = mat.shader.mStages[i].indirectStage;
	}

	bool operator==(const Indirect& rhs) const noexcept
	{
		return enabled == rhs.enabled && nIndStage == rhs.nIndStage &&
			tevOrder == rhs.tevOrder && texMtx == rhs.texMtx &&
			texScale == rhs.texScale &&
			tevStage == rhs.tevStage;
	}
};
template<>
struct io_wrapper<Indirect>
{
	static void onRead(oishii::BinaryReader& reader, Indirect& c)
	{
		// reader.warnAt("Start of IndTex block", reader.tell(), reader.tell() + 4);
		c.enabled = reader.read<u8>();
		c.nIndStage = reader.read<u8>();
		reader.read<u16>();

		assert(c.enabled <= 1 && c.nIndStage <= 4);
		assert(!c.enabled || c.nIndStage);

		for (auto& e : c.tevOrder)
		{
			// TODO: Switched? Confirm this
			e.refCoord = reader.read<u8>();
			e.refMap = reader.read<u8>();
			reader.read<u16>();
		}
		for (auto& e : c.texMtx)
		{
			glm::mat2x3 m_raw;

			m_raw[0][0] = reader.read<f32>();
			m_raw[0][1] = reader.read<f32>();
			m_raw[0][2] = reader.read<f32>();
			m_raw[1][0] = reader.read<f32>();
			m_raw[1][1] = reader.read<f32>();
			m_raw[1][2] = reader.read<f32>();
			e.quant = reader.read<u8>();
			reader.seek(3);

			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 translation;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose((glm::mat4)m_raw, scale, rotation, translation, skew, perspective);

			// assert(skew == glm::vec3(0.0f) && perspective == glm::vec4(0.0f));

			e.scale = { scale.x, scale.y };
			e.rotate = glm::degrees(glm::eulerAngles(rotation).z);
			e.trans = { translation.x, translation.y };
		}
		for (auto& e : c.texScale)
		{
			e.U = static_cast<gx::IndirectTextureScalePair::Selection>(reader.read<u8>());
			e.V = static_cast<gx::IndirectTextureScalePair::Selection>(reader.read<u8>());
			reader.seek(2);
		}
		int i = 0;
		for (auto& e : c.tevStage)
		{
			u8 id = reader.read<u8>();
			// assert(id == i || i >= c.nIndStage);
			e.format = static_cast<gx::IndTexFormat>(reader.read<u8>());
			e.bias = static_cast<gx::IndTexBiasSel>(reader.read<u8>());
			e.matrix = static_cast<gx::IndTexMtxID>(reader.read<u8>());
			e.wrapU = static_cast<gx::IndTexWrap>(reader.read<u8>());
			e.wrapV = static_cast<gx::IndTexWrap>(reader.read<u8>());
			e.addPrev = reader.read<u8>();
			e.utcLod = reader.read<u8>();
			e.alpha = static_cast<gx::IndTexAlphaSel>(reader.read<u8>());
			reader.seek(3);

			++i;
		}
	}
	static void onWrite(oishii::v2::Writer& writer, const Indirect& c)
	{
		writer.write<u8>(c.enabled);
		writer.write<u8>(c.nIndStage);
		writer.write<u16>(-1);

		for (const auto& e : c.tevOrder)
		{
			// TODO: switched?
			writer.write<u8>(e.refCoord);
			writer.write<u8>(e.refMap);
			writer.write<u16>(-1);
		}
		for (const auto& e : c.texMtx)
		{
			glm::mat4 out(1.0f);

			// TODO: Are we applying the order here wrong..?
			out = glm::translate(out, { e.trans.x, e.trans.y, 0.0f } );
			out = glm::rotate(out, e.rotate, { 0.0f, 1.0f, 0.0f });
			out = glm::scale(out, { e.scale.x, e.scale.y, 1.0f,  });

			f32 aa = out[0][0];
			f32 ab = out[0][1];
			f32 ac = out[0][2];
			f32 ba = out[1][0];
			f32 bb = out[1][1];
			f32 bc = out[1][2];
			writer.write<f32>(aa);
			writer.write<f32>(ab);
			writer.write<f32>(ac);
			writer.write<f32>(ba);
			writer.write<f32>(bb);
			writer.write<f32>(bc);

			writer.write<u8>(e.quant);
			for (int i= 0; i < 3; ++i)
				writer.write<u8>(-1);
		}
		for (int i = 0; i < c.texScale.size(); ++i)
		{
			writer.write<u8>(static_cast<u8>(c.texScale[i].U));
			writer.write<u8>(static_cast<u8>(c.texScale[i].V));
			writer.write<u16>(-1);
		}
		int i = 0;
		for (const auto& e : c.tevStage)
		{
			writer.write<u8>(i < c.nIndStage ? i : 0);
			writer.write<u8>(static_cast<u8>(e.format));
			writer.write<u8>(static_cast<u8>(e.bias));
			writer.write<u8>(static_cast<u8>(e.matrix));
			writer.write<u8>(static_cast<u8>(e.wrapU));
			writer.write<u8>(static_cast<u8>(e.wrapV));
			writer.write<u8>(static_cast<u8>(e.addPrev));
			writer.write<u8>(static_cast<u8>(e.utcLod));
			writer.write<u8>(static_cast<u8>(e.alpha));
			for (int j = 0; j < 3; ++j)
				writer.write<u8>(-1);
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

	if (loader.mSections[(u32)MatSec::IndirectTexturingInfo] != ofsStringTable)
	{
		Indirect ind{};
		loader.indexedContained<Indirect>(ind, MatSec::IndirectTexturingInfo, 0x138, idx);

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


void readMAT3(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
	if (!enterSection(ctx, 'MAT3')) return;
	
	ScopedSection g(reader, "Materials");

	u16 size = reader.read<u16>();

	for (u32 i = 0; i < size; ++i)
		ctx.mdl.getMaterials().push(std::make_unique<Material>());
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
		Material& mat = *ctx.mdl.getMaterials().at<Material>(i);
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

protected:
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

template<typename TIdx, typename T, typename TPool>
void write_array_vec(oishii::v2::Writer& writer, const T& vec, const TPool& pool)
{
	for (int i = 0; i < vec.nElements; ++i)
		writer.write<TIdx>(pool.find(vec[i]));
	for (int i = vec.nElements; i < vec.max_size(); ++i)
		writer.write<TIdx>(-1);
}

struct MAT3Node : public oishii::v2::Node
{
	template<typename T, MatSec s>
	struct Section : MCompressableVector<T, 4, 0, true>
	{
	};

	struct EntrySection final : public Section<SerializableMaterial, MatSec::Max>
	{
		struct LUTNode final : oishii::v2::LeafNode
		{
			LUTNode(const std::vector<u16>& lut)
				: mLut(lut)
			{
				mId = "MLUT";
			}
			Result write(oishii::v2::Writer& writer) const noexcept override
			{
				for (const auto e : mLut)
					writer.write<u16>(e);

				return {};
			}
			const std::vector<u16>& mLut;
		};
		struct NameTableNode final : oishii::v2::LeafNode
		{
			NameTableNode(const J3DModel& mdl, const std::string& name)
				: mMdl(mdl)
			{
				mId = name;
				getLinkingRestriction().alignment = 4;
			}
			Result write(oishii::v2::Writer& writer) const noexcept override
			{
				std::vector<std::string> names(mMdl.getMaterials().size());
				int i = 0;
				for (int i = 0; i < mMdl.getMaterials().size(); ++i)
					names[i] = mMdl.getMaterials()[i].name;
				writeNameTable(writer, names);
				writer.alignTo(4);

				return {};
			}
			const J3DModel& mMdl;
		};
		std::unique_ptr<oishii::v2::Node> spawnDataEntries(const std::string& name) const noexcept
		{
			return spawnNode(name);
		}
		std::unique_ptr<oishii::v2::Node> spawnIDLookupTable() const noexcept
		{
			return std::make_unique<LUTNode>(mLut);
		}
		std::unique_ptr<oishii::v2::Node> spawnNameTable(const std::string& name) const noexcept
		{
			return std::make_unique<NameTableNode>(mMdl, name);
		}

		EntrySection(const J3DModel& mdl, const MAT3Node& mat3)
			: mMdl(mdl)
		{
			for (int i = 0; i < mMdl.getMaterials().size(); ++i)
				mLut.push_back(append(SerializableMaterial{ mat3, i }));
		}

		const J3DModel& mMdl;
		std::vector<u16> mLut;
	};


	const J3DModel& mMdl;
	bool hasIndirect = false;
	EntrySection mEntries;

	
	MCompressableVector<Indirect, 4, 0, false> mIndirect;
	Section<gx::CullMode, MatSec::CullModeInfo> mCullModes;
	Section<gx::Color, MatSec::MaterialColors> mMaterialColors;
	Section<u8, MatSec::NumColorChannels> mNumColorChannels;
	Section<gx::ChannelControl, MatSec::ColorChannelInfo> mChannelControls;
	Section<gx::Color, MatSec::AmbientColors> mAmbColors;
	Section<gx::Color, MatSec::LightInfo> mLightColors;
	Section<u8, MatSec::NumTexGens> mNumTexGens;
	Section<gx::TexCoordGen, MatSec::TexGenInfo> mTexGens;
	Section<gx::TexCoordGen, MatSec::PostTexGenInfo> mPostTexGens; // TODO...
	Section<Material::TexMatrix, MatSec::TexMatrixInfo> mTexMatrices;
	Section<Material::TexMatrix, MatSec::PostTexMatrixInfo> mPostTexMatrices;
	Section<Material::J3DSamplerData, MatSec::TextureRemapTable> mTextureTable;
	Section<TevOrder, MatSec::TevOrderInfo> mOrders;
	Section<gx::ColorS10, MatSec::TevColors> mTevColors;
	Section<gx::Color, MatSec::TevKonstColors> mKonstColors;

	Section<u8, MatSec::NumTevStages> mNumTevStages;
	Section<gx::TevStage, MatSec::TevStageInfo> mTevStages;
	Section<SwapSel, MatSec::TevSwapModeInfo> mSwapModes;
	Section<gx::SwapTableEntry, MatSec::TevSwapModeTableInfo> mSwapTables;
	Section<Fog, MatSec::FogInfo> mFogs;

	Section<gx::AlphaComparison, MatSec::AlphaCompareInfo> mAlphaComparisons;
	Section<gx::BlendMode, MatSec::BlendModeInfo> mBlendModes;
	Section<gx::ZMode, MatSec::ZModeInfo> mZModes;
	Section<u8, MatSec::ZCompareInfo> mZCompLocs;
	Section<u8, MatSec::DitherInfo> mDithers;
	Section<NBTScale, MatSec::NBTScaleInfo> mNBTScales;

	gx::TexCoordGen postTexGen(const gx::TexCoordGen& gen) const noexcept
	{
		return gx::TexCoordGen{ // gen.id,
			gen.func, gen.sourceParam, static_cast<gx::TexMatrix>(gen.postMatrix),
					false, gen.postMatrix };
	}

	MAT3Node(const BMDExportContext& ctx)
		: mMdl(ctx.mdl), mEntries(ctx.mdl, *this)
	{
		mId = "MAT3";
		getLinkingRestriction().alignment = 32;

		for (int i = 0; i < ctx.mdl.getMaterials().size(); ++i)
		{
			const auto& mat = ctx.mdl.getMaterials()[i];
			if (mat.indEnabled)
				hasIndirect = true;

			// mCullModes.append(mat.cullMode);
			for (int i = 0; i < mat.chanData.size(); ++i)
				mMaterialColors.append(mat.chanData[i].matColor);
			mNumColorChannels.append(mat.info.nColorChan);
			for (int i = 0; i < mat.colorChanControls.size(); ++i)
				mChannelControls.append(mat.colorChanControls[i]);
			for (int i = 0; i < mat.chanData.size(); ++i)
				mAmbColors.append(mat.chanData[i].ambColor);
			for (int i = 0; i < mat.lightColors.size(); ++i)
				mLightColors.append(mat.lightColors[i]);
			mNumTexGens.append(mat.info.nTexGen);
			for (int i = 0; i < mat.texGens.size(); ++i)
				mTexGens.append(mat.texGens[i]);

			for (int i = 0; i < mat.texGens.size(); ++i)
				if (mat.texGens[i].postMatrix != gx::PostTexMatrix::Identity)
					mPostTexGens.append(postTexGen(mat.texGens[i]));

			for (int i = 0; i < mat.texMatrices.size(); ++i)
				mTexMatrices.append(*mat.texMatrices[i].get());
			for (int i = 0; i < mat.postTexMatrices.size(); ++i)
				mPostTexMatrices.append(mat.postTexMatrices[i]);
			for (int i = 0; i < mat.samplers.size(); ++i)
				mTextureTable.append(static_cast<const Material::J3DSamplerData*>(mat.samplers[i].get())->btiId);
			for (const auto& stage : mat.shader.mStages)
				mOrders.append(TevOrder{ stage.rasOrder, stage.texMap, stage.texCoord });
			for (int i = 0; i < mat.tevColors.size(); ++i)
				mTevColors.append(mat.tevColors[i]);
			for (int i = 0; i < mat.tevKonstColors.size(); ++i)
				mKonstColors.append(mat.tevKonstColors[i]);
			mNumTevStages.append(mat.info.nTevStage);
			for (const auto& stage : mat.shader.mStages)
			{
				mTevStages.append(stage);
				mSwapModes.append(SwapSel{stage.rasSwap, stage.texMapSwap});
			}
			for (const auto& table : mat.shader.mSwapTable)
				mSwapTables.append(table);
			mFogs.append(mat.fogInfo);
			mAlphaComparisons.append(mat.alphaCompare);
			mBlendModes.append(mat.blendMode);
			mZModes.append(mat.zMode);
			// mZCompLocs.append(mat.earlyZComparison);
			// mDithers.append(mat.dither);
			mNBTScales.append(mat.nbtScale);
		}
		mCullModes.append(gx::CullMode::Back);
		mCullModes.append(gx::CullMode::Front);
		mCullModes.append(gx::CullMode::None);
		mZCompLocs.append(false);
		mZCompLocs.append(true);
		mDithers.append(false);
		mDithers.append(true);
		// Has to be use compressed entries
		// if (hasIndirect)
			for (int i = 0; i < mEntries.getNumEntries(); ++i)
				mIndirect.append(Indirect(mMdl.getMaterials()[mEntries.getEntry(i).mIdx]));
	}

	const std::array<std::string, (u32)MatSec::Max + 3 + 1> sec_names = {
		"Material Data",
		"MLUT",
		"Name Table",
		"Indirect",
		"Cull Modes",
		"Material Colors",
		"Channel Control Count",
		"Channel Controls",
		"Ambient Colors",
		"Light Colors",
		"Tex Gen Count",
		"Tex Gens",
		"Post Tex Gens",
		"Tex Matrices",
		"Post Tex Matrices",
		"Texture ID Table",
		"TEV Orders",
		"TEV Colors",
		"TEV Konstant Colors",
		"TEV Stage Count",
		"TEV Stages",
		"TEV Swap Orders",
		"TEV Swap Tables",
		"Fogs",
		"Alpha Comparisons",
		"Blend Modes",
		"Z Modes",
		"Z Comparison Locations",
		"Dithers",
		"NBT Scales",
		"NBT Scales" // For +1
	};
	Result write(oishii::v2::Writer& writer) const noexcept override
	{
		writer.write<u32, oishii::EndianSelect::Big>('MAT3');
		writer.writeLink<s32>({ *this }, { *this, oishii::v2::Hook::EndOfChildren });

		writer.write<u16>(mMdl.getMaterials().size());
		writer.write<u16>(-1);


		auto writeSecLink = [&](const auto& sec, int i)
		{
			if (sec.getNumEntries() == 0)
			{
				// TODO -- dif behavior for first?
				writer.writeLink<s32>({ *this }, { sec_names[i + 1] });
			}
			else
			{
				writer.writeLink<s32>({ *this }, { sec_names[i] });
			}
		};
		auto writeSecLinkS = [&](const std::string& lnk)
		{
			writer.writeLink<s32>({ *this }, { lnk });
		};

		writeSecLinkS("Material Data");
		writeSecLinkS("MLUT");
		writeSecLinkS("Name Table");
		writeSecLink(mIndirect, 3);



		writeSecLink(mCullModes, 4);
		writeSecLink(mMaterialColors, 5);
		writeSecLink(mNumColorChannels, 6);
		writeSecLink(mChannelControls, 7);
		writeSecLink(mAmbColors, 8);
		writeSecLink(mLightColors, 9);
		writeSecLink(mNumTexGens, 10);
		writeSecLink(mTexGens, 11);
		writeSecLink(mPostTexGens, 12);
		writeSecLink(mTexMatrices, 13);
		writeSecLink(mPostTexMatrices, 14);
		writeSecLink(mTextureTable, 15);
		writeSecLink(mOrders, 16);
		writeSecLink(mTevColors, 17);
		writeSecLink(mKonstColors, 18);
		writeSecLink(mNumTevStages, 19);
		writeSecLink(mTevStages, 20);
		writeSecLink(mSwapModes, 21);
		writeSecLink(mSwapTables, 22);
		writeSecLink(mFogs, 23);
		writeSecLink(mAlphaComparisons, 24);
		writeSecLink(mBlendModes, 25);
		writeSecLink(mZModes, 26);
		writeSecLink(mZCompLocs, 27);
		writeSecLink(mDithers, 28);
		writeSecLink(mNBTScales, 29);

		return {};
	}

	Result gatherChildren(NodeDelegate& d) const noexcept override
	{
		d.addNode(mEntries.spawnDataEntries("Material Data"));
		d.addNode(mEntries.spawnIDLookupTable());
		d.addNode(mEntries.spawnNameTable("Name Table"));
		// if (hasIndirect)
			d.addNode(mIndirect.spawnNode("Indirect"));
		d.addNode(mCullModes.spawnNode("Cull Modes"));
		d.addNode(mMaterialColors.spawnNode("Material Colors"));
		d.addNode(mNumColorChannels.spawnNode("Channel Control Count"));
		d.addNode(mChannelControls.spawnNode("Channel Controls"));
		d.addNode(mAmbColors.spawnNode("Ambient Colors"));
		d.addNode(mLightColors.spawnNode("Light Colors"));
		d.addNode(mNumTexGens.spawnNode("Tex Gen Count"));
		d.addNode(mTexGens.spawnNode("Tex Gens"));
		d.addNode(mPostTexGens.spawnNode("Post Tex Gens"));
		d.addNode(mTexMatrices.spawnNode("Tex Matrices"));
		d.addNode(mPostTexMatrices.spawnNode("Post Tex Matrices"));
		d.addNode(mTextureTable.spawnNode("Texture ID Table"));
		d.addNode(mOrders.spawnNode("TEV Orders"));
		d.addNode(mTevColors.spawnNode("TEV Colors"));
		d.addNode(mKonstColors.spawnNode("TEV Konstant Colors"));
		d.addNode(mNumTevStages.spawnNode("TEV Stage Count"));
		d.addNode(mTevStages.spawnNode("TEV Stages"));
		d.addNode(mSwapModes.spawnNode("TEV Swap Orders"));
		d.addNode(mSwapTables.spawnNode("TEV Swap Tables"));
		d.addNode(mFogs.spawnNode("Fogs"));
		d.addNode(mAlphaComparisons.spawnNode("Alpha Comparisons"));
		d.addNode(mBlendModes.spawnNode("Blend Modes"));
		d.addNode(mZModes.spawnNode("Z Modes"));
		d.addNode(mZCompLocs.spawnNode("Z Comparison Locations"));
		d.addNode(mDithers.spawnNode("Dithers"));
		d.addNode(mNBTScales.spawnNode("NBT Scales"));
		return {};
	}
};
bool SerializableMaterial::operator==(const SerializableMaterial& rhs) const noexcept
{
	return mMAT3.mMdl.getMaterials()[mIdx] == rhs.mMAT3.mMdl.getMaterials()[rhs.mIdx];
}
template<>
struct io_wrapper<SerializableMaterial>
{
	static void onWrite(oishii::v2::Writer& writer, const SerializableMaterial& smat)
	{
		const Material& m = smat.mMAT3.mMdl.getMaterials()[smat.mIdx];
		
		oishii::DebugExpectSized dbg(writer, 332);

		writer.write<u8>(m.flag);
		writer.write<u8>(smat.mMAT3.mCullModes.find(m.cullMode));
		writer.write<u8>(smat.mMAT3.mNumColorChannels.find(m.info.nColorChan));
		writer.write<u8>(smat.mMAT3.mNumTexGens.find(m.info.nTexGen));
		writer.write<u8>(smat.mMAT3.mNumTevStages.find(m.info.nTevStage));
		writer.write<u8>(smat.mMAT3.mZCompLocs.find(m.earlyZComparison));
		writer.write<u8>(smat.mMAT3.mZModes.find(m.zMode));
		writer.write<u8>(smat.mMAT3.mDithers.find(m.dither));

		dbg.assertSince(0x008);
		const auto& m3 = smat.mMAT3;

		array_vector<gx::Color, 2> matColors, ambColors;
		for (int i = 0; i < m.chanData.size(); ++i)
		{
			matColors.push_back(m.chanData[i].matColor);
			ambColors.push_back(m.chanData[i].ambColor);
		}
		write_array_vec<u16>(writer, matColors, m3.mMaterialColors);
		write_array_vec<u16>(writer, m.colorChanControls, m3.mChannelControls);
		write_array_vec<u16>(writer, ambColors, m3.mAmbColors);
		write_array_vec<u16>(writer, m.lightColors, m3.mLightColors);
		
		for (int i = 0; i < m.texGens.size(); ++i)
			writer.write<u16>(m3.mTexGens.find(m.texGens[i]));
		for (int i = m.texGens.size(); i < m.texGens.max_size(); ++i)
			writer.write<u16>(-1);

		for (int i = 0; i < m.texGens.size(); ++i)
			writer.write<u16>(m.texGens[i].postMatrix == gx::PostTexMatrix::Identity ?
				-1 : m3.mPostTexGens.find(m3.postTexGen(m.texGens[i])));
		for (int i = m.texGens.size(); i < m.texGens.max_size(); ++i)
			writer.write<u16>(-1);

		dbg.assertSince(0x048);
		array_vector<Material::TexMatrix, 10> texMatrices;
		for (int i = 0; i < m.texMatrices.size(); ++i)
			texMatrices.push_back(*m.texMatrices[i].get());
		write_array_vec<u16>(writer, texMatrices, m3.mTexMatrices);
		// TODO: Assumption
		write_array_vec<u16>(writer, m.postTexMatrices, m3.mPostTexMatrices);
		//	write_array_vec<u16>(writer, texMatrices, m3.mTexMatrices);
		//	for (int i = 0; i < 10; ++i)
		//		writer.write<s16>(-1);

		dbg.assertSince(0x084);
		array_vector<Material::J3DSamplerData, 8> samplers;
		samplers.nElements = m.samplers.size();
		for (int i = 0; i < m.samplers.size(); ++i)
			samplers[i] = (Material::J3DSamplerData&)*m.samplers[i].get();
		dbg.assertSince(0x084);
		write_array_vec<u16>(writer, samplers, m3.mTextureTable);
		dbg.assertSince(0x094);
		write_array_vec<u16>(writer, m.tevKonstColors, m3.mKonstColors);

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
			writer.write<u16>(m3.mOrders.find(TevOrder{ m.shader.mStages[i].rasOrder, m.shader.mStages[i].texMap, m.shader.mStages[i].texCoord }));
		for (int i = m.shader.mStages.size(); i < 16; ++i)
			writer.write<u16>(-1);

		dbg.assertSince(0x0dc);
		write_array_vec<u16>(writer, m.tevColors, m3.mTevColors);

		dbg.assertSince(0x0e4);
		for (int i = 0; i < m.shader.mStages.size(); ++i)
			writer.write<u16>(m3.mTevStages.find(m.shader.mStages[i]));
		for (int i = m.shader.mStages.size(); i < 16; ++i)
			writer.write<u16>(-1);

		dbg.assertSince(0x104);
		for (int i = 0; i < m.shader.mStages.size(); ++i)
			writer.write<u16>(m3.mSwapModes.find(SwapSel{ m.shader.mStages[i].rasSwap, m.shader.mStages[i].texMapSwap }));
		for (int i = m.shader.mStages.size(); i < 16; ++i)
			writer.write<u16>(-1);

		dbg.assertSince(0x124);
		for (const auto& table : m.shader.mSwapTable)
			writer.write<u16>(m3.mSwapTables.find(table));

		for (const u8 s : m.stackTrash)
			writer.write<u8>(s);

		dbg.assertSince(0x144);
		writer.write<u16>(m3.mFogs.find(m.fogInfo));
		writer.write<u16>(m3.mAlphaComparisons.find(m.alphaCompare));
		writer.write<u16>(m3.mBlendModes.find(m.blendMode));
		writer.write<u16>(m3.mNBTScales.find(m.nbtScale));
	}
};
std::unique_ptr<oishii::v2::Node> makeMAT3Node(BMDExportContext& ctx)
{
	return std::make_unique<MAT3Node>(ctx);
}

}
