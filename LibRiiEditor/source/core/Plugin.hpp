#pragma once

#include "pluginapi/Plugin.hpp"
#include "ui/widgets/Outliner.hpp"

struct EditorWindow : public WindowManager, public Window
{
	~EditorWindow() override = default;
	EditorWindow(const pl::FileEditor& registration)
		: mEditor(registration)
	{}

	void draw(WindowContext* ctx) noexcept override final
	{
		if (!ctx)
			return;

		if (ImGui::Begin("EditorWindow", &bOpen))
		{
			ImGui::Text("Extensions");
			for (const auto& str : mEditor.mExtensions)
				ImGui::Text(str.c_str());
			ImGui::Text("Magics");
			for (const auto m : mEditor.mMagics)
				ImGui::Text("%c%c%c%c", m & 0xff000000, (m & 0x00ff0000) >> 8, (m & 0x0000ff00) >> 16, (m & 0xff) >> 24);
			ImGui::Text("Interfaces");
			for (const auto& str : mEditor.mInterfaces)
				ImGui::Text(std::to_string(static_cast<u32>(str->mInterfaceId)).c_str());
			ImGui::End();

			// TODO: Interface handling

		}

		for (const auto it : mEditor.mInterfaces)
		{
			switch (it->mInterfaceId)
			{
			case pl::InterfaceID::TextureList:
				if (ImGui::Begin("TextureList"))
				{
					pl::ITextureList* pTexList = static_cast<pl::ITextureList*>(it);

					for (int i = 0; i < pTexList->getNumTex(); ++i)
					{
						ImGui::Text(pTexList->getNameAt(i).c_str());
					}
					ImGui::End();
				}
			}
		}

		// Check for IRenderable

		// Fill in top bar, check for IExtendedBar

		// Draw childen
		drawWindows();
	}

	const pl::FileEditor& mEditor;
};
