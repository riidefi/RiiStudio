use std::slice;

#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
pub mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

pub const _CTF: u32 = 0x20;
pub const _ZTF: u32 = 0x10;

#[repr(u32)]
#[derive(Debug, Copy, Clone)]
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

fn get_format_info(format: u32) -> ImageFormatInfo {
    let (xshift, yshift) = match format {
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
    if format == TextureFormat::RGBA8 as u32 || format == ZTextureFormat::Z24X8 as u32 {
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

#[no_mangle]
pub fn rii_compute_image_size(format: u32, width: u32, height: u32) -> u32 {
    if format == TextureFormat::ExtensionRawRGBA32 as u32 {
        return width * height * 4;
    }
    let info = get_format_info(format);
    info_compute_image_size(info, width, height)
}

#[no_mangle]
pub fn rii_compute_image_size_mip(
    format: u32,
    mut width: u32,
    mut height: u32,
    number_of_images: u32,
) -> u32 {
    if number_of_images <= 1 {
        return rii_compute_image_size(format, width, height);
    }

    let mut size = 0;

    for _i in 0..number_of_images {
        size += rii_compute_image_size(format, width, height);

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
        let format = 9;
        let width = 128;
        let height = 128;
        let number_of_images = 1;

        let result = rii_compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 16384);
    }

    #[test]
    fn test_rii_compute_image_size_mip_with_1x1() {
        let format = 0xE;
        let width = 16;
        let height = 16;
        let number_of_images = 5;

        let result = rii_compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 256);
    }

    #[test]
    fn test_rii_compute_image_size_mip_with_small_size() {
        let format = 1;
        let width = 10;
        let height = 10;
        let number_of_images = 1;

        let result = rii_compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 192);
    }

    #[test]
    fn test_rii_compute_image_size_mip_with_large_size() {
        let format = 1;
        let width = 1000;
        let height = 1000;
        let number_of_images = 1;

        let result = rii_compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 1000000);
    }

    #[test]
    fn test_rii_compute_image_size_mip_with_multiple_images() {
        let format = 1;
        let width = 1000;
        let height = 1000;
        let number_of_images = 5;

        let result = rii_compute_image_size_mip(format, width, height, number_of_images);

        assert_eq!(result, 1336992);
    }
}

// Requires expanded size to be block-aligned
pub fn decode_fast(
    dst: &mut [u8],
    src: &[u8],
    width: u32,
    height: u32,
    texformat: u32,
    tlut: &[u8],
    tlutformat: u32,
) {
    let info = get_format_info(texformat);
    let expanded_width = round_up(width, info.block_width_in_texels);
    let expanded_height = round_up(height, info.block_height_in_texels);
    assert!(dst.len() as u32 >= expanded_width * expanded_height * 4);
    assert!(src.len() as u32 >= rii_compute_image_size(texformat, width, height));
    unsafe {
        bindings::impl_rii_decode(
            dst.as_mut_ptr(),
            src.as_ptr(),
            width,
            height,
            texformat,
            tlut.as_ptr(),
            tlutformat,
        );
    }
}

pub fn decode_into(
    dst: &mut [u8],
    src: &[u8],
    width: u32,
    height: u32,
    texformat: u32,
    tlut: &[u8],
    tlutformat: u32,
) {
    assert!(dst.len() as u32 >= width * height * 4);

    let info = get_format_info(texformat);
    let expanded_width = round_up(width, info.block_width_in_texels);
    let expanded_height = round_up(height, info.block_height_in_texels);

    assert!(src.len() as u32 >= rii_compute_image_size(texformat, width, height));

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
        decode_fast(&mut dst, &src, 500, 500, 0xE, tlut, 0);

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
        decode(&mut dst, &src, 500, 500, 0xE, tlut, 0);

        let mut expected_file = File::open("tests/monke_expected_result").unwrap();
        let mut expected_dst = Vec::new();
        expected_file.read_to_end(&mut expected_dst).unwrap();
        expected_dst.resize(500 * 500 * 4, 0);

        assert_eq!(dst, expected_dst);
    }
}

pub fn decode(
    src: &[u8],
    width: u32,
    height: u32,
    texformat: u32,
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

pub fn encode_i4_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeI4(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

pub fn encode_i8_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeI8(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

pub fn encode_ia4_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeIA4(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

pub fn encode_ia8_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeIA8(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

pub fn encode_rgb565_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeRGB565(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

pub fn encode_rgb5a3_into(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeRGB5A3(dst.as_mut_ptr(), src.as_ptr(), width, height);
    }
}

pub fn encode_rgba8_into(dst: &mut [u8], src4: &[u8], width: u32, height: u32) {
    unsafe {
        bindings::impl_rii_encodeRGBA8(dst.as_mut_ptr(), src4.as_ptr(), width, height);
    }
}

pub fn encode_into(format: TextureFormat, dst: &mut [u8], src: &[u8], width: u32, height: u32) {
    match format {
        TextureFormat::I4 => encode_i4_into(dst, src, width, height),
        TextureFormat::I8 => encode_i8_into(dst, src, width, height),
        TextureFormat::IA4 => encode_ia4_into(dst, src, width, height),
        TextureFormat::IA8 => encode_ia8_into(dst, src, width, height),
        TextureFormat::RGB565 => encode_rgb565_into(dst, src, width, height),
        TextureFormat::RGB5A3 => encode_rgb5a3_into(dst, src, width, height),
        TextureFormat::RGBA8 => encode_rgba8_into(dst, src, width, height),
        TextureFormat::CMPR => encode_cmpr_into(dst, src, width, height),
        _ => panic!("Unsupported texture format: {:?}", format),
    }
}

pub fn encode(format: TextureFormat, src: &[u8], width: u32, height: u32) -> Vec<u8> {
    let size = rii_compute_image_size(format as u32, width, height) as usize;
    let mut dst = vec![0u8; size];

    encode_into(format, &mut dst, src, width, height);

    dst
}

pub fn get_version() -> &'static str {
    env!("CARGO_PKG_VERSION")
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
        dst_slice, src_slice, width, height, texformat, tlut_slice, tlutformat,
    );
}

#[no_mangle]
pub extern "C" fn gctex_get_version_unstable_api(buffer: *mut u8, length: u32) -> i32 {
    let pkg_version = env!("CARGO_PKG_VERSION");
    let profile = if cfg!(debug_assertions) {
        "debug"
    } else {
        "release"
    };
    let target = env!("TARGET");
    let version_info = format!(
        "Version: {}, Profile: {}, Target: {}",
        pkg_version, profile, target
    );

    let string_length = version_info.len();

    if string_length > length as usize {
        return -1;
    }

    let buffer_slice = unsafe {
        assert!(!buffer.is_null());
        slice::from_raw_parts_mut(buffer, length as usize)
    };

    for (i, byte) in version_info.as_bytes().iter().enumerate() {
        buffer_slice[i] = *byte;
    }

    string_length as i32
}
