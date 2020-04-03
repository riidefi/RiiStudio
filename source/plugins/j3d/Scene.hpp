#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/kpi/Node.hpp>

#include "VertexBuffer.hpp"
#include "DrawMatrix.hpp"
#include "Material.hpp"
#include "Joint.hpp"
#include "Shape.hpp"
#include "Texture.hpp"


namespace riistudio::j3d {


struct TevOrder
{
	libcube::gx::ColorSelChanApi rasOrder;
	u8 texMap, texCoord;

	bool operator==(const TevOrder& rhs) const noexcept
	{
		return rasOrder == rhs.rasOrder && texMap == rhs.texMap && texCoord == rhs.texCoord;
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

struct Collection : public lib3d::Scene {
	bool operator==(const Collection& rhs) const { return true;}
};


struct Tex
{
	u8 mFormat;
	u8 bTransparent;
	u16 mWidth, mHeight;
	libcube::gx::TextureWrapMode mWrapU, mWrapV;
	u8 mPaletteFormat;
	u16 nPalette;
	u32 ofsPalette;
	u8 bMipMap;
	u8 bEdgeLod;
	u8 bBiasClamp;
	libcube::gx::AnisotropyLevel mMaxAniso;
	libcube::gx::TextureFilter mMinFilter;
	libcube::gx::TextureFilter mMagFilter;
	s8 mMinLod;
	s8 mMaxLod;
	u8 mMipmapLevel;
	s16 mLodBias;
	u32 ofsTex;

	// Not written, tracked
	s32 btiId = -1;

	bool operator==(const Tex& rhs) const {
		return mFormat == rhs.mFormat && bTransparent == rhs.bTransparent && mWidth == rhs.mWidth && mHeight == rhs.mHeight &&
			mWrapU == rhs.mWrapU && mWrapV == rhs.mWrapV &&
			mPaletteFormat == rhs.mPaletteFormat && nPalette == rhs.nPalette && ofsPalette == rhs.ofsPalette && bMipMap == rhs.bMipMap &&
			bEdgeLod == rhs.bEdgeLod && bBiasClamp == rhs.bBiasClamp && mMaxAniso == rhs.mMaxAniso &&
			mMinFilter == rhs.mMinFilter && mMagFilter == rhs.mMagFilter && mMinLod == rhs.mMinLod && mMaxLod == rhs.mMaxLod &&
			mMipmapLevel == rhs.mMipmapLevel && mLodBias == rhs.mLodBias && btiId == rhs.btiId; // ofsTex not checked
	}

	void transfer(oishii::BinaryReader& stream);
	void write(oishii::v2::Writer& stream) const;
	Tex() = default;
	Tex(const Texture& data, const libcube::GCMaterialData::SamplerData& sampler);
};

struct Model : public lib3d::Model {
	virtual ~Model() = default;
    // Shallow comparison
	bool operator==(const Model& rhs) const {
		return false;
    }
	struct Information
	{
		// For texmatrix calculations
		enum class ScalingRule
		{
			Basic,
			XSI,
			Maya
		};

		ScalingRule mScalingRule = ScalingRule::Basic;
		// nPacket, nVtx

		// Hierarchy data is included in joints.
	};

	Information info;

	struct Bufs
	{
		// FIXME: Good default values
		VertexBuffer<glm::vec3, VBufferKind::position> pos{ VQuantization{
			libcube::gx::VertexComponentCount(libcube::gx::VertexComponentCount::Position::xyz),
			libcube::gx::VertexBufferType(libcube::gx::VertexBufferType::Generic::f32),
			0,
			0,
			12
		} };
		VertexBuffer<glm::vec3, VBufferKind::normal> norm{ VQuantization{
			libcube::gx::VertexComponentCount(libcube::gx::VertexComponentCount::Normal::xyz),
			libcube::gx::VertexBufferType(libcube::gx::VertexBufferType::Generic::f32),
			0,
			0,
			12
		} };
		std::array<VertexBuffer<libcube::gx::Color, VBufferKind::color>, 2> color;
		std::array<VertexBuffer<glm::vec2, VBufferKind::textureCoordinate>, 8> uv;

		Bufs() {
			for (auto& clr : color) {
				clr = { VQuantization{
					libcube::gx::VertexComponentCount(libcube::gx::VertexComponentCount::Color::rgba),
					libcube::gx::VertexBufferType(libcube::gx::VertexBufferType::Color::FORMAT_32B_8888),
					0,
					0,
					4
				} };
			}
			for (auto& uv_ : uv) {
				uv_ = { VQuantization{
					libcube::gx::VertexComponentCount(libcube::gx::VertexComponentCount::TextureCoordinate::st),
					libcube::gx::VertexBufferType(libcube::gx::VertexBufferType::Generic::f32),
					0,
					0,
					8
				} };
			}
		}
	} mBufs = Bufs();

	
	std::vector<DrawMatrix> mDrawMatrices;

	struct Indirect
	{
		bool enabled;
		u8 nIndStage;

		std::array<libcube::gx::IndOrder, 4> tevOrder;
		std::array<libcube::gx::IndirectMatrix, 3> texMtx;
		std::array<libcube::gx::IndirectTextureScalePair, 4> texScale;
		std::array<libcube::gx::TevStage::IndirectStage, 16> tevStage;

		Indirect() = default;
		Indirect(const MaterialData& mat)
		{
			enabled = mat.indEnabled;
			nIndStage = mat.info.nIndStage;

			assert(nIndStage <= 4);

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
	mutable struct MatCache {
		template<typename T>
		using Section = std::vector<T>;
		Section<Indirect> indirectInfos;
		Section<libcube::gx::CullMode> cullModes;
		Section<libcube::gx::Color> matColors;
		Section<u8> nColorChan;
		Section<libcube::gx::ChannelControl> colorChans;
		Section<libcube::gx::Color> ambColors;
		Section<libcube::gx::Color> lightColors;
		Section<u8> nTexGens;
		Section<libcube::gx::TexCoordGen> texGens;
		Section<libcube::gx::TexCoordGen> posTexGens;
		Section<Material::TexMatrix> texMatrices;
		Section<Material::TexMatrix> postTexMatrices;
		Section<Material::J3DSamplerData> samplers;
		Section<TevOrder> orders;
		Section<libcube::gx::ColorS10> tevColors;
		Section<libcube::gx::Color> konstColors;

		Section<u8> nTevStages;
		Section<libcube::gx::TevStage> tevStages;
		Section<SwapSel> swapModes;
		Section<libcube::gx::SwapTableEntry> swapTables;
		Section<Fog> fogs;

		Section<libcube::gx::AlphaComparison> alphaComparisons;
		Section<libcube::gx::BlendMode> blendModes;
		Section<libcube::gx::ZMode> zModes;
		Section<u8> zCompLocs;
		Section<u8> dithers;
		Section<NBTScale> nbtScales;

		void clear() {
			*this = MatCache{};
		}
		template<typename T>
		void update_section(std::vector<T>& sec, const T& data) {
			if (std::find(sec.begin(), sec.end(), data) == sec.end()) {
				sec.push_back(data);
			}
		}
		template<typename T, typename U>
		void update_section_multi(std::vector<T>& sec, const U& source) {
			for (int i = 0; i < source.size(); ++i) {
				update_section(sec, source[i]);
			}
		}
		void propogate(Material& mat);
	} mMatCache;

	mutable std::vector<Tex> mTexCache;
};

struct ModelAccessor : public kpi::NodeAccessor<Model> {
	KPI_NODE_FOLDER_SIMPLE(Material);
	KPI_NODE_FOLDER_SIMPLE(Joint);
	KPI_NODE_FOLDER_SIMPLE(Shape);
};

struct CollectionAccessor : public kpi::NodeAccessor<Collection> {
	using super = kpi::NodeAccessor<Collection>;
	using super::super;

    KPI_NODE_FOLDER(Model, ModelAccessor);
	KPI_NODE_FOLDER_SIMPLE(Texture);
};

} // namespace riistudio::j3d
