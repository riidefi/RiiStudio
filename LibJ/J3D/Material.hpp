#pragma once

#include <LibCore/common.h>
#include <vector>
#include <array>

#include <LibCube/GX/Material.hpp>
#include <ThirdParty/glm/glm.hpp>

#include <LibCube/Export/Material.hpp>

#include <ThirdParty/fa5/IconsFontAwesome5.h>

namespace libcube::jsystem {

template<typename T>
using ID = int;


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
	bool operator==(const array_vector& rhs) const noexcept
	{
		if (rhs.nElements != nElements) return false;
		for (int i = 0; i < nElements; ++i)
		{
			const T& l = this->at(i);
			const T& r = rhs.at(i);
			if (!(l == r))
				return false;
		}
		return true;
	}
};
struct MaterialData
{
	struct GenInfo
	{
		u8 nColorChannel, nTexGen = 0;
		u8 nTevStage = 1;
		// From Ind block
		bool indirect = false;
		u8 nInd = 0;

		bool operator==(const GenInfo& rhs) const
		{
			return nColorChannel == rhs.nColorChannel && nTexGen == rhs.nTexGen &&
				nTevStage == rhs.nTevStage && indirect == rhs.indirect && nInd == rhs.nInd;
		}
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

		std::array<f32, 16>	effectMatrix;

		bool operator==(const TexMatrix& rhs) const noexcept
		{
			return projection == rhs.projection &&
				maya == rhs.maya &&
				mappingMethod == rhs.mappingMethod &&
				origin == rhs.origin &&
				scale == rhs.scale &&
				rotate == rhs.rotate &&
				translate == rhs.translate &&
				effectMatrix == rhs.effectMatrix;
		}
	};
	struct NBTScale
	{
		bool enable = false;
		glm::vec3 scale = { 0.0f, 0.0f, 0.0f };

		bool operator==(const NBTScale& rhs) const noexcept
		{
			return enable == rhs.enable && scale == rhs.scale;
		}
	};

	std::string name = "";
	ID<Material> id;

	u8 flag = 0;

	GenInfo info;
	gx::CullMode cullMode = gx::CullMode::Back;

	bool earlyZComparison;
	gx::ZMode ZMode;

	array_vector<gx::Color, 2> matColors;
	array_vector<gx::ChannelControl, 4> colorChanControls;
	array_vector<gx::Color, 2> ambColors;
	array_vector<gx::Color, 8> lightColors;

	array_vector<gx::TexCoordGen, 8> texGenInfos;

	array_vector<TexMatrix, 10> texMatrices;
	array_vector<TexMatrix, 20> postTexMatrices;

	struct SamplerData
	{
		std::string mTexture;
		// TODO: TLUT

		gx::TextureWrapMode mWrapU = gx::TextureWrapMode::Repeat;
		gx::TextureWrapMode mWrapV = gx::TextureWrapMode::Repeat;

		bool bMipMap = false;
		bool bEdgeLod;
		bool bBiasClamp;

		u8 mMaxAniso;
		u8 mMinFilter;
		u8 mMagFilter;
		s16 mLodBias;

		// Only used in TEX1 linking. Set the string.
		u32 btiId;

		bool operator==(const SamplerData& rhs) const noexcept
		{
			return mTexture == rhs.mTexture && mWrapU == rhs.mWrapU && mWrapV == rhs.mWrapV &&
				bMipMap == rhs.bMipMap && bEdgeLod == rhs.bEdgeLod && bBiasClamp == rhs.bBiasClamp &&
				mMaxAniso == rhs.mMaxAniso && mMinFilter == rhs.mMinFilter && mMagFilter == rhs.mMagFilter &&
				mLodBias == rhs.mLodBias;
		}
	};
	array_vector<SamplerData, 8> textures;

	array_vector<gx::Color, 4> tevKonstColors;
	array_vector<gx::ColorS10, 4> tevColors;

	gx::Shader shader = gx::Shader();
	// Split up -- only 3 indmtx
	std::vector<gx::IndirectTextureScalePair> mIndScales;
	std::vector<gx::IndirectMatrix> mIndMatrices;

	// FIXME: This will exist in our scene and be referenced.
	// For now, for 1:1, just placed here..
	struct Fog
	{
		enum class Type
		{
			None,

			PerspectiveLinear,
			PerspectiveExponential,
			PerspectiveQuadratic,
			PerspectiveInverseExponential,
			PerspectiveInverseQuadratic,

			OrthographicLinear,
			OrthographicExponential,
			OrthographicQuadratic,
			OrthographicInverseExponential,
			OrthographicInverseQuadratic
		};
		Type type = Type::None;
		bool enabled = false;
		u16 center;
		f32 startZ, endZ = 0.0f;
		f32 nearZ, farZ = 0.0f;
		gx::Color color = gx::Color(0xffffffff);
		std::array<u16, 10> rangeAdjTable;

		bool operator==(const Fog& rhs) const noexcept
		{
			return type == rhs.type && enabled == rhs.enabled &&
				center == rhs.center &&
				startZ == rhs.startZ && endZ == rhs.endZ &&
				nearZ == rhs.nearZ && farZ == rhs.farZ &&
				color == rhs.color &&
				rangeAdjTable == rhs.rangeAdjTable;
		}
	};
	Fog fogInfo;

	gx::AlphaComparison alphaCompare;
	gx::BlendMode blendMode;
	bool dither;
	NBTScale nbtScale;

	std::array<u8, 24> stackTrash{}; //!< We have to remember this for 1:1

	// Note: For comparison we don't check the ID or name (or stack trash)
	bool operator==(const MaterialData& rhs) const noexcept
	{
		return flag == rhs.flag &&
			info == rhs.info &&
			cullMode == rhs.cullMode &&
			earlyZComparison == rhs.earlyZComparison &&
			ZMode == rhs.ZMode &&
			matColors == rhs.matColors &&
			colorChanControls == rhs.colorChanControls &&
			ambColors == rhs.ambColors &&
			lightColors == rhs.lightColors &&
			texGenInfos == rhs.texGenInfos &&
			texMatrices == rhs.texMatrices &&
			postTexMatrices == rhs.postTexMatrices &&
			tevKonstColors == rhs.tevKonstColors &&
			tevColors == rhs.tevColors &&
			shader == rhs.shader &&
			mIndScales == rhs.mIndScales &&
			mIndMatrices == rhs.mIndMatrices &&
			fogInfo == rhs.fogInfo &&
			alphaCompare == rhs.alphaCompare &&
			blendMode == rhs.blendMode &&
			dither == rhs.dither &&
			nbtScale == rhs.nbtScale;
	}
};


struct Material final : public MaterialData, public IMaterialDelegate
{
	PX_TYPE_INFO_EX("J3D Material", "j3d_material", "J::Material", ICON_FA_PAINT_BRUSH, ICON_FA_PAINT_BRUSH);

	~Material() override = default;
	Material()
	{
		using P = PropertySupport;
		support.setSupport(P::Feature::CullMode, P::Coverage::ReadWrite);
		support.setSupport(P::Feature::ZCompareLoc, P::Coverage::ReadWrite);
		support.setSupport(P::Feature::ZCompare, P::Coverage::ReadWrite);
		support.setSupport(P::Feature::GenInfo, P::Coverage::ReadWrite);
		support.setSupport(P::Feature::MatAmbColor, P::Coverage::ReadWrite);
	}
	std::string getName() const override
	{
		return name;
	}
	s64 getId() const override
	{
		return id;
	}
	void* getRaw() override
	{
		return this;
	}
	gx::CullMode getCullMode() const override
	{
		return cullMode;
	}
	void setCullMode(gx::CullMode c) override
	{
		cullMode = c;
	}
	bool getZCompLoc() const override
	{
		return earlyZComparison;
	}
	void setZCompLoc(bool b) override
	{
		earlyZComparison = b;
	}

	GenInfoCounts getGenInfo() const override
	{
		return {
			info.nColorChannel,
			info.nTexGen,
			info.nTevStage,
			info.indirect ? info.nInd : u8(0)
		};
	}
	void setGenInfo(const GenInfoCounts& c) override
	{
		info.nColorChannel = c.colorChan;
		info.nTexGen = c.texGen;
		info.nTevStage = c.tevStage;
		info.indirect = c.indStage;
		info.nInd = c.indStage;
	}


	gx::Color getMatColor(u64 idx) const override
	{
		if (idx < 2)
			return matColors[idx];

		return {};
	}
	virtual gx::Color getAmbColor(u64 idx) const override
	{
		if (idx < 2)
			return ambColors[idx];

		return {};
	}
	void setMatColor(u64 idx, gx::Color v) override
	{
		if (idx < 2)
			matColors[idx] = v;
	}
	void setAmbColor(u64 idx, gx::Color v) override
	{
		if (idx < 2)
			ambColors[idx] = v;
	}

};

}
