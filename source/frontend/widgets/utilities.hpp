#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <optional>

namespace riistudio::frontend {
void DrawItemBorder(const ImVec4& color, std::optional<ImRect> padding);
void DrawItemBorder(ImU32 color, std::optional<ImRect> padding);
}
