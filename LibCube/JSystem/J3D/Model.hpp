#pragma once

#include <LibRiiEditor/pluginapi/FileState.hpp>
#include <LibRiiEditor/pluginapi/Interfaces/TextureList.hpp>
#include <LibCube/GX/VertexTypes.hpp>
#include <map>
#include <string>
#include <vector>
#include <ThirdParty/glm/mat4x4.hpp>

#include <LibCube/Common/BoundBox.hpp>

#include <LibCube/GX/Material.hpp>

#include <oishii/writer/binary_writer.hxx>

namespace libcube::jsystem {

// Not a FileState for now -- always accessed through a J3DCollection.
// This is subject to change
// No compatibility with old BMD files yet.
struct J3DModel
{
	J3DModel() = default;
	~J3DModel() = default;

	template<typename T>
	using ID = std::string;

	// Assumption: all elements are contiguous--no holes
	// Much faster than a vector for the many static sized arrays in materials
	template<typename T, size_t N>
	struct array_vector : public std::array<T, N>
	{
		size_t size() const
		{
			return nElements;
		}
		size_t nElements = 0;

		void push_back(T elem)
		{
			at(nElements) = elem;
		}
		void pop_back()
		{
			--nElements;
		}
	};
	struct Material;
	struct Shape;

	struct Information
	{
		// For texmatrix calculations
		enum class ScalingRule
		{
			Basic,
			XSI,
			Maya
		};

		ScalingRule mScalingRule;
		// nPacket, nVtx

		// Hierarchy data is included in joints.
	};

	Information info;

	struct VQuantization
	{
		gx::VertexComponentCount comp = gx::VertexComponentCount(gx::VertexComponentCount::Position::xyz);
		gx::VertexBufferType type = gx::VertexBufferType(gx::VertexBufferType::Generic::f32);
		u8 divisor = 0;
		u8 stride = 12;

		VQuantization(gx::VertexComponentCount c, gx::VertexBufferType t, u8 d, u8 s)
			: comp(c), type(t), divisor(d), stride(s)
		{}
		VQuantization(const VQuantization& other)
			: comp(other.comp), type(other.type), divisor(other.divisor), stride(other.stride)
		{}
		VQuantization() = default;
	};
	enum class VBufferKind
	{
		position,
		normal,
		color,
		textureCoordinate,

		undefined = -1
	};

	template<typename T, VBufferKind kind>
	struct VertexBuffer
	{
		VQuantization mQuant;
		std::vector<T> mData;

		int ComputeComponentCount() const
		{
			switch (kind)
			{
			case VBufferKind::position:
				if (mQuant.comp.position == gx::VertexComponentCount::Position::xy)
					throw "Buffer: XY Pos Component count.";
				return static_cast<int>(mQuant.comp.position) + 2; // xy -> 2; xyz -> 3
			case VBufferKind::normal:
				return 3;
			case VBufferKind::color:
				return static_cast<int>(mQuant.comp.color) + 3; // rgb -> 3; rgba -> 4
			case VBufferKind::textureCoordinate:
				return static_cast<int>(mQuant.comp.texcoord) + 1; // s -> 1, st -> 2
			default: // never reached
				return 0;
			}
		}

		template<int n, typename T, glm::qualifier q>
		void readBufferEntryGeneric(oishii::BinaryReader& reader, glm::vec<n, T, q>& result)
		{
			for (int i = 0; i < ComputeComponentCount(); ++i)
			{
				switch (mQuant.type.generic)
				{
				case gx::VertexBufferType::Generic::u8:
					result[i] = reader.read<u8>() / f32(1 << mQuant.divisor);
					break;
				case gx::VertexBufferType::Generic::s8:
					result[i] = reader.read<s8>() / f32(1 << mQuant.divisor);
					break;
				case gx::VertexBufferType::Generic::u16:
					result[i] = reader.read<u16>() / f32(1 << mQuant.divisor);
					break;
				case gx::VertexBufferType::Generic::s16:
					result[i] = reader.read<s16>() / f32(1 << mQuant.divisor);
					break;
				case gx::VertexBufferType::Generic::f32:
					result[i] = reader.read<f32>();
					break;
				default:
					throw "Invalid buffer type!";
				}
			}
		}
		void readBufferEntryColor(oishii::BinaryReader& reader, gx::Color& result)
		{
			switch (mQuant.type.color)
			{
			case gx::VertexBufferType::Color::rgb565:
			{
				const u16 c = reader.read<u16>();
				result.r = (c & 0xF800) >> 11;
				result.g = (c & 0x07E0) >> 5;
				result.b = (c & 0x001F);
				break;
			}
			case gx::VertexBufferType::Color::rgb8:
				result.r = reader.read<u8>();
				result.g = reader.read<u8>();
				result.b = reader.read<u8>();
				break;
			case gx::VertexBufferType::Color::rgbx8:
				result.r = reader.read<u8>();
				result.g = reader.read<u8>();
				result.b = reader.read<u8>();
				reader.seek(1);
				break;
			case gx::VertexBufferType::Color::rgba4:
			{
				const u16 c = reader.read<u16>();
				result.r = (c & 0xF000) >> 12;
				result.g = (c & 0x0F00) >> 8;
				result.b = (c & 0x00F0) >> 4;
				result.a = (c & 0x000F);
				break;
			}
			case gx::VertexBufferType::Color::rgba6:
			{
				const u32 c = reader.read<u32>();
				result.r = (c & 0xFC0000) >> 18;
				result.g = (c & 0x03F000) >> 12;
				result.b = (c & 0x000FC0) >> 6;
				result.a = (c & 0x00003F);
				break;
			}
			case gx::VertexBufferType::Color::rgba8:
				result.r = reader.read<u8>();
				result.g = reader.read<u8>();
				result.b = reader.read<u8>();
				result.a = reader.read<u8>();
				break;
			default:
				throw "Invalid buffer type!";
			};
		}

		template<int n, typename T, glm::qualifier q>
		void writeBufferEntryGeneric(oishii::Writer& writer, const glm::vec<n, T, q>& v) const
		{
			const auto cnt = ComputeComponentCount();
			for (int i = 0; i < cnt; ++i)
			{
				switch (mQuant.type.generic)
				{
				case gx::VertexBufferType::Generic::u8:
					writer.write<u8>(roundf(v[i] * (1 << mQuant.divisor)));
					break;
				case gx::VertexBufferType::Generic::s8:
					writer.write<s8>(roundf(v[i] * (1 << mQuant.divisor)));
					break;
				case gx::VertexBufferType::Generic::u16:
					writer.write<u16>(roundf(v[i] * (1 << mQuant.divisor)));
					break;
				case gx::VertexBufferType::Generic::s16:
					writer.write<s16>(roundf(v[i] * (1 << mQuant.divisor)));
					break;
				case gx::VertexBufferType::Generic::f32:
					writer.write<f32>(v[i]);
					break;
				default:
					throw "Invalid buffer type!";
				}
			}
		}
		void writeBufferEntryColor(oishii::Writer& writer, const gx::Color& c) const
		{
			switch (mQuant.type.color)
			{
			case gx::VertexBufferType::Color::rgb565:
				writer.write<u16>(((c.r & 0xf8) << 8) | ((c.g & 0xfc) << 3) | ((c.b & 0xf8) >> 3));
				break;
			case gx::VertexBufferType::Color::rgb8:
				writer.write<u8>(c.r);
				writer.write<u8>(c.g);
				writer.write<u8>(c.b);
				break;
			case gx::VertexBufferType::Color::rgbx8:
				writer.write<u8>(c.r);
				writer.write<u8>(c.g);
				writer.write<u8>(c.b);
				writer.write<u8>(0);
				break;
			case gx::VertexBufferType::Color::rgba4:
				writer.write<u16>( ((c.r & 0xf0) << 8) | ((c.g & 0xf0) << 4) | (c.b & 0xf0) | ((c.a & 0xf0) >> 4) );
				break;
			case gx::VertexBufferType::Color::rgba6:
			{
				// TODO: Verify
				u32 v = ((c.r & 0x3f) << 18) | ((c.g & 0x3f) << 12) | (c.b & 0x3f << 6) | (c.a & 0x3f);
				writer.write<u8>((v & 0x00ff0000) >> 16);
				writer.write<u8>((v & 0x0000ff00) >> 8);
				writer.write<u8>((v & 0x000000ff));
				break;
			}
			case gx::VertexBufferType::Color::rgba8:
				writer.write<u8>(c.r);
				writer.write<u8>(c.g);
				writer.write<u8>(c.b);
				writer.write<u8>(c.a);
				break;
			default:
				throw "Invalid buffer type!";
			};
		}

		// For dead cases
		template<int n, typename T, glm::qualifier q>
		void writeBufferEntryColor(oishii::Writer& writer, const glm::vec<n, T, q>& v) const
		{
			assert(!"Invalid kind/type template match!");
		}
		void writeBufferEntryGeneric(oishii::Writer& writer, const gx::Color& c) const
		{
			assert(!"Invalid kind/type template match!");
		}

		void writeData(oishii::Writer& writer) const
		{
			switch (kind)
			{
			case VBufferKind::position:
				if (mQuant.comp.position == gx::VertexComponentCount::Position::xy)
					throw "Buffer: XY Pos Component count.";

				for (const auto& d : mData)
					writeBufferEntryGeneric(writer, d);
				break;
			case VBufferKind::normal:
				if (mQuant.comp.normal != gx::VertexComponentCount::Normal::xyz)
					throw "Buffer: NBT Vectors.";

				for (const auto& d : mData)
					writeBufferEntryGeneric(writer, d);
				break;
			case VBufferKind::color:
				for (const auto& d : mData)
					writeBufferEntryColor(writer, d);
				break;
			case VBufferKind::textureCoordinate:
				if (mQuant.comp.texcoord == gx::VertexComponentCount::TextureCoordinate::s)
					throw "Buffer: Single component texcoord vectors.";

				for (const auto& d : mData)
					writeBufferEntryGeneric(writer, d);
				break;
			}
		}
		
		VertexBuffer() {}
		VertexBuffer(VQuantization q)
			: mQuant(q)
		{}

	};

	struct Bufs
	{
		// FIXME: Good default values
		VertexBuffer<glm::vec3, VBufferKind::position> pos { VQuantization() };
		VertexBuffer<glm::vec3, VBufferKind::normal> norm { VQuantization() };
		std::array<VertexBuffer<gx::Color, VBufferKind::color>, 2> color;
		std::array<VertexBuffer<glm::vec2, VBufferKind::textureCoordinate>, 8> uv;

		Bufs() {}
	} mBufs = Bufs();

	//! Encapsulates low level envelopes and draw matrices
	struct DrawMatrix
	{
		struct MatrixWeight
		{
			// TODO: Proper reference system
			/* std::string */ u32 boneId;
			f32 weight;

			MatrixWeight()
			: boneId(-1), weight(0.0f)
			{}
			MatrixWeight(u32 b, f32 w)
			: boneId(b), weight(w)
			{}
		};

		std::vector<MatrixWeight> mWeights; // 1 weight -> singlebound, no envelope
	};

	std::vector<DrawMatrix> mDrawMatrices;


	struct Joint
	{
		// Four LSBs of flag; left is matrix type
		enum class MatrixType : u16
		{
			Standard = 0,
			Billboard,
			BillboardY
		};

		ID<Joint> id;

		u16 flag; // Unused four bits; default value in galaxy is 1
		MatrixType bbMtxType;
		bool mayaSSC; // 0xFF acts as false -- likely for compatibility

		glm::vec3 scale, rotate, translate;

		f32 boundingSphereRadius;
		AABB boundingBox;

		// From INF1
		ID<Joint> parent;
		std::vector<ID<Joint>> children;

		struct Display
		{
			ID<Material> material;
			u32/*ID<Shape>*/ shape;

			Display() = default;
			Display(ID<Material> mat, u32/*ID<Shape>*/ shp)
				: material(mat), shape(shp)
			{}
		};
		std::vector<Display> displays;

		// From EVP1
		glm::mat4x4 inverseBindPoseMtx;
	};

	std::vector<Joint> mJoints;

	using todo = int;

	

	struct Material
	{
		struct GenInfo
		{
			u8 nColorChannel, nTexGen, nTevStage;
			bool indirect;
			u8 nInd;
		};
		struct TexMatrix
		{
			gx::TexGenType projection; // Only 3x4 and 2x4 valid

			bool		maya;
			u8			mappingMethod;

			glm::vec3	origin;

			glm::vec2	scale;
			f32			rotate;
			glm::vec2	translate;

			glm::vec4	effectMatrix;
		};
		struct NBTScale
		{
			bool enable;
			glm::vec3 scale;
		};
		
		ID<Material> id;

		u8 flag;

		GenInfo info;
		gx::CullMode cullMode;

		bool earlyZComparison;
		gx::ZMode ZMode;

		array_vector<gx::Color, 2> matColors;
		array_vector<gx::ChannelControl, 4> colorChanControls;
		array_vector<gx::Color, 2> ambColors;
		array_vector<gx::Color, 8> lightColors;

		array_vector<gx::TexCoordGen, 8> texGenInfos;

		array_vector<TexMatrix, 10> texMatrices;
		array_vector<TexMatrix, 20> postTexMatrices;

		// FIXME: Sampler data will be moved here from TEX1
		array_vector<todo, 8> textures;

		array_vector<gx::Color, 4> tevKonstColors;
		array_vector<gx::ColorS10, 4> tevColors;

		gx::Shader shader;

		// FIXME: This will exist in our scene and be referenced.
		todo fogInfo;

		gx::AlphaComparison alphaCompare;
		gx::BlendMode blendMode;
		bool dither;
		NBTScale nbtScale;
	};
	std::vector<Material> mMaterials;
};

} // namespace libcube::jsystem
