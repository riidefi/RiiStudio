#pragma once

#include <LibRiiEditor/ui/Window.hpp>
#include <LibCube/JSystem/J3D/Collection.hpp>

namespace ed {


struct MaterialEditor final : public Window
{
	int selected = -1;
	void draw(WindowContext* ctx) noexcept override;

private:
	bool drawLeft(std::vector<libcube::GCCollection::IMaterialDelegate*>& ctx);
	bool drawRight(std::vector<libcube::GCCollection::IMaterialDelegate*>& ctx);

	bool drawGenInfoTab(std::vector<libcube::GCCollection::IMaterialDelegate*>& ctx);
};

}
