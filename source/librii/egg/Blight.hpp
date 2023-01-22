#pragma once

#include <librii/gx.h>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <rsl/Ranges.hpp>
#include <vector>

namespace librii::egg {

//! \brief Coordinate spaces for the light
enum class CoordinateSpace {
  _0,
  //! \brief World coordinate space
  World,
  //! \brief View coordinate space
  View,
  //! \brief Top orthographic coordinate space
  TopOrtho,
  //! \brief Bottom orthographic coordinate space
  BottomOrtho,
};

//! \brief Light types
enum class LightType {
  //! \brief Point light
  Point,
  //! \brief Directional light
  Sun,
  //! \brief Spot light
  Spot,
};

//! \brief EGG::LightManager
struct Blight {
  //! \brief Flags for the light
  enum Flag {
    //! \brief If not set, only initializes light color to pure black and
    //! disables calculations
    FLAG_ENABLED = (1 << 0),
    //! \brief Bind position/aim to another light
    FLAG_SNAP_TO = (1 << 1),
    FLAG_4 = (1 << 2),
    FLAG_8 = (1 << 3),
    FLAG_16 = (1 << 4),
    FLAG_USE_BLMAP = (1 << 5),
    //! \brief Unknown. If not set, only initializes light color to pure black
    FLAG_ENABLED_2 = (1 << 6),
    //! \brief When enabled, use GXInitLightSpot rather than GXInitLightAttnA
    FLAG_SPOTLIGHT = (1 << 7),
    //! \brief When enabled, use manual k0, k1, k2 for distance attenuation
    //! (GXInitLightAttnK). Otherwise, use GXInitLightDistAttn.
    FLAG_MANUAL_DISTANCE_ATTN = (1 << 8),
    //! \brief 8022BF70 - unset, set disable color flag in g3d lightobject.
    //! Doesn't affect player.
    FLAG_ENABLE_G3D_LIGHTCOLOR = (1 << 9),
    //! \brief 8022BF8C - when unset, set disable alpha flag in g3d lightobject.
    //! Doesn't affect player.
    FLAG_ENABLE_G3D_LIGHTALPHA = (1 << 10),
    //! \brief When set, overwrite previous settings with GXInitLightShininess
    //! based on mShininess (default = 16.0f). Affects GX and G3D.
    FLAG_USE_SHININESS = (1 << 11),
  };

  using CoordinateSpace = librii::egg::CoordinateSpace;
  using LightType = librii::egg::LightType;

  //! \brief Data for a single light
  //!
  //! Default values based on EGG::LightObject::reset() 8022B7C0
  struct LightObject {
    constexpr static inline f32 DEFAULT_SHININESS = 16.0f;

    u8 version = 2;
    librii::gx::SpotFn spotFunction{librii::gx::SpotFn::Off};
    librii::gx::DistAttnFn distAttnFunction{librii::gx::DistAttnFn::Off};
    CoordinateSpace coordSpace{CoordinateSpace::World};
    LightType lightType{LightType::Sun};
    u16 ambientLightIndex{0};
    u16 flags{FLAG_ENABLED | FLAG_USE_BLMAP | FLAG_ENABLED_2 |
              FLAG_ENABLE_G3D_LIGHTCOLOR | FLAG_ENABLE_G3D_LIGHTALPHA};
    glm::vec3 position{-10000.0, 10000.0, 10000.0};
    glm::vec3 aim{0.0f, 0.0f, 0.0f};
    float intensity{1.0f};
    librii::gx::Color color{0xFF, 0xFF, 0xFF, 0xFF};
    librii::gx::Color specularColor{0x00, 0x00, 0x00, 0xFF};

    float spotCutoffAngle{90.0f}; // Degrees
    float refDist{0.5f};
    float refBright{0.5f};
    u16 snapTargetIndex{0};
  };

  //! \brief Data for a single ambient light
  struct AmbientObject {
    librii::gx::Color color{0x64, 0x64, 0x64, 0xFF};
    std::array<u8, 4> reserved{};
  };

  std::vector<LightObject> lightObjects;
  std::vector<AmbientObject> ambientObjects;

  u8 version = 2;
  std::array<u8, 7> reserved{};
  //! \brief Background color
  librii::gx::Color backColor{0, 0, 0, 0xFF};

  Blight() = default;

  void save(oishii::Writer& stream) {
    auto& writer = stream;
    write(writer);
  }

  Result<void> read(oishii::BinaryReader& reader);

  void write(oishii::Writer& writer);
};

// Higher-level xml-suitable representation of a blight object
struct Light {
  bool enable_calc = false;
  bool enable_sendToGpu = false;

  librii::egg::Blight::LightType lightType{librii::egg::Blight::LightType::Sun};
  glm::vec3 position{-10000.0, 10000.0, 10000.0}; // If !LightType::Sun
  glm::vec3 aim{0.0f, 0.0f, 0.0f};                // If !LightType::Point
  std::optional<u16> snapTargetIndex{}; // {FLAG_SNAP_TO, snapTargetIndex}

  librii::egg::Blight::CoordinateSpace coordSpace{
      librii::egg::Blight::CoordinateSpace::World};

  bool blmap = true;
  bool brresColor = true;
  bool brresAlpha = true;

  // Ovwrite GXInitLightDistAttn(refDist, refBrightness, distAttnFunction) with
  // GXInitLightAttnK with default params
  bool runtimeDistAttn = false; // !FLAG_MANUAL_KATTN -> Distance coefficients
  float refDist{0.5f};
  float refBright{0.5f};
  librii::gx::DistAttnFn distAttnFunction{librii::gx::DistAttnFn::Off};

  // Simplification: if lightType == Point
  bool runtimeAngularAttn = false; // !FLAG_SPOTLIGHT
  float spotCutoffAngle{90.0f};    // Degrees
  librii::gx::SpotFn spotFunction{librii::gx::SpotFn::Off};

  // 16.0f by default
  bool runtimeShininess =
      false; // FLAG_USE_SHININESS -> Distance + Angular coefficients

  u16 miscFlags{}; // FLAG_4, FLAG_8, FLAG_16, any above bit 11

  float intensity{1.0f};
  librii::gx::Color color{0xFF, 0xFF, 0xFF, 0xFF};
  librii::gx::Color specularColor{0x00, 0x00, 0x00, 0xFF};

  u16 ambientLightIndex{0};

  bool operator==(const Light&) const = default;

  constexpr bool hasPosition() const {
    return !snapTargetIndex.has_value() &&
           lightType != librii::egg::Blight::LightType::Sun;
  }
  constexpr bool hasAim() const {
    return !snapTargetIndex.has_value() &&
           lightType != librii::egg::Blight::LightType::Point;
  }
  constexpr bool hasDistAttn() const {
    return !runtimeShininess && !runtimeDistAttn;
  }
  constexpr bool hasAngularAttn() const {
    return !runtimeShininess && !runtimeAngularAttn;
  }

  // Bits we unpack/pack
  static constexpr inline u16 usedBits =
      ((1 << 12) - 1) &
      ~(librii::egg::Blight::FLAG_4 | librii::egg::Blight::FLAG_8 |
        librii::egg::Blight::FLAG_16);

  Result<void> from(const librii::egg::Blight::LightObject& lobj) {
    enable_calc = lobj.flags & librii::egg::Blight::FLAG_ENABLED;
    enable_sendToGpu = lobj.flags & librii::egg::Blight::FLAG_ENABLED_2;
    lightType = lobj.lightType;
    position = lobj.position;
    aim = lobj.aim;
    if (lobj.flags & librii::egg::Blight::FLAG_SNAP_TO) {
      snapTargetIndex.emplace(lobj.snapTargetIndex);
    }
    coordSpace = lobj.coordSpace;
    blmap = lobj.flags & librii::egg::Blight::FLAG_USE_BLMAP;
    brresColor = lobj.flags & librii::egg::Blight::FLAG_ENABLE_G3D_LIGHTCOLOR;
    brresAlpha = lobj.flags & librii::egg::Blight::FLAG_ENABLE_G3D_LIGHTALPHA;
    runtimeDistAttn =
        lobj.flags & librii::egg::Blight::FLAG_MANUAL_DISTANCE_ATTN;
    refDist = lobj.refDist;
    refBright = lobj.refBright;
    distAttnFunction = lobj.distAttnFunction;
    runtimeAngularAttn = !(lobj.flags & librii::egg::Blight::FLAG_SPOTLIGHT);
    spotCutoffAngle = lobj.spotCutoffAngle;
    spotFunction = lobj.spotFunction;
    runtimeShininess = lobj.flags & librii::egg::Blight::FLAG_USE_SHININESS;
    miscFlags = lobj.flags & ~usedBits;
    intensity = lobj.intensity;
    color = lobj.color;
    specularColor = lobj.specularColor;
    ambientLightIndex = lobj.ambientLightIndex;
    return {};
  }
  void to(librii::egg::Blight::LightObject& lobj) const {
    using F = librii::egg::Blight::Flag;
    u16 flags = miscFlags & ~usedBits;
    if (enable_calc)
      flags |= F::FLAG_ENABLED;
    if (enable_sendToGpu)
      flags |= F::FLAG_ENABLED_2;
    if (snapTargetIndex.has_value())
      flags |= F::FLAG_SNAP_TO;
    if (blmap)
      flags |= F::FLAG_USE_BLMAP;
    if (brresColor)
      flags |= F::FLAG_ENABLE_G3D_LIGHTCOLOR;
    if (brresAlpha)
      flags |= F::FLAG_ENABLE_G3D_LIGHTALPHA;
    if (runtimeDistAttn)
      flags |= F::FLAG_MANUAL_DISTANCE_ATTN;
    if (!runtimeAngularAttn)
      flags |= F::FLAG_SPOTLIGHT;
    if (runtimeShininess)
      flags |= F::FLAG_USE_SHININESS;
    lobj = {
        .version = 2,
        .spotFunction = spotFunction,
        .distAttnFunction = distAttnFunction,
        .coordSpace = coordSpace,
        .lightType = lightType,
        .ambientLightIndex = ambientLightIndex,
        .flags = flags,
        .position = position,
        .aim = aim,
        .intensity = intensity,
        .color = color,
        .specularColor = specularColor,
        .refDist = refDist,
        .refBright = refBright,
        .snapTargetIndex = snapTargetIndex.value_or(0),
    };
  }
};
struct LightSet {
  std::vector<Light> lights;
  std::vector<librii::gx::Color> ambientColors;
  librii::gx::Color backColor; // ?

  bool operator==(const LightSet&) const = default;

  Result<void> from(const librii::egg::Blight& manager) {
    backColor = manager.backColor;
    ambientColors = manager.ambientObjects |
                    std::views::transform([](auto& x) { return x.color; }) |
                    rsl::ToList();
    for (auto& lobj : manager.lightObjects) {
      auto& l = lights.emplace_back();
      TRY(l.from(lobj));
    }
    return {};
  }
  void to(librii::egg::Blight& manager) const {
    manager.version = 2;
    manager.backColor = backColor;
    manager.ambientObjects =
        ambientColors | std::views::transform([](auto& x) {
          return librii::egg::Blight::AmbientObject{.color = x};
        }) |
        rsl::ToList();
    manager.lightObjects = lights | std::views::transform([](auto& x) {
                             librii::egg::Blight::LightObject o;
                             x.to(o);
                             return o;
                           }) |
                           rsl::ToList();
  }
};

} // namespace librii::egg
