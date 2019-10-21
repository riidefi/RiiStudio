#pragma once

#include <LibCube/GX/Material.hpp>
#include <ThirdParty/imgui/imgui.h>

namespace ed {

struct CullMode
{
	bool front, back;

	CullMode()
		: front(true), back(false)
	{}
	CullMode(bool f, bool b)
		: front(f), back(b)
	{}
	CullMode(libcube::gx::CullMode c)
	{
		set(c);
	}

	void set(libcube::gx::CullMode c) noexcept
	{
		switch (c)
		{
		case libcube::gx::CullMode::All:
			front = back = false;
			break;
		case libcube::gx::CullMode::None:
			front = back = true;
			break;
		case libcube::gx::CullMode::Front:
			front = false;
			back = true;
			break;
		case libcube::gx::CullMode::Back:
			front = true;
			back = false;
			break;
		}
	}
	libcube::gx::CullMode get() const noexcept
	{
		if (front && back)
			return libcube::gx::CullMode::None;

		if (!front && !back)
			return libcube::gx::CullMode::All;

		return front ? libcube::gx::CullMode::Back : libcube::gx::CullMode::Front;
	}

	void draw()
	{
		ImGui::Text("Show sides of faces:");
		ImGui::Checkbox("Front", &front);
		ImGui::Checkbox("Back", &back);
	}
};

} // namespace ed
