#pragma once

#include <LibRiiEditor/common.hpp>
#include <vector>
#include <array>

#include <LibCube/GX/Material.hpp>
#include <ThirdParty/glm/glm.hpp>

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
};
struct MaterialData
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

	std::string name;
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

	//	// FIXME: Sampler data will be moved here from TEX1
	//	array_vector<todo, 8> textures;

	array_vector<gx::Color, 4> tevKonstColors;
	array_vector<gx::ColorS10, 4> tevColors;

	gx::Shader shader;
	// FIXME: This will exist in our scene and be referenced.
	// For now, for 1:1, just placed here..
	struct Fog
	{
		enum Type
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
		Type type;
		bool enabled;
		u16 center;
		f32 startZ, endZ;
		f32 nearZ, farZ;
		gx::Color color;
		std::array<u16, 10> rangeAdjTable;
	};
	Fog fogInfo;

	gx::AlphaComparison alphaCompare;
	gx::BlendMode blendMode;
	bool dither;
	NBTScale nbtScale;
};


struct Material final : public MaterialData, public GCCollection::IMaterialDelegate
{
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
	int getId() const override
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
	void setGenInfo(const GenInfoCounts& c)
	{
		info.nColorChannel = c.colorChan;
		info.nTexGen = c.texGen;
		info.nTevStage = c.tevStage;
		info.indirect = c.indStage;
		info.nInd = c.indStage;
	}


	gx::Color getMatColor(u32 idx) const override
	{
		if (idx < 2)
			return matColors[idx];

		return {};
	}
	virtual gx::Color getAmbColor(u32 idx) const override
	{
		if (idx < 2)
			return ambColors[idx];

		return {};
	}
	void setMatColor(u32 idx, gx::Color v) override
	{
		if (idx < 2)
			matColors[idx] = v;
	}
	void setAmbColor(u32 idx, gx::Color v) override
	{
		if (idx < 2)
			ambColors[idx] = v;
	}

};

}
