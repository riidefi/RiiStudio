#pragma once

#include "PropertySupport.hpp"
#include <Lib3D/interface/i3dmodel.hpp>
#include <LibCube/GX/Material.hpp>
#include <LibCore/common.h>

#include <LibCore/api/Node.hpp>

namespace libcube {

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
struct IMaterialDelegate : public lib3d::Material
{
    PX_TYPE_INFO("GC Material", "gc_mat", "GC::IMaterialDelegate");
    
    enum class Feature
    {
        CullMode,
        ZCompareLoc,
        ZCompare,
        GenInfo,

        MatAmbColor,

        Max
    };
    // Compat
    struct PropertySupport : public TPropertySupport<Feature>
    {
        using Feature = Feature;
        static constexpr std::array<const char*, (u64)Feature::Max> featureStrings = {
            "Culling Mode",
            "Early Z Comparison",
            "Z Comparison",
            "GenInfo",
            "Material/Ambient Colors"
        };
    };
    
    PropertySupport support;

    // Properties must be allocated in the delegate instance.
    virtual gx::CullMode getCullMode() const = 0;
    virtual void setCullMode(gx::CullMode value) = 0;

    struct GenInfoCounts { u8 colorChan, texGen, tevStage, indStage; };
    virtual GenInfoCounts getGenInfo() const = 0;
    virtual void setGenInfo(const GenInfoCounts& value) = 0;

    virtual bool getZCompLoc() const = 0;
    virtual void setZCompLoc(bool value) = 0;
    // Always assumed out of 2
    virtual gx::Color getMatColor(u64 idx) const = 0;
    virtual gx::Color getAmbColor(u64 idx) const = 0;
    virtual void setMatColor(u64 idx, gx::Color v) = 0;
    virtual void setAmbColor(u64 idx, gx::Color v) = 0;

    virtual std::string getName() const = 0;
    virtual void* getRaw() = 0; // For selection

	virtual gx::Shader& getShader() = 0;
	virtual array_vector<gx::TexCoordGen, 8>& getTexGens() = 0;

	virtual gx::AlphaComparison getAlphaComparison() = 0;
	virtual gx::IndirectTextureScalePair getIndScale(u64 idx) = 0;

	std::pair<std::string, std::string> generateShaders() const override;
	void generateUniforms(DelegatedUBOBuilder& builder, const glm::mat4& M, const glm::mat4& V, const glm::mat4& P) const override;

	virtual gx::BlendMode getBlendMode() = 0;

protected:
};

}
