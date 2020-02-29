#pragma once

#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include <core/common.h>
#include <plugins/gc/Export/Material.hpp>

namespace kpi { class IDocumentNode; }

namespace riistudio::g3d {

// Note: In G3D this is per-material; in J3D it is per-texmatrix
enum class TexMatrixMode {
	Maya, XSI, Max
};

enum class G3dMappingMode {
	Standard,
	EnvCamera,
	Projection,
	EnvLight,
	EnvSpec
};

enum class G3dIndMethod {
	Warp,
	NormalMap,
	SpecNormalMap,
	Fur,
	Res0, Res1,
	User0, User1
};

struct G3dIndConfig {
	G3dIndMethod method { G3dIndMethod::Warp };
	s8 normalMapLightRef { -1 };

	bool operator==(const G3dIndConfig& rhs) const {
		return method == rhs.method && normalMapLightRef == rhs.normalMapLightRef;
	}
};

struct G3dMaterialData : public libcube::GCMaterialData {
	libcube::array_vector<G3dIndConfig, 4> indConfig;
	u32 flag;

	bool operator==(const G3dMaterialData& rhs) const {
		return libcube::GCMaterialData::operator==(rhs) && indConfig == rhs.indConfig && flag == rhs.flag;
	}
};

struct Material : public G3dMaterialData, public libcube::IGCMaterial {
	GCMaterialData& getMaterialData() override { return *this; }
	const GCMaterialData& getMaterialData() const override { return *this; }
	const libcube::Texture& getTexture(const std::string& id) const override;

	virtual const kpi::IDocumentNode* getParent() const { return nullptr; }

	bool operator==(const Material& rhs) const {
		return G3dMaterialData::operator==(rhs);
	}
	Material& operator=(const Material& rhs) {
		static_cast<libcube::GCMaterialData&>(*this) = static_cast<const libcube::GCMaterialData&>(rhs);
		return *this;
	}
};

}
