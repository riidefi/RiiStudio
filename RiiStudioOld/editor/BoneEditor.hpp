#pragma once

#include <LibRiiEditor/ui/Window.hpp>
#include <LibCube/JSystem/J3D/Collection.hpp>

namespace ed {


struct BoneEditor final : public Window
{
	int selected = -1;
	void drawBone(libcube::IBoneDelegate& d, bool leaf=false);
	void draw(WindowContext* ctx) noexcept override;

	BoneEditor(libcube::GCCollection& c)
		: samp(c)
	{}
	libcube::GCCollection& samp;
};

}
