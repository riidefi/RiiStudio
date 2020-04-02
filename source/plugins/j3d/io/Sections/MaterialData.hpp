#define GLM_ENABLE_EXPERIMENTAL
#include <vendor/glm/gtx/matrix_decompose.hpp>

#include "../Sections.hpp"
#include <oishii/v2/writer/binary_writer.hxx>

#include <plugins/gc/Util/glm_serialization.hpp>

namespace riistudio::j3d {

using namespace libcube;

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
		const auto in = reader.read<u8>();
		out.attenuationFn = in < cvt.size() ? cvt[in] : (gx::AttenuationFunction)(in);
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
		constexpr const std::array<u8, 4> cvt{
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

		if (mappingMethod >= J3DMappingMethods.size())
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
template<>
struct io_wrapper<Model::Indirect>
{
	static void onRead(oishii::BinaryReader& reader, Model::Indirect& c)
	{
		reader.warnAt("Start of IndTex block", reader.tell(), reader.tell() + 4);
		c.enabled = reader.read<u8>();
		c.nIndStage = reader.read<u8>();
		if (c.nIndStage > 4) reader.warnAt("Invalid stage count", reader.tell() - 1, reader.tell());
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
	static void onWrite(oishii::v2::Writer& writer, const Model::Indirect& c)
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
			out = glm::translate(out, { e.trans.x, e.trans.y, 0.0f });
			out = glm::rotate(out, e.rotate, { 0.0f, 1.0f, 0.0f });
			out = glm::scale(out, { e.scale.x, e.scale.y, 1.0f, });

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
			for (int i = 0; i < 3; ++i)
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

template<>
struct io_wrapper<gx::CullMode>
{
	static void onRead(oishii::BinaryReader& reader, gx::CullMode& out) {
		out = static_cast<gx::CullMode>(reader.read<u32>());
	}
	static void onWrite(oishii::v2::Writer& writer, gx::CullMode cm)
	{
		writer.write<u32>(static_cast<u32>(cm));
	}
};
}
