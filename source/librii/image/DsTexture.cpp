#include "DsTexture.hpp"
#include <glm/gtc/type_ptr.hpp>

extern "C" {
#include <vendor/texconv/analysis.h>
#include <vendor/texconv/color.h>
#include <vendor/texconv/texconv.h>
}

IMPORT_STD;

namespace librii::image {

// analysis.c

// Drop cv qualifiers to access C api without const
static DWORD* Decay(std::span<const u32> span) {
  return reinterpret_cast<DWORD*>(const_cast<u32*>(span.data()));
}

u32 ComputeAverageColor(std::span<const u32> colors) {
  return getAverageColor(Decay(colors), colors.size());
}

glm::vec3 ComputePrincipalComponent(std::span<const u32> colors) {
  glm::vec3 result;
  getPrincipalComponent(Decay(colors), colors.size(), glm::value_ptr(result));
  return result;
}

std::array<u32, 2> ComputeColorEndpoints(std::span<const u32> colors) {
  std::array<u32, 2> result;
  getColorEndPoints(Decay(colors), colors.size(),
                    reinterpret_cast<DWORD*>(result.data()));
  return result;
}

// texconv.c

struct WrappedCreateParams {
  WrappedCreateParams() = default;
  WrappedCreateParams(const WrappedCreateParams&) = delete;
  WrappedCreateParams(WrappedCreateParams&&) = delete;

  ~WrappedCreateParams() {
    // If the C API forgot to free it, let's handle that
    if (custom_palette)
      free(custom_palette);
  }

  CREATEPARAMS _internal;
  COLOR* custom_palette = nullptr; // freed by C api with free()

  TEXTURE _internal_dest;
};

std::unique_ptr<WrappedCreateParams> CreateConvertParams(
    std::span<const u32> pixels, u32 width, u32 height, DsFormat format,
    DitherMode dither, u32 palette_size,
    std::optional<std::span<const u32>> custom_palette = std::nullopt,
    float yuv_merge_threshhold = 0.0f) {
  if (format == DsFormat::DS_4x4 && custom_palette.has_value()) {
    assert(palette_size % 8 == 0);
    assert(palette_size >= 16);
  }

  auto result = std::make_unique<WrappedCreateParams>();

  if (custom_palette.has_value()) {
    // The C code will free() this, so we use malloc
    if (result->custom_palette)
      free(result->custom_palette);
    result->custom_palette = reinterpret_cast<COLOR*>(
        malloc(sizeof(COLOR) * custom_palette->size()));
    for (size_t i = 0; i < custom_palette->size(); ++i) {
      result->custom_palette[i] = ColorConvertToDS((*custom_palette)[i]);
    }
  }

  result->_internal = CREATEPARAMS{
      .px = Decay(pixels),
      .width = static_cast<int>(width),
      .height = static_cast<int>(height),
      .fmt = static_cast<int>(format),
      .dither = dither != DitherMode::None,
      .ditherAlpha = dither == DitherMode::ColorAlpha,
      .colorEntries = static_cast<int>(palette_size),
      .useFixedPalette = custom_palette.has_value() ? 1 : 0,
      .fixedPalette =
          custom_palette.has_value() ? result->custom_palette : nullptr,
      .threshold = static_cast<int>(yuv_merge_threshhold * 97537.5f),
      .dest = &result->_internal_dest,
      .callback = nullptr,
      .callbackParam = nullptr};

  strcpy(result->_internal.pnam, "Wau");

  return result;
}

static std::mutex sConvertMutex;

std::optional<DsTexturePalette>
ConvertToDS(std::span<const u32> pixels, u32 width, u32 height, DsFormat format,
            DitherMode dither, u32 palette_size,
            std::optional<std::span<const u32>> custom_palette,
            float yuv_merge_threshhold) {
  assert(pixels.size() == width * height * 4);

  assert((width & (width - 1)) == 0 && "Width must be a power of two");
  assert((height & (height - 1)) == 0 && "Height must be a power of two");
  assert(width >= 8 && width <= 1024 && "Width must be [8, 1024]");
  assert(height >= 8 && height <= 1024 && "Height must be [8, 1024]");

  auto params =
      CreateConvertParams(pixels, width, height, format, dither, palette_size,
                          custom_palette, yuv_merge_threshhold);

  {
    // Because of the _glob* externs, we can't convert multiple images at once
    std::scoped_lock g(sConvertMutex);

    startConvert(&params->_internal);
  }

  auto& c_texels = params->_internal_dest.texels;

  assert(c_texels.texel);
  assert(c_texels.cmp);
  DsTexture tex{
      .pixel_data = {c_texels.texel, c_texels.texel + (width * height / 4)},
      .index_data = {c_texels.cmp, c_texels.cmp + (width * height / 2)},
      .ds_param = c_texels.texImageParam,
  };

  free(params->_internal_dest.texels.texel);
  free(params->_internal_dest.texels.cmp);
  free(params->_internal_dest.palette.pal);

  auto& c_palette = params->_internal_dest.palette;

  assert(c_palette.pal);
  DsPalette pal{
      .color_data = {c_palette.pal, c_palette.pal + c_palette.nColors}};

  free(c_palette.pal);

  return DsTexturePalette{.texture = std::move(tex), .palette = std::move(pal)};
}

} // namespace librii::image
