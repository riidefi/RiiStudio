#pragma once

#include <librii/gx.h>
#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <vector>

namespace librii::egg {

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

  //! \brief Coordinate spaces for the light
  enum class CoordinateSpace {
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
    Directional,
    //! \brief Spot light
    Spot,
  };

  //! \brief Data for a single light
  //!
  //! Default values based on EGG::LightObject::reset() 8022B7C0
  struct LightObject {
    constexpr static inline f32 DEFAULT_SHININESS = 16.0f;

    u8 version = 2;
    librii::gx::SpotFn spotFunction{librii::gx::SpotFn::Off};
    librii::gx::DistAttnFn distAttnFunction{librii::gx::DistAttnFn::Off};
    CoordinateSpace coordSpace{CoordinateSpace::World};
    LightType lightType{LightType::Directional};
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
    float refBrightness{0.5f};
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

  Blight(oishii::BinaryReader& stream) {
    oishii::BinaryReader reader(stream);
    read(reader);
  }

  void save(oishii::Writer& stream) {
    auto& writer = stream;
    write(writer);
  }

  void read(oishii::BinaryReader& reader);

  void write(oishii::Writer& writer);
};

} // namespace librii::egg
