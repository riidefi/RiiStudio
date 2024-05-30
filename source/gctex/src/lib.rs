use std::slice;

#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
pub mod bindings {
    #[cfg(feature = "run_bindgen")]
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

    // For now, just manually embed these...
    #[cfg(not(feature = "run_bindgen"))]
    extern "C" {
        pub fn impl_rii_decode(
            dst: *mut u8,
            src: *const u8,
            width: u32,
            height: u32,
            texformat: u32,
            tlut: *const u8,
            tlutformat: u32,
        );
        pub fn impl_rii_encodeCMPR(
            dest_img: *mut u8,
            source_img: *const u8,
            width: u32,
            height: u32,
        );
        pub fn impl_rii_encodeI4(dst: *mut u8, src: *const u8, width: u32, height: u32);
        pub fn impl_rii_encodeI8(dst: *mut u8, src: *const u8, width: u32, height: u32);
        pub fn impl_rii_encodeIA4(dst: *mut u8, src: *const u8, width: u32, height: u32);
        pub fn impl_rii_encodeIA8(dst: *mut u8, src: *const u8, width: u32, height: u32);
        pub fn impl_rii_encodeRGB565(dst: *mut u8, src: *const u8, width: u32, height: u32);
        pub fn impl_rii_encodeRGB5A3(dst: *mut u8, src: *const u8, width: u32, height: u32);
        pub fn impl_rii_encodeRGBA8(dst: *mut u8, src4: *const u8, width: u32, height: u32);
    }
}

pub const _CTF: u32 = 0x20;
pub const _ZTF: u32 = 0x10;

#[repr(u32)]
#[derive(Debug, Copy, Clone, PartialEq)]
pub enum TextureFormat {
    I4 = 0,
    I8,
    IA4,
    IA8,
    RGB565,
    RGB5A3,
    RGBA8,

    C4 = 0x8,
    C8,
    C14X2,
    CMPR = 0xE,

    ExtensionRawRGBA32 = 0xFF,
}

impl TextureFormat {
    pub fn from_u32(val: u32) -> Option<Self> {
        match val {
            x if x == TextureFormat::I4 as u32 => Some(TextureFormat::I4),
            x if x == TextureFormat::I8 as u32 => Some(TextureFormat::I8),
            x if x == TextureFormat::IA4 as u32 => Some(TextureFormat::IA4),
            x if x == TextureFormat::IA8 as u32 => Some(TextureFormat::IA8),
            x if x == TextureFormat::RGB565 as u32 => Some(TextureFormat::RGB565),
            x if x == TextureFormat::RGB5A3 as u32 => Some(TextureFormat::RGB5A3),
            x if x == TextureFormat::RGBA8 as u32 => Some(TextureFormat::RGBA8),
            x if x == TextureFormat::C4 as u32 => Some(TextureFormat::C4),
            x if x == TextureFormat::C8 as u32 => Some(TextureFormat::C8),
            x if x == TextureFormat::C14X2 as u32 => Some(TextureFormat::C14X2),
            x if x == TextureFormat::CMPR as u32 => Some(TextureFormat::CMPR),
            x if x == TextureFormat::ExtensionRawRGBA32 as u32 => {
                Some(TextureFormat::ExtensionRawRGBA32)
            }
            _ => None,
        }
    }
}

#[repr(u32)]
#[derive(Debug, Copy, Clone)]
pub enum CopyTextureFormat {
    R4 = 0x0 | _CTF,
    RA4 = 0x2 | _CTF,
    RA8 = 0x3 | _CTF,
    YUVA8 = 0x6 | _CTF,
    A8 = 0x7 | _CTF,
    R8 = 0x8 | _CTF,
    G8 = 0x9 | _CTF,
    B8 = 0xA | _CTF,
    RG8 = 0xB | _CTF,
    GB8 = 0xC | _CTF,
}

#[repr(u32)]
#[derive(Debug, Copy, Clone)]
pub enum ZTextureFormat {
    Z8 = 0x1 | _ZTF,
    Z16 = 0x3 | _ZTF,
    Z24X8 = 0x6 | _ZTF,

    Z4 = 0x0 | _ZTF | _CTF,
    Z8M = 0x9 | _ZTF | _CTF,
    Z8L = 0xA | _ZTF | _CTF,
    Z16L = 0xC | _ZTF | _CTF,
}

fn round_up(val: u32, round_to: u32) -> u32 {
    let remainder = val % round_to;
    if remainder == 0 {
        val
    } else {
        val - remainder + round_to
    }
}

#[derive(Debug, Copy, Clone)]
struct ImageFormatInfo {
    xshift: u32,
    yshift: u32,
    bitsize: u32,

    block_width_in_texels: u32,
    block_height_in_texels: u32,
}

fn get_format_info(format: TextureFormat) -> ImageFormatInfo {
    let (xshift, yshift) = match format as u32 {
        x if x == TextureFormat::C4 as u32
            || x == TextureFormat::I4 as u32
            || x == TextureFormat::CMPR as u32
            || x == CopyTextureFormat::R4 as u32
            || x == CopyTextureFormat::RA4 as u32
            || x == ZTextureFormat::Z4 as u32 =>
        {
            (3, 3)
        }
        x if x == ZTextureFormat::Z8 as u32
            || x == TextureFormat::C8 as u32
            || x == TextureFormat::I8 as u32
            || x == TextureFormat::IA4 as u32
            || x == CopyTextureFormat::A8 as u32
            || x == CopyTextureFormat::R8 as u32
            || x == CopyTextureFormat::G8 as u32
            || x == CopyTextureFormat::B8 as u32
            || x == CopyTextureFormat::RG8 as u32
            || x == CopyTextureFormat::GB8 as u32
            || x == ZTextureFormat::Z8M as u32
            || x == ZTextureFormat::Z8L as u32 =>
        {
            (3, 2)
        }
        x if x == TextureFormat::C14X2 as u32
            || x == TextureFormat::IA8 as u32
            || x == ZTextureFormat::Z16 as u32
            || x == ZTextureFormat::Z24X8 as u32
            || x == TextureFormat::RGB565 as u32
            || x == TextureFormat::RGB5A3 as u32
            || x == TextureFormat::RGBA8 as u32
            || x == ZTextureFormat::Z16L as u32
            || x == CopyTextureFormat::RA8 as u32 =>
        {
            (2, 2)
        }
        _ => panic!("Invalid texture format"),
    };

    let mut bitsize = 32;
    // TODO: This function should allow ZTextureFormat too..
    if format == TextureFormat::RGBA8 || format as u32 == ZTextureFormat::Z24X8 as u32 {
        bitsize = 64;
    }

    ImageFormatInfo {
        xshift,
        yshift,
        bitsize,
        block_width_in_texels: (1 << xshift),
        block_height_in_texels: (1 << yshift),
    }
}

fn info_compute_image_size(info: ImageFormatInfo, width: u32, height: u32) -> u32 {
    // div_ceil(width, 1 << info.xshift)
    let xtiles = ((width + (1 << info.xshift)) - 1) >> info.xshift;
    // div_ceil(height, 1 << info.yshift)
    let ytiles = ((height + (1 << info.yshift)) - 1) >> info.yshift;

    xtiles * ytiles * info.bitsize
}

/// Computes the size of the image based on its texture format, width, and height.
///
/// # Parameters
/// - `format`: The texture format of the image.
/// - `width`: The width of the image.
/// - `height`: The height of the image.
///
/// # Returns
/// The computed size of the image in bytes.
///
/// # Example
/// ```
/// let format = gctex::TextureFormat::C8;
/// let width = 128;
/// let height = 128;
/// let size = gctex::compute_image_size(format, width, height);
/// assert_eq!(size, 16384);
/// ```
pub fn compute_image_size(format: TextureFormat, width: u32, height: u32) -> u32 {
    if format == TextureFormat::ExtensionRawRGBA32 {
        return width * height * 4;
    }
    let info = get_format_info(format);
    info_compute_image_size(info, width, height)
}

/// Computes the total size of the image and its mipmaps based on the texture format, width, height, and number of images.
///
/// # Parameters
/// - `format`: The texture format of the image.
/// - `width`: The initial width of the image.
/// - `height`: The initial height of the image.
/// - `number_of_images`: The number of images including the original and its mipmaps.
///
/// # Returns
/// The computed total size of the image and its mipmaps in bytes.
///
/// # Example
/// ```
/// let format = gctex::TextureFormat::I8;
/// let width = 1000;
/// let height = 1000;
/// let number_of_images = 5;
/// let size = gctex::compute_image_size_mip(format, width, height, number_of_images);
/// assert_eq!(size, 1336992);
/// ```
pub fn compute_image_size_mip(
    format: TextureFormat,
    mut width: u32,
    mut height: u32,
    number_of_images: u32,
) -> u32 {
    assert!(number_of_images != 0);
    if number_of_images <= 1 {
        return compute_image_size(format, width, height);
    }

    let mut size = 0;

    for _i in 0..number_of_images {
        size += compute_image_size(format, width, height);

        if width == 1 && height == 1 {
            break;
        }

        if width > 1 {
            width >>= 1;
        } else {
            width = 1;
        }

        if height > 1 {
            height >>= 1;
        } else {
            height = 1;
        }
    }

    size
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn image_size_c8() {
        let format = TextureFormat::C8;
        let width = 128;
        let height = 128;
        let number_of_images = 1;

        let result = compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 16384);
    }

    #[test]
    fn test_rii_compute_image_size_mip_with_1x1() {
        let format = TextureFormat::CMPR;
        let width = 16;
        let height = 16;
        let number_of_images = 5;

        let result = compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 256);
    }

    #[test]
    fn test_rii_compute_image_size_mip_with_small_size() {
        let format = TextureFormat::I8;
        let width = 10;
        let height = 10;
        let number_of_images = 1;

        let result = compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 192);
    }

    #[test]
    fn test_rii_compute_image_size_mip_with_large_size() {
        let format = TextureFormat::I8;
        let width = 1000;
        let height = 1000;
        let number_of_images = 1;

        let result = compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 1000000);
    }

    #[test]
    fn test_rii_compute_image_size_mip_with_multiple_images() {
        let format = TextureFormat::I8;
        let width = 1000;
        let height = 1000;
        let number_of_images = 5;

        let result = compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 1336992);
    }
}

const fn convert_3_to_8(v: u8) -> u8 {
    // Swizzle bits: 00000123 -> 12312312
    (v << 5) | (v << 2) | (v >> 1)
}

const fn convert_4_to_8(v: u8) -> u8 {
    // Swizzle bits: 00001234 -> 12341234
    (v << 4) | v
}

const fn convert_5_to_8(v: u8) -> u8 {
    // Swizzle bits: 00012345 -> 12345123
    (v << 3) | (v >> 2)
}

const fn convert_6_to_8(v: u8) -> u8 {
    // Swizzle bits: 00123456 -> 12345612
    (v << 2) | (v >> 4)
}

fn decode_texture_i4(dst: &mut [u8], src: &[u8], width: usize, height: usize) {
    let mut src_idx = 0;
    for y in (0..height).step_by(8) {
        for x in (0..width).step_by(8) {
            for iy in 0..8 {
                for ix in 0..4 {
                    let val = src[src_idx + ix];
                    let i1 = convert_4_to_8(val >> 4);
                    let i2 = convert_4_to_8(val & 0xF);
                    let dst_idx1 = (y + iy) * width + x + ix * 2;
                    let dst_idx2 = dst_idx1 + 1;
                    dst[dst_idx1*4..dst_idx1*4+4].fill(i1);
                    dst[dst_idx2*4..dst_idx2*4+4].fill(i2);
                }
                src_idx += 4;
            }
        }
    }
}

// Requires expanded size to be block-aligned
/// Decodes a texture using a fast decoding method optimized for the Nintendo GameCube and Wii texture formats.
///
/// This method is optimized for scenarios where the given texture's dimensions
/// are block-aligned according to the GameCube and Wii texture standards.
/// Ensure the dimensions are block-aligned before using this method for accurate decoding.
///
/// # Arguments
/// * `dst` - The destination buffer to write the decoded data into.
/// * `src` - The source buffer containing the encoded texture data.
/// * `width` - The width of the texture.
/// * `height` - The height of the texture.
/// * `texformat` - The format of the texture, specific to GameCube and Wii, to be decoded.
/// * `tlut` - A table look-up texture used for certain texture formats.
/// * `tlutformat` - The format of the `tlut`.
///
/// # Panics
/// This function will panic if:
/// * The destination buffer is too small to hold the decoded data.
/// * The source buffer does not have enough data for the given `width` and `height`.
///
pub fn decode_fast(
    dst: &mut [u8],
    src: &[u8],
    width: u32,
    height: u32,
    texformat: TextureFormat,
    tlut: &[u8],
    tlutformat: u32,
) {
    let info = get_format_info(texformat);
    let expanded_width = round_up(width, info.block_width_in_texels);
    let expanded_height = round_up(height, info.block_height_in_texels);
    assert!(dst.len() as u32 >= expanded_width * expanded_height * 4);
    assert!(src.len() as u32 >= compute_image_size(texformat, width, height));

    #[cfg(feature = "no_simd")]
    {
      if texformat == TextureFormat::I4 {
          decode_texture_i4(dst, src, width as usize, height as usize);
          return;
      }
    }

    unsafe {
        bindings::impl_rii_decode(
            dst.as_mut_ptr(),
            src.as_ptr(),
            width,
            height,
            texformat as u32,
            tlut.as_ptr(),
            tlutformat,
        );
    }
}

/// Decodes a Nintendo GameCube or Wii texture into the provided destination buffer.
///
/// Based on the alignment of the texture's dimensions, this method determines if
/// the fast decoding method (`decode_fast`) can be utilized. If the texture dimensions
/// are not block-aligned, padding will be handled, and then the fast decoding method is employed.
///
/// # Arguments
/// * `dst` - The destination buffer to write the decoded data into.
/// * `src` - The source buffer containing the encoded texture data.
/// * `width` - The width of the texture.
/// * `height` - The height of the texture.
/// * `texformat` - The format of the texture, specific to GameCube and Wii, to be decoded.
/// * `tlut` - A table look-up texture used for certain texture formats.
/// * `tlutformat` - The format of the `tlut`.
///
/// # Panics
/// This function will panic if:
/// * The destination buffer is too small to hold the decoded data.
/// * The source buffer does not have enough data for the given `width` and `height`.
///
pub fn decode_into(
    dst: &mut [u8],
    src: &[u8],
    width: u32,
    height: u32,
    texformat: TextureFormat,
    tlut: &[u8],
    tlutformat: u32,
) {
    assert!(dst.len() as u32 >= width * height * 4);

    let info = get_format_info(texformat);
    let expanded_width = round_up(width, info.block_width_in_texels);
    let expanded_height = round_up(height, info.block_height_in_texels);

    assert!(src.len() as u32 >= compute_image_size(texformat, width, height));

    if expanded_width == width && expanded_height == height {
        decode_fast(dst, src, width, height, texformat, tlut, tlutformat);
    } else {
        let mut tmp = vec![0 as u8; (expanded_width * expanded_height * 4) as usize];

        decode_fast(
            &mut tmp[..],
            src,
            width,
            height,
            texformat,
            tlut,
            tlutformat,
        );
        let nonpadding_dst_size = (width * height * 4) as usize;
        dst[..nonpadding_dst_size].copy_from_slice(&tmp[..nonpadding_dst_size]);
    }
}

#[cfg(test)]
mod tests2 {
    use super::*;
    use std::fs::File;
    use std::io::Read;

    #[test]
    fn test_decode_cmpr_fast() {
        let mut file = File::open("tests/monke.cmpr").unwrap();
        let mut src = Vec::new();
        file.read_to_end(&mut src).unwrap();

        let mut dst = vec![0; 504 * 504 * 4];

        let tlut = &[];
        decode_fast(&mut dst, &src, 500, 500, TextureFormat::CMPR, tlut, 0);

        let mut expected_file = File::open("tests/monke_expected_result").unwrap();
        let mut expected_dst = Vec::new();
        expected_file.read_to_end(&mut expected_dst).unwrap();

        assert_eq!(dst.len(), expected_dst.len());
        assert_eq!(dst, expected_dst);
    }
    #[test]
    fn test_decode_cmpr_nonfast() {
        let mut file = File::open("tests/monke.cmpr").unwrap();
        let mut src = Vec::new();
        file.read_to_end(&mut src).unwrap();

        let mut dst = vec![0; 500 * 500 * 4];

        let tlut = &[];
        decode_into(&mut dst, &src, 500, 500, TextureFormat::CMPR, tlut, 0);

        let mut expected_file = File::open("tests/monke_expected_result").unwrap();
        let mut expected_dst = Vec::new();
        expected_file.read_to_end(&mut expected_dst).unwrap();
        expected_dst.resize(500 * 500 * 4, 0);

        assert_eq!(dst, expected_dst);
    }
}

/// Decodes a Nintendo GameCube or Wii texture and returns the decoded data as a `Vec<u8>`.
///
/// This function uses the `decode_into` method internally to decode the texture.
/// It automatically determines the necessary buffer size based on the provided `width`
/// and `height` and then returns the decoded data.
///
/// # Arguments
/// * `src` - The source buffer containing the encoded texture data.
/// * `width` - The width of the texture.
/// * `height` - The height of the texture.
/// * `texformat` - The format of the texture, specific to GameCube and Wii, to be decoded.
/// * `tlut` - A table look-up texture used for certain texture formats.
/// * `tlutformat` - The format of the `tlut`.
///
/// # Returns
/// A `Vec<u8>` containing the decoded texture data.
///
/// # Panics
/// This function will panic if:
/// * The source buffer does not have enough data for the given `width` and `height`.
///
pub fn decode(
    src: &[u8],
    width: u32,
    height: u32,
    texformat: TextureFormat,
    tlut: &[u8],
    tlutformat: u32,
) -> Vec<u8> {
    let size = (width * height * 4) as usize;
    let mut dst = vec![0u8; size];

    decode_into(&mut dst, src, width, height, texformat, tlut, tlutformat);

    dst
}

pub fn encode_cmpr_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeCMPR(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

pub fn legacy_encode_i4_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeI4(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

struct Rgba {
    r: u8,
    g: u8,
    b: u8,
    a: u8,
}

impl Rgba {
    fn from_slice(slice: &[u8]) -> Rgba {
        Rgba {
            r: slice[0],
            g: slice[1],
            b: slice[2],
            a: slice[3],
        }
    }
}

fn luminosity(rgba: &Rgba) -> u8 {
    let f = rgba.r as f64 * 0.299 + rgba.g as f64 * 0.587 + rgba.b as f64 * 0.114;
    f as u8
}

fn encode_i4_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    let mut dst_index: u32 = 0;
    for y in (0..height).step_by(8) {
        for x in (0..width).step_by(8) {
            for row in 0..8 {
                // Process two columns at a time
                for column in 0..4 {
                    let pos = (y + row) * width + x + column * 2;
                    let rgba0 = Rgba::from_slice(&src[(pos * 4) as usize..]);
                    let rgba1 = Rgba::from_slice(&src[(pos * 4 + 4) as usize..]);

                    let i0 = luminosity(&rgba0);
                    let i1 = luminosity(&rgba1);

                    let i0i1 = (i0 & 0b1111_0000) | ((i1 >> 4) & 0b0000_1111);
                    dst[(dst_index + column) as usize] = i0i1;
                }
                dst_index += 4;
            }
        }
    }
}

pub fn legacy_encode_i8_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeI8(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

fn encode_i8_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    let mut dst_index: usize = 0;
    for y in (0..height).step_by(4) {
        for x in (0..width).step_by(8) {
            for row in 0..4 {
                for column in 0..8 {
                    let pos = (((y + row) * width + x + column) * 4) as usize;
                    let rgba = Rgba::from_slice(&src[pos..pos + 4]);
                    dst[dst_index] = luminosity(&rgba);
                    dst_index += 1;
                }
            }
        }
    }
}

pub fn legacy_encode_ia4_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeIA4(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

fn encode_ia4_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    let mut dst_index: usize = 0;
    for y in (0..height).step_by(4) {
        for x in (0..width).step_by(8) {
            for row in 0..4 {
                for column in 0..8 {
                    let pos = (((y + row) * width + x + column) * 4) as usize;
                    let rgba = Rgba::from_slice(&src[pos..pos + 4]);
                    dst[dst_index] = (luminosity(&rgba) & 0b1111_0000) | (rgba.a >> 4);
                    dst_index += 1;
                }
            }
        }
    }
}

pub fn legacy_encode_ia8_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeIA8(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

fn encode_ia8_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    let mut dst_index: usize = 0;
    for y in (0..height).step_by(4) {
        for x in (0..width).step_by(4) {
            for row in 0..4 {
                for column in 0..4 {
                    let pos = (((y + row) * width + x + column) * 4) as usize;
                    let rgba = Rgba::from_slice(&src[pos..pos + 4]);
                    dst[dst_index] = luminosity(&rgba);
                    dst[dst_index + 1] = rgba.a;
                    dst_index += 2;
                }
            }
        }
    }
}

pub fn legacy_encode_rgb565_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeRGB565(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

fn encode_rgb565_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    let mut dst_index: usize = 0;
    for y in (0..height).step_by(4) {
        for x in (0..width).step_by(4) {
            for row in 0..4 {
                for column in 0..4 {
                    let pos = (((y + row) * width + x + column) * 4) as usize;
                    let rgba = Rgba::from_slice(&src[pos..pos + 4]);
                    let packed = ((rgba.r & 0xF8) as u16) << 8
                        | ((rgba.g & 0xFC) as u16) << 3
                        | ((rgba.b & 0xF8) as u16) >> 3;
                    dst[dst_index] = (packed >> 8) as u8;
                    dst[dst_index + 1] = packed as u8;
                    dst_index += 2;
                }
            }
        }
    }
}

pub fn legacy_encode_rgb5a3_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeRGB5A3(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

fn encode_rgb5a3_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    let mut dst_index: usize = 0;
    for y in (0..height).step_by(4) {
        for x in (0..width).step_by(4) {
            for row in 0..4 {
                for column in 0..4 {
                    let pos = (((y + row) * width + x + column) * 4) as usize;
                    let rgba = Rgba::from_slice(&src[pos..pos + 4]);
                    let packed = if rgba.a < 0xE0 {
                        ((rgba.a & 0xE0) as u16) << 7
                            | ((rgba.r & 0xF0) as u16) << 4
                            | ((rgba.g & 0xF0) as u16)
                            | ((rgba.b & 0xF0) as u16) >> 4
                    } else {
                        0x8000
                            | ((rgba.r & 0xF8) as u16) << 7
                            | ((rgba.g & 0xF8) as u16) << 2
                            | ((rgba.b & 0xF8) as u16) >> 3
                    };
                    dst[dst_index] = (packed >> 8) as u8;
                    dst[dst_index + 1] = packed as u8;
                    dst_index += 2;
                }
            }
        }
    }
}

pub fn legacy_encode_rgba8_into(dst: &mut [u8], src4: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeRGBA8(dst.as_mut_ptr(), src4.as_ptr(), width, height);
    }
}

fn encode_rgba8_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    let mut dst_index: usize = 0;

    for block in (0..height).step_by(4) {
        for i in (0..width).step_by(4) {
            for c in 0..4 {
                let block_wid = ((block + c) * width + i) * 4;

                let rgba0 = Rgba::from_slice(&src[block_wid as usize..block_wid as usize + 4]);
                let rgba1 =
                    Rgba::from_slice(&src[(block_wid + 4) as usize..(block_wid + 8) as usize]);
                let rgba2 =
                    Rgba::from_slice(&src[(block_wid + 8) as usize..(block_wid + 12) as usize]);
                let rgba3 =
                    Rgba::from_slice(&src[(block_wid + 12) as usize..(block_wid + 16) as usize]);

                dst[dst_index] = rgba0.a;
                dst[dst_index + 1] = rgba0.r;
                dst[dst_index + 2] = rgba1.a;
                dst[dst_index + 3] = rgba1.r;
                dst[dst_index + 4] = rgba2.a;
                dst[dst_index + 5] = rgba2.r;
                dst[dst_index + 6] = rgba3.a;
                dst[dst_index + 7] = rgba3.r;

                dst_index += 8;
            }

            for c in 0..4 {
                let block_wid = ((block + c) * width + i) * 4;

                let rgba0 = Rgba::from_slice(&src[block_wid as usize..block_wid as usize + 4]);
                let rgba1 =
                    Rgba::from_slice(&src[(block_wid + 4) as usize..(block_wid + 8) as usize]);
                let rgba2 =
                    Rgba::from_slice(&src[(block_wid + 8) as usize..(block_wid + 12) as usize]);
                let rgba3 =
                    Rgba::from_slice(&src[(block_wid + 12) as usize..(block_wid + 16) as usize]);

                dst[dst_index] = rgba0.g;
                dst[dst_index + 1] = rgba0.b;
                dst[dst_index + 2] = rgba1.g;
                dst[dst_index + 3] = rgba1.b;
                dst[dst_index + 4] = rgba2.g;
                dst[dst_index + 5] = rgba2.b;
                dst[dst_index + 6] = rgba3.g;
                dst[dst_index + 7] = rgba3.b;

                dst_index += 8;
            }
        }
    }
}

pub fn encode_raw_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    let bytes_to_copy = (width * height * 4) as usize;

    if src.len() < bytes_to_copy || dst.len() < bytes_to_copy {
        panic!("encode_raw_into: Buffer sizes are not sufficient!");
    }

    dst[..bytes_to_copy].copy_from_slice(&src[..bytes_to_copy]);
}

/// Encodes a Nintendo GameCube or Wii texture into the specified format and writes the result into the provided destination buffer.
///
/// This function is optimized for scenarios where the given texture's dimensions are block-aligned according to the GameCube and Wii texture standards.
/// Ensure the dimensions are block-aligned before using this method for accurate encoding.
///
/// # Arguments
/// * `format` - The format of the texture specific to GameCube and Wii to which the source data will be encoded.
/// * `dst` - The destination buffer to write the encoded data into.
/// * `src` - The source buffer containing the RGBA texture data to be encoded.
/// * `width` - The width of the texture.
/// * `height` - The height of the texture.
///
/// # Panics
/// This function will panic if:
/// * The destination buffer is too small to hold the encoded data for the specified format and texture dimensions.
/// * The source buffer does not have the expected size for the given `width` and `height`.
/// * An unsupported `TextureFormat` is provided.
///
pub fn encode_fast(format: TextureFormat, dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    assert!(dst.len() >= compute_image_size(format, width, height) as usize);
    assert!(src.len() >= (width * height * 4) as usize);
    match format {
        TextureFormat::I4 => encode_i4_into(dst, src, width, height),
        TextureFormat::I8 => encode_i8_into(dst, src, width, height),
        TextureFormat::IA4 => encode_ia4_into(dst, src, width, height),
        TextureFormat::IA8 => encode_ia8_into(dst, src, width, height),
        TextureFormat::RGB565 => encode_rgb565_into(dst, src, width, height),
        TextureFormat::RGB5A3 => encode_rgb5a3_into(dst, src, width, height),
        TextureFormat::RGBA8 => encode_rgba8_into(dst, src, width, height),
        TextureFormat::CMPR => encode_cmpr_into(dst, src, width, height),
        TextureFormat::ExtensionRawRGBA32 => encode_raw_into(dst, src, width, height),
        _ => panic!("encode_fast: Unsupported texture format: {:?}", format),
    }
}

/// Encodes a Nintendo GameCube or Wii texture into the specified format and writes the result into the provided destination buffer.
///
/// Based on the alignment of the texture's dimensions, this method determines if the fast encoding method (`encode_fast`) can be utilized.
/// If the texture dimensions are not block-aligned, padding will be handled, and then the fast encoding method is employed.
///
/// # Arguments
/// * `format` - The format of the texture specific to GameCube and Wii to which the source data will be encoded.
/// * `dst` - The destination buffer to write the encoded data into.
/// * `src` - The source buffer containing the RGBA texture data to be encoded.
/// * `width` - The width of the texture.
/// * `height` - The height of the texture.
///
/// # Panics
/// This function will panic if:
/// * The destination buffer is too small to hold the encoded data for the specified format and texture dimensions.
/// * The source buffer does not have the expected size for the given `width` and `height`.
/// * An unsupported `TextureFormat` is provided.
///
pub fn encode_into(format: TextureFormat, dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    let info = get_format_info(format);
    let expanded_width = round_up(width, info.block_width_in_texels);
    let expanded_height = round_up(height, info.block_height_in_texels);

    assert!(dst.len() >= compute_image_size(format, width, height) as usize);
    assert!(src.len() >= (width * height * 4) as usize);

    if expanded_width == width && expanded_height == height {
        encode_fast(format, dst, src, width, height);
    } else {
        let mut tmp_src = vec![0u8; (expanded_width * expanded_height * 4) as usize];

        if width > 0 && height > 0 {
            for y in 0..expanded_height {
                for x in 0..expanded_width {
                    // Row-major matrix
                    let min = |a, b| if a < b { a } else { b };
                    let src_cell = width * min(y, height - 1) + min(x, width - 1);
                    let dst_cell = expanded_width * y + x;
                    tmp_src[(dst_cell * 4) as usize..(dst_cell * 4 + 4) as usize].copy_from_slice(
                        &src[(src_cell * 4) as usize..(src_cell * 4 + 4) as usize],
                    );
                }
            }
        }

        encode_fast(format, dst, &tmp_src, expanded_width, expanded_height);
    }
}

/// Encodes a Nintendo GameCube or Wii texture into the specified format and returns the encoded data as a `Vec<u8>`.
///
/// This function uses the `encode_into` method internally to encode the texture.
/// It automatically determines the necessary buffer size based on the provided `width`,
/// `height`, and `format`, then returns the encoded data.
///
/// # Arguments
/// * `format` - The format of the texture specific to GameCube and Wii to which the source data will be encoded.
/// * `src` - The source buffer containing the RGBA texture data to be encoded.
/// * `width` - The width of the texture.
/// * `height` - The height of the texture.
///
/// # Returns
/// A `Vec<u8>` containing the encoded texture data.
///
/// # Panics
/// This function will panic if:
/// * The source buffer does not have the expected size for the given `width` and `height`.
/// * An unsupported `TextureFormat` is provided.
///
pub fn encode(format: TextureFormat, src: &[u8], width: u32, height: u32) -> Vec<u8> {
    let size = compute_image_size(format, width, height) as usize;
    let mut dst = vec![0u8; size];

    encode_into(format, &mut dst, src, width, height);

    dst
}

#[cfg(test)]
mod tests3 {
    use super::*;
    use rand::rngs::StdRng;
    use rand::{Rng, SeedableRng};
    use std::fs;
    use std::fs::File;
    use std::io::Read;
    use std::path::Path;

    const IMAGE_WIDTH: u32 = 500;
    const IMAGE_HEIGHT: u32 = 500;
    const RAW_IMAGE_PATH: &str = "tests/monke_expected_result";
    const CACHE_DIR: &str = "tests";

    fn load_raw_image(path: &str) -> Vec<u8> {
        let mut file = File::open(path).expect("Failed to open raw image file");
        let mut buffer = Vec::new();
        file.read_to_end(&mut buffer)
            .expect("Failed to read raw image file");
        buffer
    }

    fn save_encoded_blob(format: TextureFormat, data: &[u8], cache_name: &str) {
        let cache_path = format!("{}/{}_{}", CACHE_DIR, cache_name, format as u32);
        fs::write(cache_path, data).expect("Failed to write encoded blob to cache");
    }

    fn load_encoded_blob(format: TextureFormat, cache_name: &str) -> Option<Vec<u8>> {
        let cache_path = format!("{}/{}_{}", CACHE_DIR, cache_name, format as u32);
        if Path::new(&cache_path).exists() {
            Some(fs::read(cache_path).expect("Failed to read encoded blob from cache"))
        } else {
            None
        }
    }

    fn generate_random_image_data(width: u32, height: u32, seed: u64) -> Vec<u8> {
        let mut rng = StdRng::seed_from_u64(seed);
        (0..width * height * 4).map(|_| rng.gen()).collect()
    }

    fn assert_buffers_equal(encoded_image: &[u8], cached_image: &[u8], format: &str) {
        if encoded_image != cached_image {
            let diff_count = encoded_image
                .iter()
                .zip(cached_image.iter())
                .filter(|(a, b)| a != b)
                .count();
            println!(
                "Mismatch for format {:?} - {} bytes differ",
                format, diff_count
            );

            // Print some example differences (up to 10)
            let mut diff_examples = vec![];
            for (i, (a, b)) in encoded_image.iter().zip(cached_image.iter()).enumerate() {
                if a != b {
                    diff_examples.push((i, *a, *b));
                    if diff_examples.len() >= 10 {
                        break;
                    }
                }
            }

            println!("Example differences (up to 10):");
            for (i, a, b) in diff_examples {
                println!("Byte {}: encoded = {}, cached = {}", i, a, b);
            }

            panic!("Buffers are not equal");
        }
    }

    #[test]
    fn test_encode_all_formats() {
        // Load the raw RGBA image
        let mut raw_image = load_raw_image(RAW_IMAGE_PATH);

        // hack
        raw_image.resize(500 * 500 * 4, 0);

        // Ensure the image has the expected size
        assert_eq!(raw_image.len(), (IMAGE_WIDTH * IMAGE_HEIGHT * 4) as usize);

        // Iterate over all texture formats
        for format in [
            TextureFormat::I4,
            TextureFormat::I8,
            TextureFormat::IA4,
            TextureFormat::IA8,
            TextureFormat::RGB565,
            TextureFormat::RGB5A3,
            TextureFormat::RGBA8,
            TextureFormat::CMPR,
        ] {
            // Encode the image
            let encoded_image = encode(format, &raw_image, IMAGE_WIDTH, IMAGE_HEIGHT);

            // Decode the image for debugging
            let decoded_image = decode(&encoded_image, IMAGE_WIDTH, IMAGE_HEIGHT, format, &[0], 0);

            // Save the decoded image as a .png file for debugging
            save_png(
                &decoded_image,
                IMAGE_WIDTH,
                IMAGE_HEIGHT,
                &format!("tests/mnk_dbg_{:?}.png", format),
            );

            // Load the cached result, if available
            if let Some(cached_image) = load_encoded_blob(format, "encoded_monke") {
                // Compare the encoded image with the cached result
                assert_buffers_equal(
                    &encoded_image,
                    &cached_image,
                    &format!("Mismatch for format {:?}", format),
                );
            } else {
                // Save the encoded image as the cached result
                save_encoded_blob(format, &encoded_image, "encoded_monke");
                // Trivially pass the test if there's no cached result
                println!(
                    "Cached result for format {:?} not found. Generated and saved new cache.",
                    format
                );
            }
        }
    }

    #[test]
    fn test_encode_with_random_data() {
        // Generate random image data with a fixed seed
        let raw_image = generate_random_image_data(1024, 1024, 1337);

        // Iterate over all texture formats
        for format in [
            TextureFormat::I4,
            TextureFormat::I8,
            TextureFormat::IA4,
            TextureFormat::IA8,
            TextureFormat::RGB565,
            TextureFormat::RGB5A3,
            TextureFormat::RGBA8,
            TextureFormat::CMPR,
        ] {
            println!("Encoding {}", format as u32);
            // Encode the image
            let encoded_image = encode(format, &raw_image, IMAGE_WIDTH, IMAGE_HEIGHT);

            // Decode the image for debugging
            let decoded_image = decode(&encoded_image, IMAGE_WIDTH, IMAGE_HEIGHT, format, &[0], 0);

            // Save the decoded image as a .png file for debugging
            save_png(
                &decoded_image,
                IMAGE_WIDTH,
                IMAGE_HEIGHT,
                &format!("tests/rng_dbg_{:?}.png", format),
            );

            // Load the cached result, if available
            if let Some(cached_image) = load_encoded_blob(format, "encoded_random") {
                // Compare the encoded image with the cached result
                assert_buffers_equal(
                    &encoded_image,
                    &cached_image,
                    &format!("Mismatch for format {:?}", format),
                );
            } else {
                // Save the encoded image as the cached result
                save_encoded_blob(format, &encoded_image, "encoded_random");
                // Trivially pass the test if there's no cached result
                println!(
                    "Cached result for format {:?} not found. Generated and saved new cache.",
                    format
                );
            }
        }
    }

    // Function to save a raw image buffer as a PNG file
    fn save_png(image: &[u8], width: u32, height: u32, path: &str) {
        use image::{ImageBuffer, Rgba};
        let buffer = ImageBuffer::<Rgba<u8>, _>::from_raw(width, height, image).unwrap();
        buffer.save(path).unwrap();
    }

    // Decoding tests

    #[test]
    fn test_decode_all_formats() {
        // Load the raw RGBA image
        let mut raw_image = load_raw_image(RAW_IMAGE_PATH);

        // hack
        raw_image.resize(500 * 500 * 4, 0);

        // Ensure the image has the expected size
        assert_eq!(raw_image.len(), (IMAGE_WIDTH * IMAGE_HEIGHT * 4) as usize);

        // Iterate over all texture formats
        for format in [
            TextureFormat::I4,
            TextureFormat::I8,
            TextureFormat::IA4,
            TextureFormat::IA8,
            TextureFormat::RGB565,
            TextureFormat::RGB5A3,
            TextureFormat::RGBA8,
            TextureFormat::CMPR,
        ] {
            // Load the cached encoded image, if available
            let encoded_image =
                if let Some(cached_encoded) = load_encoded_blob(format, "encoded_monke") {
                    cached_encoded
                } else {
                    // Encode the image
                    let new_encoded = encode(format, &raw_image, IMAGE_WIDTH, IMAGE_HEIGHT);
                    // Save the encoded image as the cached result
                    save_encoded_blob(format, &new_encoded, "encoded_monke");
                    new_encoded
                };

            // Decode the image
            let decoded_image = decode(&encoded_image, IMAGE_WIDTH, IMAGE_HEIGHT, format, &[0], 0);

            // Save the decoded image for debugging
            save_png(
                &decoded_image,
                IMAGE_WIDTH,
                IMAGE_HEIGHT,
                &format!("tests/mnk_dbg_decode_{:?}.png", format),
            );

            // Load the cached result, if available
            if let Some(cached_image) = load_encoded_blob(format, "decoded_monke") {
                // Compare the decoded image with the cached result
                assert_buffers_equal(
                    &decoded_image,
                    &cached_image,
                    &format!("Mismatch for format {:?}", format),
                );
            } else {
                // Save the decoded image as the cached result
                save_encoded_blob(format, &decoded_image, "decoded_monke");
                // Trivially pass the test if there's no cached result
                println!(
                    "Cached result for format {:?} not found. Generated and saved new cache.",
                    format
                );
            }
        }
    }

    #[test]
    fn test_decode_with_random_data() {
        // Generate random image data with a fixed seed
        let raw_image = generate_random_image_data(1024, 1024, 1337);

        // Iterate over all texture formats
        for format in [
            TextureFormat::I4,
            TextureFormat::I8,
            TextureFormat::IA4,
            TextureFormat::IA8,
            TextureFormat::RGB565,
            TextureFormat::RGB5A3,
            TextureFormat::RGBA8,
            TextureFormat::CMPR,
        ] {
            println!("Decoding {}", format as u32);

            // Load the cached encoded image, if available
            let encoded_image =
                if let Some(cached_encoded) = load_encoded_blob(format, "encoded_random") {
                    cached_encoded
                } else {
                    // Encode the image
                    let new_encoded = encode(format, &raw_image, IMAGE_WIDTH, IMAGE_HEIGHT);
                    // Save the encoded image as the cached result
                    save_encoded_blob(format, &new_encoded, "encoded_random");
                    new_encoded
                };

            // Decode the image
            let decoded_image = decode(&encoded_image, IMAGE_WIDTH, IMAGE_HEIGHT, format, &[0], 0);

            // Save the decoded image for debugging
            save_png(
                &decoded_image,
                IMAGE_WIDTH,
                IMAGE_HEIGHT,
                &format!("tests/rng_dbg_decode_{:?}.png", format),
            );

            // Load the cached result, if available
            if let Some(cached_image) = load_encoded_blob(format, "decoded_random") {
                // Compare the decoded image with the cached result
                assert_buffers_equal(
                    &decoded_image,
                    &cached_image,
                    &format!("Mismatch for format {:?}", format),
                );
            } else {
                // Save the decoded image as the cached result
                save_encoded_blob(format, &decoded_image, "decoded_random");
                // Trivially pass the test if there's no cached result
                println!(
                    "Cached result for format {:?} not found. Generated and saved new cache.",
                    format
                );
            }
        }
    }
}

/// Returns a formatted string representing the version, build profile, and target of the `gctex` crate.
///
/// This function makes use of several compile-time environment variables provided by Cargo
/// to determine the version, build profile (debug or release), and target platform.
///
/// # Returns
/// A `String` that contains:
/// - The version of the `gctex` crate, as specified in its `Cargo.toml`.
/// - The build profile, which can either be "debug" or "release".
/// - The target platform for which the crate was compiled.
///
/// # Example
/// ```
/// let version_string = gctex::get_version();
/// println!("{}", version_string);
/// // Outputs: "riidefi/gctex: Version: 0.1.0, Profile: release, Target: x86_64-unknown-linux-gnu"
/// ```
///
pub fn get_version() -> String {
    let pkg_version = env!("CARGO_PKG_VERSION");
    let profile = if cfg!(debug_assertions) {
        "debug"
    } else {
        "release"
    };
    let target = env!("TARGET");
    format!(
        "riidefi/gctex: Version: {}, Profile: {}, Target: {}",
        pkg_version, profile, target
    )
}

//--------------------------------------------------------
//
// C BINDINGS BEGIN
//
//--------------------------------------------------------
#[cfg(feature = "c_api")]
pub mod c_api {
    use crate::*;

    #[no_mangle]
    pub unsafe extern "C" fn rii_compute_image_size(format: u32, width: u32, height: u32) -> u32 {
        compute_image_size(TextureFormat::from_u32(format).unwrap(), width, height)
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_compute_image_size_mip(
        format: u32,
        width: u32,
        height: u32,
        number_of_images: u32,
    ) -> u32 {
        compute_image_size_mip(
            TextureFormat::from_u32(format).unwrap(),
            width,
            height,
            number_of_images,
        )
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_encode(
        format: u32,
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len as usize);
        let src_slice = slice::from_raw_parts(src as *const u8, src_len as usize);
        let texture_format = match TextureFormat::from_u32(format) {
            Some(tf) => tf,
            None => {
                panic!("Invalid texture format: {}", format);
            }
        };
        encode_into(texture_format, dst_slice, src_slice, width, height);
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_encode_cmpr(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len as usize);
        let src_slice = slice::from_raw_parts(src as *const u8, src_len as usize);
        encode_cmpr_into(dst_slice, src_slice, width, height);
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_encode_i4(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len as usize);
        let src_slice = slice::from_raw_parts(src as *const u8, src_len as usize);
        encode_i4_into(dst_slice, src_slice, width, height);
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_encode_i8(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len as usize);
        let src_slice = slice::from_raw_parts(src as *const u8, src_len as usize);
        encode_i8_into(dst_slice, src_slice, width, height);
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_encode_ia4(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len as usize);
        let src_slice = slice::from_raw_parts(src as *const u8, src_len as usize);
        encode_ia4_into(dst_slice, src_slice, width, height);
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_encode_ia8(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len as usize);
        let src_slice = slice::from_raw_parts(src as *const u8, src_len as usize);
        encode_ia8_into(dst_slice, src_slice, width, height);
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_encode_rgb565(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len as usize);
        let src_slice = slice::from_raw_parts(src as *const u8, src_len as usize);
        encode_rgb565_into(dst_slice, src_slice, width, height);
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_encode_rgb5a3(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len as usize);
        let src_slice = slice::from_raw_parts(src as *const u8, src_len as usize);
        encode_rgb5a3_into(dst_slice, src_slice, width, height);
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_encode_rgba8(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len as usize);
        let src_slice = slice::from_raw_parts(src as *const u8, src_len as usize);
        encode_rgba8_into(dst_slice, src_slice, width, height);
    }

    #[no_mangle]
    pub unsafe extern "C" fn rii_decode(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        width: u32,
        height: u32,
        texformat: u32,
        tlut: *const u8,
        tlut_len: u32,
        tlutformat: u32,
    ) {
        let dst_slice = slice::from_raw_parts_mut(dst, dst_len as usize);
        let src_slice = slice::from_raw_parts(src, src_len as usize);
        let tlut_slice = slice::from_raw_parts(tlut, tlut_len as usize);
        decode_into(
            dst_slice,
            src_slice,
            width,
            height,
            TextureFormat::from_u32(texformat).unwrap(),
            tlut_slice,
            tlutformat,
        );
    }

    #[no_mangle]
    pub extern "C" fn gctex_get_version_unstable_api(buffer: *mut u8, length: u32) -> i32 {
        let version_info = get_version();
        let string_length = version_info.len();

        if string_length > length as usize {
            return -1;
        }

        let buffer_slice = unsafe {
            assert!(!buffer.is_null());
            std::slice::from_raw_parts_mut(buffer, length as usize)
        };

        for (i, byte) in version_info.as_bytes().iter().enumerate() {
            buffer_slice[i] = *byte;
        }

        string_length as i32
    }
}
