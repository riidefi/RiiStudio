#pragma once

#include <ThirdParty/imgui/imgui.h>
#include <cmath>
#include <array>

class ThemeManager
{
public:
#pragma region Basic Themes

	enum class BasicTheme
	{
		// Basic Themes
		ImClassic,
		ImDark,
		ImLight,
		UE4ish,
		Raikiri,
		Adaptive,
		Num,

		Default = UE4ish
	};

	//	std::array<const char, static_cast<int>(BasicTheme::Num)> ThemeNames
	//	{
	//		"Classic",
	//		"Dark",
	//		"Light",
	//		"Not Quite UE4",
	//		"Raikiri",
	//		"Adaptive"
	//	};
	static inline const char* ThemeNames = "Classic\0Dark\0Light\0Not Quite UE4\0Raikiri\0Adaptive\0";


	template<BasicTheme theme>
	static inline void setTheme()
	{}

	static inline void setTheme(BasicTheme theme)
	{
		switch (theme)
		{
		case BasicTheme::ImClassic:
			setTheme<BasicTheme::ImClassic>();
			break;
		case BasicTheme::ImDark:
			setTheme<BasicTheme::ImDark>();
			break;
		case BasicTheme::ImLight:
			setTheme<BasicTheme::ImLight>();
			break;
		case BasicTheme::UE4ish:
			setTheme<BasicTheme::UE4ish>();
			break;
		case BasicTheme::Raikiri:
			setTheme<BasicTheme::Raikiri>();
			break;
		}
	}

	// Support for adaptive theme requires nonstatic
	inline void setThemeEx(BasicTheme theme)
	{
		if (theme == BasicTheme::Adaptive)
			setThemeAdaptive();
		else
			setTheme(theme);
	}

	template<>
	static inline void setTheme<BasicTheme::ImClassic>()
	{
		ImGui::StyleColorsClassic();
	}

	template<>
	static inline void setTheme<BasicTheme::ImDark>()
	{
		ImGui::StyleColorsDark();
	}

	template<>
	static inline void setTheme<BasicTheme::ImLight>()
	{
		ImGui::StyleColorsLight();
	}

	template<>
	static inline void setTheme<BasicTheme::UE4ish>()
	{
		// https://github.com/ocornut/imgui/issues/707#issuecomment-415097227
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
		colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.70f, 0.70f, 0.70f, 0.31f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
		colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	}

	template<>
	static inline void setTheme<BasicTheme::Raikiri>()
	{
		// https://github.com/ocornut/imgui/issues/707#issuecomment-512669512

		//imGuiIO.Fonts->AddFontFromFileTTF("../data/Fonts/Ruda-Bold.ttf", 15.0f, &config);
		//ImGui::GetStyle().FrameRounding = 4.0f;
		//ImGui::GetStyle().GrabRounding = 4.0f;

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}

#pragma endregion

#pragma region Adaptive Theme
	// Based on https://github.com/ocornut/imgui/issues/707#issuecomment-508691523

	enum Colors
	{
		Black = 0x00000000,
		White = 0xFFFFFF00,
		AlphaTransparent = 0x00,
		Alpha20 = 0x33,
		Alpha40 = 0x66,
		Alpha50 = 0x80,
		Alpha60 = 0x99,
		Alpha80 = 0xCC,
		Alpha90 = 0xE6,
		AlphaFull = 0xFF
	};

	int BackGroundColor = 0x25213100;
	int TextColor = 0xF4F1DE00;
	int MainColor = 0xDA115E00;
	int MainAccentColor = 0x79235900;
	int HighlightColor = 0xC7EF0000;

	static constexpr float GetR(int colorCode) { return (float)((colorCode & 0xFF000000) >> 24) / (float)(0xFF); }
	static constexpr float GetG(int colorCode) { return (float)((colorCode & 0x00FF0000) >> 16) / (float)(0xFF); }
	static constexpr float GetB(int colorCode) { return (float)((colorCode & 0x0000FF00) >> 8) / (float)(0xFF); }
	static constexpr float GetA(int alphaCode) { return ((float)alphaCode / (float)0xFF); }

	// TODO: Make this all constexpr

	static ImVec4 GetColor(int c, int a = Alpha80) { return ImVec4(GetR(c), GetG(c), GetB(c), GetA(a)); }
	static ImVec4 Darken(ImVec4 c, float p) { return ImVec4(fmax(0.f, c.x - 1.0f * p), fmax(0.f, c.y - 1.0f * p), fmax(0.f, c.z - 1.0f *p), c.w); }
	static ImVec4 Lighten(ImVec4 c, float p) { return ImVec4(fmax(0.f, c.x + 1.0f * p), fmax(0.f, c.y + 1.0f * p), fmax(0.f, c.z + 1.0f *p), c.w); }

	static ImVec4 Disabled(ImVec4 c) { return Darken(c, 0.6f); }
	static ImVec4 Hovered(ImVec4 c) { return Lighten(c, 0.2f); }
	static ImVec4 Active(ImVec4 c) { return Lighten(ImVec4(c.x, c.y, c.z, 1.0f), 0.1f); }
	static ImVec4 Collapsed(ImVec4 c) { return Darken(c, 0.2f); }

	inline void SetColors(int backGroundColor, int textColor, int mainColor, int mainAccentColor, int highlightColor)
	{
		BackGroundColor = backGroundColor;
		TextColor = textColor;
		MainColor = mainColor;
		MainAccentColor = mainAccentColor;
		HighlightColor = highlightColor;
	}

	inline void setThemeAdaptive()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;

		colors[ImGuiCol_Text] = GetColor(TextColor);
		colors[ImGuiCol_TextDisabled] = Disabled(colors[ImGuiCol_Text]);
		colors[ImGuiCol_WindowBg] = GetColor(BackGroundColor);
		colors[ImGuiCol_ChildBg] = GetColor(Black, Alpha20);
		colors[ImGuiCol_PopupBg] = GetColor(BackGroundColor, Alpha90);
		colors[ImGuiCol_Border] = Lighten(GetColor(BackGroundColor), 0.4f);
		colors[ImGuiCol_BorderShadow] = GetColor(Black);
		colors[ImGuiCol_FrameBg] = GetColor(MainAccentColor, Alpha40);
		colors[ImGuiCol_FrameBgHovered] = Hovered(colors[ImGuiCol_FrameBg]);
		colors[ImGuiCol_FrameBgActive] = Active(colors[ImGuiCol_FrameBg]);
		colors[ImGuiCol_TitleBg] = GetColor(BackGroundColor, Alpha80);
		colors[ImGuiCol_TitleBgActive] = Active(colors[ImGuiCol_TitleBg]);
		colors[ImGuiCol_TitleBgCollapsed] = Collapsed(colors[ImGuiCol_TitleBg]);
		colors[ImGuiCol_MenuBarBg] = Darken(GetColor(BackGroundColor), 0.2f);
		colors[ImGuiCol_ScrollbarBg] = Lighten(GetColor(BackGroundColor, Alpha50), 0.4f);
		colors[ImGuiCol_ScrollbarGrab] = Lighten(GetColor(BackGroundColor), 0.3f);
		colors[ImGuiCol_ScrollbarGrabHovered] = Hovered(colors[ImGuiCol_ScrollbarGrab]);
		colors[ImGuiCol_ScrollbarGrabActive] = Active(colors[ImGuiCol_ScrollbarGrab]);
		colors[ImGuiCol_CheckMark] = GetColor(HighlightColor);
		colors[ImGuiCol_SliderGrab] = GetColor(HighlightColor);
		colors[ImGuiCol_SliderGrabActive] = Active(colors[ImGuiCol_SliderGrab]);
		colors[ImGuiCol_Button] = GetColor(MainColor, Alpha80);
		colors[ImGuiCol_ButtonHovered] = Hovered(colors[ImGuiCol_Button]);
		colors[ImGuiCol_ButtonActive] = Active(colors[ImGuiCol_Button]);
		colors[ImGuiCol_Header] = GetColor(MainAccentColor, Alpha80);
		colors[ImGuiCol_HeaderHovered] = Hovered(colors[ImGuiCol_Header]);
		colors[ImGuiCol_HeaderActive] = Active(colors[ImGuiCol_Header]);
		colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered] = Hovered(colors[ImGuiCol_Separator]);
		colors[ImGuiCol_SeparatorActive] = Active(colors[ImGuiCol_Separator]);
		colors[ImGuiCol_ResizeGrip] = GetColor(MainColor, Alpha20);
		colors[ImGuiCol_ResizeGripHovered] = Hovered(colors[ImGuiCol_ResizeGrip]);
		colors[ImGuiCol_ResizeGripActive] = Active(colors[ImGuiCol_ResizeGrip]);
		colors[ImGuiCol_Tab] = GetColor(MainColor, Alpha60);
		colors[ImGuiCol_TabHovered] = Hovered(colors[ImGuiCol_Tab]);
		colors[ImGuiCol_TabActive] = Active(colors[ImGuiCol_Tab]);
		colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab];
		colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabActive];
		colors[ImGuiCol_DockingPreview] = Darken(colors[ImGuiCol_HeaderActive], 0.2f);
		colors[ImGuiCol_DockingEmptyBg] = Darken(colors[ImGuiCol_HeaderActive], 0.6f);
		colors[ImGuiCol_PlotLines] = GetColor(HighlightColor);
		colors[ImGuiCol_PlotLinesHovered] = Hovered(colors[ImGuiCol_PlotLines]);
		colors[ImGuiCol_PlotHistogram] = GetColor(HighlightColor);
		colors[ImGuiCol_PlotHistogramHovered] = Hovered(colors[ImGuiCol_PlotHistogram]);
		colors[ImGuiCol_TextSelectedBg] = GetColor(HighlightColor, Alpha40);
		colors[ImGuiCol_DragDropTarget] = GetColor(HighlightColor, Alpha80);;
		colors[ImGuiCol_NavHighlight] = GetColor(White);
		colors[ImGuiCol_NavWindowingHighlight] = GetColor(White, Alpha80);
		colors[ImGuiCol_NavWindowingDimBg] = GetColor(White, Alpha20);
		colors[ImGuiCol_ModalWindowDimBg] = GetColor(Black, Alpha60);

		ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_Right;
	}
#pragma endregion
};
