#pragma once
#include "ColorRGBA.hpp"

namespace ImGG { namespace internal {

auto CIELAB_Premultiplied_from_sRGB_Straight(ColorRGBA const&) -> ImVec4;
auto sRGB_Straight_from_CIELAB_Premultiplied(ImVec4 const&) -> ColorRGBA;

}} // namespace ImGG::internal