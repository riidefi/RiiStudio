#pragma once

#include <core/common.h>

#include <optional>
#include <tuple>

#include <librii/gx.h>

namespace librii::image {

//! @brief Compute padded dimensions for an image.
//!
//! @param[in] width  Width of the image.
//! @param[in] height Height of the image.
//! @param[in] format Format of the image. All formats except RGBA8 have 32 byte
//! blocks, the exception being twice that.
//!
//! @return Dimensions rounded up blocked size.
//!
[[nodiscard]] std::pair<u32, u32>
getBlockedDimensions(u32 width, u32 height, gx::TextureFormat format);

//! @brief Compute the encoded size of an image.
//!
//! @param[in] width Width of the image. Does not need to be padded.
//! @param[in] height Height of the image. Does not need to be padded.
//! @param[in] format Format of the image.
//! @param[in] mipMapCount Number of additional levels of detail past the first
//! image. Zero corresponds to the base image--no mipmapping.
//!
//! @return The computed size of the image in bytes.
//!
[[nodiscard]] int getEncodedSize(int width, int height,
                                 gx::TextureFormat format, u32 mipMapCount = 0);

//! @brief Decode an image to 8-bit, four-channel RGBA.
//!
//! @param[in] dst The destination pointer to the decoded data. The decoded
//! image will be written here directly.
//! @param[in] src The source pointer to the encoded data.
//! @param[in] width The width of the image in pixels.
//! @param[in] height The height of the image in pixels.
//! @param[in] texformat The format of the image.
//! @param[in] tlut Pointer to palette (Texture Lookup) data.
//! @param[in] tlutformat Format of the palette (Texture Lookup) data.
//!
//! @pre For efficiency reasons, this method does not handle the case where dst
//! == src.
//!
void decode(std::span<u8> dst, std::span<const u8> src, int width, int height,
            gx::TextureFormat texformat, std::span<const u8> tlut = {},
            gx::PaletteFormat tlutformat = gx::PaletteFormat::IA8);

//! @brief Encode an image to a GPU texture.
//!
//! @param[in] dst The destination pointer to the decoded data. The encoded
//! image will be written here directly.
//! @param[in] src The source pointer to the raw data.
//! @param[in] width The width of the image in pixels.
//! @param[in] height The height of the image in pixels.
//! @param[in] texformat The format of the image.
//!
//! @pre For efficiency reasons, this method does not handle the case where dst
//! == src.
//!
[[nodiscard]] Result<void> encode(std::span<u8> dst, std::span<const u8> src,
                                  int width, int height,
                                  gx::TextureFormat texformat);

//! @brief Specifies an algorithm for downscaling/upscaling an image.
//!
enum class ResizingAlgorithm { AVIR, Lanczos };

// dst and source may be equal
// raw 8-bit RGBA resize
//! @brief Resize a raw, 8-bit RGBA buffer.
//!
//! @param[in] dst  The desination pointer. (May equal the source pointer)
//! @param[in] dx   Width of the target image in pixels.
//! @param[in] dy   Height of the target image in pixels.
//! @param[in] src  Pointer to the source image.
//! @param[in] sx   Width of the source image in pixels.
//! @param[in] sy   Height of the source image in pixels.
//! @param[in] type Algorithm to utilize for upscaling/downscaling.
//!
void resize(std::span<u8> dst, int dx, int dy, std::span<const u8> src, int sx,
            int sy, ResizingAlgorithm type = ResizingAlgorithm::Lanczos);

//! @brief Perform a composite transformation on image data, with mipmap
//! support.
//!
//! @param[in] dst			The desination pointer. (May equal the
//! source pointer)
//! @param[in] dx			Width of the target image in pixels.
//! @param[in] dy			Height of the target image in pixels.
//! @param[in] oldformat	Format of the source data.
//! gx::TextureFormat::Extension_RawRGBA32 may be passed in to indicate raw
//! data.
//! @param[in] newformat	Format of the target data.
//! gx::TextureFormat::Extension_RawRGBA32 may be passed in to indicate raw
//! data.
//! @param[in] src			Pointer to the source data. If nullptr,
//! the value of dst is used.
//! @param[in] sx			Width of the source image in pixels. If
//! below 1, the value of dx is used.
//! @param[in] sy			Height of the source image in pixels. If
//! below 1, the value of dy is used.
//! @param[in] mipMapCount	Number of additional levels of detail past the
//! first image. Zero corresponds to the base image--no mipmapping.
//! @param[in] algorithm	Algorithm to utilize for upscaling/downscaling.
//!
[[nodiscard]] Result<void>
transform(std::span<u8> dst, int dwidth, int dheight,
          gx::TextureFormat oldformat = gx::TextureFormat::Extension_RawRGBA32,
          std::optional<gx::TextureFormat> newformat = std::nullopt,
          std::span<const u8> src = {}, int sx = -1, int sy = -1,
          u32 mipMapCount = 0,
          ResizingAlgorithm algorithm = ResizingAlgorithm::Lanczos);

std::string_view gctex_version();

} // namespace librii::image
