#pragma once

namespace librii::gx {

enum class ColorSource { Register, Vertex };
enum class LightID {
  None = 0,
  Light0 = 0x001,
  Light1 = 0x002,
  Light2 = 0x004,
  Light3 = 0x008,
  Light4 = 0x010,
  Light5 = 0x020,
  Light6 = 0x040,
  Light7 = 0x080
};
enum DiffuseFunction { None, Sign, Clamp };

enum class AttenuationFunction {
  Specular,
  Spotlight,
  None,
  // Necessary for J3D compatibility at the moment.
  // Really we're looking at SpecDisabled/SpotDisabled there (2 adjacent bits in
  // HW field)
  None2
};

struct ChannelControl {
  bool enabled = false;
  ColorSource Ambient = ColorSource::Register;
  ColorSource Material = ColorSource::Register;
  LightID lightMask = LightID::None;
  DiffuseFunction diffuseFn = DiffuseFunction::None;
  AttenuationFunction attenuationFn = AttenuationFunction::None;

  bool operator==(const ChannelControl& other) const = default;
};

struct ChannelData {
  gx::Color matColor{0xff, 0xff, 0xff, 0xff};
  gx::Color ambColor{0, 0, 0, 0xff};

  bool operator==(const ChannelData& rhs) const = default;
};

} // namespace librii::gx
