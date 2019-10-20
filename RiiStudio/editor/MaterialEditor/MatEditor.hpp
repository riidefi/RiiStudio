#pragma once

#include <LibRiiEditor/ui/Window.hpp>

namespace ed {


struct MaterialEditor final : public Window
{
	int selected = -1;
	void draw(WindowContext* ctx) noexcept override;
};

}
