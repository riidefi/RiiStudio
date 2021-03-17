#pragma once

#include <bit>

namespace librii::kcol {

enum class Quantization {
  Float32, //!< IEE-754 floating single
  NitroFX  //!< DS fixed point values (32 and 16 bit)
};

struct SerializationProfile {
  std::endian endian{std::endian::big};
  Quantization quantization{Quantization::Float32};

  int major_revision{1};       //!< Only V1 is supported*
  float runtime_scale{100.0f}; //!< Property of the target game
};

enum class Platform { Nitro, Revolution, Citra, Cafe, NX };

constexpr SerializationProfile PlatformProfile(Platform platform) {
  switch (platform) {
  case Platform::Nitro:
    return SerializationProfile{.endian = std::endian::little,
                                .quantization = Quantization::NitroFX,
                                .major_revision = 1,
                                .runtime_scale = 10.0f};
  default:
  case Platform::Revolution:
    return SerializationProfile{.endian = std::endian::big,
                                .quantization = Quantization::Float32,
                                .major_revision = 1,
                                .runtime_scale = 100.0f};
  case Platform::Citra:
    return SerializationProfile{.endian = std::endian::little,
                                .quantization = Quantization::Float32,
                                .major_revision = 1,
                                .runtime_scale = 10.0f};
  case Platform::Cafe:
    return SerializationProfile{.endian = std::endian::big,
                                .quantization = Quantization::Float32,
                                .major_revision = 2,
                                .runtime_scale = 10.0f};
  case Platform::NX:
    return SerializationProfile{.endian = std::endian::little,
                                .quantization = Quantization::Float32,
                                .major_revision = 2,
                                .runtime_scale = 10.0f};
  }
}

} // namespace librii::kcol
