use libc::{c_void, size_t};
use std::slice;

pub mod librii {
    #[allow(non_upper_case_globals)]
    #[allow(non_camel_case_types)]
    #[allow(non_snake_case)]
    pub mod bindings {
        include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
    }

    pub fn avir_resize(dst: &mut [u8], dx: u32, dy: u32, src: &[u8], sx: u32, sy: u32) {
        unsafe {
            bindings::impl_avir_resize(src.as_ptr(), sx, sy, dst.as_mut_ptr(), dx, dy);
        }
    }

    pub fn clancir_resize(dst: &mut [u8], dx: u32, dy: u32, src: &[u8], sx: u32, sy: u32) {
        unsafe {
            bindings::impl_clancir_resize(src.as_ptr(), sx, sy, dst.as_mut_ptr(), dx, dy);
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn avir_resize(
    dst: *mut c_void,
    dst_len: size_t,
    dx: u32,
    dy: u32,
    src: *const c_void,
    src_len: size_t,
    sx: u32,
    sy: u32,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u8, src_len);
    librii::avir_resize(dst_slice, dx, dy, src_slice, sx, sy);
}

#[no_mangle]
pub unsafe extern "C" fn clancir_resize(
    dst: *mut c_void,
    dst_len: size_t,
    dx: u32,
    dy: u32,
    src: *const c_void,
    src_len: size_t,
    sx: u32,
    sy: u32,
) {
    let dst_slice = slice::from_raw_parts_mut(dst as *mut u8, dst_len);
    let src_slice = slice::from_raw_parts(src as *const u8, src_len);
    librii::clancir_resize(dst_slice, dx, dy, src_slice, sx, sy);
}
