use libc::{c_uint, c_void, size_t};
use std::slice;

pub mod librii {
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

        C4,
        C8,
        C14X2,
        CMPR = 0xE,

        ExtensionRawRGBA32 = 0xFF,
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

    #[derive(Debug, Copy, Clone)]
    struct ImageFormatInfo {
        xshift: u32,
        yshift: u32,
        bitsize: u32,
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
        }
    }

    fn info_compute_image_size(info: ImageFormatInfo, width: u32, height: u32) -> u32 {
        let xtiles = ((width + (1 << info.xshift)) - 1) >> info.xshift;
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
            if width == 1 && height == 1 {
                break;
            }

            size += rii_compute_image_size(format, width, height);

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

    pub fn rii_decode(
        dst: &mut [u8],
        src: &[u8],
        width: u32,
        height: u32,
        texformat: u32,
        tlut: &[u8],
        tlutformat: u32,
    ) {
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

    pub fn rii_encode_cmpr(dst: &mut [u8], src: &[u8], width: u32, height: u32) {
        unsafe {
            bindings::impl_rii_encodeCMPR(dst.as_mut_ptr(), src.as_ptr(), width, height);
        }
    }

    pub fn rii_encode_i4(dst: &mut [u8], src: &[u32], width: u32, height: u32) {
        unsafe {
            bindings::impl_rii_encodeI4(dst.as_mut_ptr(), src.as_ptr(), width, height);
        }
    }

    pub fn rii_encode_i8(dst: &mut [u8], src: &[u32], width: u32, height: u32) {
        unsafe {
            bindings::impl_rii_encodeI8(dst.as_mut_ptr(), src.as_ptr(), width, height);
        }
    }

    pub fn rii_encode_ia4(dst: &mut [u8], src: &[u32], width: u32, height: u32) {
        unsafe {
            bindings::impl_rii_encodeIA4(dst.as_mut_ptr(), src.as_ptr(), width, height);
        }
    }

    pub fn rii_encode_ia8(dst: &mut [u8], src: &[u32], width: u32, height: u32) {
        unsafe {
            bindings::impl_rii_encodeIA8(dst.as_mut_ptr(), src.as_ptr(), width, height);
        }
    }

    pub fn rii_encode_rgb565(dst: &mut [u8], src: &[u32], width: u32, height: u32) {
        unsafe {
            bindings::impl_rii_encodeRGB565(dst.as_mut_ptr(), src.as_ptr(), width, height);
        }
    }

    pub fn rii_encode_rgb5a3(dst: &mut [u8], src: &[u32], width: u32, height: u32) {
        unsafe {
            bindings::impl_rii_encodeRGB5A3(dst.as_mut_ptr(), src.as_ptr(), width, height);
        }
    }

    pub fn rii_encode_rgba8(dst: &mut [u8], src4: &[u32], width: u32, height: u32) {
        unsafe {
            bindings::impl_rii_encodeRGBA8(dst.as_mut_ptr(), src4.as_ptr(), width, height);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn rii_encode_cmpr(
    dst: *mut c_void,
    dst_len: size_t,
    src: *const c_void,
    src_len: size_t,
    width: c_uint,
    height: c_uint,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u8, src_len);
    librii::rii_encode_cmpr(dst_slice, src_slice, width, height);
}

#[no_mangle]
pub unsafe extern "C" fn rii_encode_i4(
    dst: *mut c_void,
    dst_len: size_t,
    src: *const c_void,
    src_len: size_t,
    width: c_uint,
    height: c_uint,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u32, src_len);
    librii::rii_encode_i4(dst_slice, src_slice, width, height);
}

#[no_mangle]
pub unsafe extern "C" fn rii_encode_i8(
    dst: *mut c_void,
    dst_len: size_t,
    src: *const c_void,
    src_len: size_t,
    width: c_uint,
    height: c_uint,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u32, src_len);
    librii::rii_encode_i8(dst_slice, src_slice, width, height);
}

#[no_mangle]
pub unsafe extern "C" fn rii_encode_ia4(
    dst: *mut c_void,
    dst_len: size_t,
    src: *const c_void,
    src_len: size_t,
    width: c_uint,
    height: c_uint,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u32, src_len);
    librii::rii_encode_ia4(dst_slice, src_slice, width, height);
}

#[no_mangle]
pub unsafe extern "C" fn rii_encode_ia8(
    dst: *mut c_void,
    dst_len: size_t,
    src: *const c_void,
    src_len: size_t,
    width: c_uint,
    height: c_uint,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u32, src_len);
    librii::rii_encode_ia8(dst_slice, src_slice, width, height);
}

#[no_mangle]
pub unsafe extern "C" fn rii_encode_rgb565(
    dst: *mut c_void,
    dst_len: size_t,
    src: *const c_void,
    src_len: size_t,
    width: c_uint,
    height: c_uint,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u32, src_len);
    librii::rii_encode_rgb565(dst_slice, src_slice, width, height);
}

#[no_mangle]
pub unsafe extern "C" fn rii_encode_rgb5a3(
    dst: *mut c_void,
    dst_len: size_t,
    src: *const c_void,
    src_len: size_t,
    width: c_uint,
    height: c_uint,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u32, src_len);
    librii::rii_encode_rgb5a3(dst_slice, src_slice, width, height);
}

#[no_mangle]
pub unsafe extern "C" fn rii_encode_rgba8(
    dst: *mut c_void,
    dst_len: size_t,
    src: *const c_void,
    src_len: size_t,
    width: c_uint,
    height: c_uint,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u32, src_len);
    librii::rii_encode_rgba8(dst_slice, src_slice, width, height);
}

#[no_mangle]
pub unsafe extern "C" fn rii_decode(
    dst: *mut c_void,
    dst_len: size_t,
    src: *const c_void,
    src_len: size_t,
    width: u32,
    height: u32,
    texformat: u32,
    tlut: *const c_void,
    tlut_len: size_t,
    tlutformat: u32,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u8, src_len);
    let tlut_slice = slice::from_raw_parts(tlut as *const u8, tlut_len);
    librii::rii_decode(
        dst_slice, src_slice, width, height, texformat, tlut_slice, tlutformat,
    );
}
