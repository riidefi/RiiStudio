#pragma once

#include <LibRiiEditor/ui/Window.hpp>
#include <LibCube/JSystem/J3D/Collection.hpp>

namespace ed {


struct MaterialEditor final : public Window
{
	using MatDelegate = libcube::GCCollection::IMaterialDelegate;
	int selected = -1;
	void draw(WindowContext* ctx) noexcept override;

private:
	bool drawLeft(std::vector<MatDelegate*>& ctx);
	bool drawRight(std::vector<MatDelegate*>& ctx, MatDelegate& active);

	bool drawGenInfoTab(std::vector<MatDelegate*>& ctx, MatDelegate& active);
	bool drawColorTab(std::vector<MatDelegate*>& ctx, MatDelegate& active);
};

}
