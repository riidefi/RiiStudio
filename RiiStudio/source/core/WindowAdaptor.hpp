#pragma once

#include <ui/Window.hpp>
#include <string>


/*
Adapts between a core window:
interface CoreWindow
{
	const char* mTitle = "Title";
	void windowDraw(CoreContext& ctx) noexcept;
};

*/

template <class T>
struct WindowAdaptor : public Window
{
	~WindowAdaptor() override = default;
	WindowAdaptor() = default;

	void draw(WindowContext* ctx) noexcept
	{
		if (ImGui::Begin((std::string(mWrapped.mTitle) + std::string("###") + std::to_string(mId)).c_str(), &bOpen))
		{
			if (ctx)
				mWrapped.windowDraw(*ctx);
#ifdef DEBUG
			else
				ImGui::Text("DEBUG: Invalid context.");
#endif
		}
		ImGui::End();
	}

	T mWrapped;
};
