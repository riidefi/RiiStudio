use libc::{c_uint, c_void, size_t};
use std::slice;

pub mod librii {

    #[allow(non_upper_case_globals)]
    #[allow(non_camel_case_types)]
    #[allow(non_snake_case)]
    pub mod bindings {
        include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
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
