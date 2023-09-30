#include "color_conversions.hpp"
#include <cmath>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.hpp"

namespace ImGG { namespace internal {

namespace {
struct Vec3 {
    float x;
    float y;
    float z;
};
} // namespace

static auto clamp(float x, float min, float max) -> float
{
    return x < min
               ? min
           : x > max
               ? max
               : x;
}

static auto saturate(Vec3 const& v) -> Vec3
{
    return {
        clamp(v.x, 0.f, 1.f),
        clamp(v.y, 0.f, 1.f),
        clamp(v.z, 0.f, 1.f),
    };
}

// Start of [Block1]
// From https://entropymine.com/imageworsener/srgbformula/

static auto LinearRGB_from_sRGB_impl(float x) -> float
{
    return x < 0.04045f
               ? x / 12.92f
               : std::pow((x + 0.055f) / 1.055f, 2.4f);
}

static auto sRGB_from_LinearRGB_impl(float x) -> float
{
    return x < 0.0031308f
               ? x * 12.92f
               : 1.055f * std::pow(x, 1.f / 2.4f) - 0.055f;
}

static auto LinearRGB_from_sRGB(Vec3 srgb) -> Vec3
{
    srgb = saturate(srgb);
    return {
        LinearRGB_from_sRGB_impl(srgb.x),
        LinearRGB_from_sRGB_impl(srgb.y),
        LinearRGB_from_sRGB_impl(srgb.z),
    };
}

static auto sRGB_from_LinearRGB(Vec3 rgb) -> Vec3
{
    rgb = saturate(rgb);
    return {
        sRGB_from_LinearRGB_impl(rgb.x),
        sRGB_from_LinearRGB_impl(rgb.y),
        sRGB_from_LinearRGB_impl(rgb.z),
    };
}
// End of [Block1]

// Start of [Block2]
// From http://www.easyrgb.com/en/math.php

static auto XYZ_from_LinearRGB(Vec3 const& c) -> Vec3
{
    return {
        c.x * 0.4124f + c.y * 0.3576f + c.z * 0.1805f,
        c.x * 0.2126f + c.y * 0.7152f + c.z * 0.0722f,
        c.x * 0.0193f + c.y * 0.1192f + c.z * 0.9505f,
    };
}

static auto XYZ_from_sRGB(Vec3 const& c) -> Vec3
{
    return XYZ_from_LinearRGB(LinearRGB_from_sRGB(c));
}

static auto CIELAB_from_XYZ(Vec3 const& c) -> Vec3
{
    auto const n = Vec3{c.x / 0.95047f, c.y, c.z / 1.08883f};
    Vec3 const v{
        (n.x > 0.008856) ? std::pow(n.x, 1.f / 3.f) : (7.787f * n.x) + (16.f / 116.f),
        (n.y > 0.008856) ? std::pow(n.y, 1.f / 3.f) : (7.787f * n.y) + (16.f / 116.f),
        (n.z > 0.008856) ? std::pow(n.z, 1.f / 3.f) : (7.787f * n.z) + (16.f / 116.f),
    };
    return {
        (1.16f * v.y) - 0.16f,
        5.f * (v.x - v.y),
        2.f * (v.y - v.z),
    };
}

static auto CIELAB_from_sRGB(Vec3 const& c) -> Vec3
{
    return CIELAB_from_XYZ(XYZ_from_sRGB(c));
}

static auto XYZ_from_CIELAB(Vec3 const& c) -> Vec3
{
    float const fy = (c.x + 0.16f) / 1.16f;
    float const fx = c.y / 5.f + fy;
    float const fz = fy - c.z / 2.f;

    float const fx3 = fx * fx * fx;
    float const fy3 = fy * fy * fy;
    float const fz3 = fz * fz * fz;
    return {
        0.95047f * ((fx3 > 0.008856f) ? fx3 : (fx - 16.f / 116.f) / 7.787f),
        1.00000f * ((fy3 > 0.008856f) ? fy3 : (fy - 16.f / 116.f) / 7.787f),
        1.08883f * ((fz3 > 0.008856f) ? fz3 : (fz - 16.f / 116.f) / 7.787f),
    };
}

static auto LinearRGB_from_XYZ(Vec3 const& c) -> Vec3
{
    return {
        c.x * 3.2406f + c.y * -1.5372f + c.z * -0.4986f,
        c.x * -0.9689f + c.y * 1.8758f + c.z * 0.0415f,
        c.x * 0.0557f + c.y * -0.2040f + c.z * 1.0570f,
    };
}

static auto sRGB_from_XYZ(Vec3 const& c) -> Vec3
{
    return sRGB_from_LinearRGB(LinearRGB_from_XYZ(c));
}

static auto sRGB_from_CIELAB(Vec3 const& c) -> Vec3
{
    return sRGB_from_XYZ(XYZ_from_CIELAB(c));
}

// End of [Block2]

auto CIELAB_Premultiplied_from_sRGB_Straight(ColorRGBA const& col) -> ImVec4
{
    auto const lab = CIELAB_from_sRGB({col.x, col.y, col.z});
    return {
        lab.x * col.w,
        lab.y * col.w,
        lab.z * col.w,
        col.w,
    };
}

auto sRGB_Straight_from_CIELAB_Premultiplied(ImVec4 const& col) -> ColorRGBA
{
    auto const srgb = sRGB_from_CIELAB({col.x / col.w, col.y / col.w, col.z / col.w});
    return {
        srgb.x,
        srgb.y,
        srgb.z,
        col.w,
    };
}

}} // namespace ImGG::internal