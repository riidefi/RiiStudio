use anyhow::bail;
use std::ptr;

#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
pub mod bindings {
    use std::os::raw::c_void;

    #[repr(C)]
    pub struct CBrres {
        pub json_metadata: *const u8,
        pub len_json_metadata: u32,
        pub buffer_data: *const u8,
        pub len_buffer_data: u32,
        pub freeResult: extern "C" fn(*mut CBrres),
        pub opaque: *mut c_void,
    }

    extern "C" {
        pub fn brres_read_from_bytes(result: *mut CBrres, buf: *const u8, len: u32) -> u32;
    }
}

pub struct CBrresWrapper<'a> {
    pub json_metadata: &'a str,
    pub buffer_data: &'a [u8],
    result: Box<bindings::CBrres>,
}

impl<'a> CBrresWrapper<'a> {
    pub fn from_bytes(buf: &[u8]) -> anyhow::Result<Self> {
        unsafe {
            let mut result: Box<bindings::CBrres> = Box::new(bindings::CBrres {
                json_metadata: ptr::null(),
                len_json_metadata: 0,
                buffer_data: ptr::null(),
                len_buffer_data: 0,
                freeResult: dummy_free_result,
                opaque: ptr::null_mut(),
            });

            let ok = bindings::brres_read_from_bytes(
                &mut *result as *mut bindings::CBrres,
                buf.as_ptr(),
                buf.len() as u32,
            );

            let json_metadata = {
                let slice = std::slice::from_raw_parts(
                    result.json_metadata,
                    result.len_json_metadata as usize,
                );
                std::str::from_utf8(slice).unwrap()
            };

            if ok == 0 {
                bail!("Failed to read BRRES file: {json_metadata}");
            }

            let buffer_data =
                std::slice::from_raw_parts(result.buffer_data, result.len_buffer_data as usize);

            Ok(CBrresWrapper {
                json_metadata,
                buffer_data,
                result,
            })
        }
    }
}

impl<'a> Drop for CBrresWrapper<'a> {
    fn drop(&mut self) {
        unsafe {
            (self.result.freeResult)(&mut *self.result as *mut bindings::CBrres);
        }
    }
}

extern "C" fn dummy_free_result(_: *mut bindings::CBrres) {
    // This is a dummy function to avoid calling a null function pointer.
}

// Reentrant Rust (this crate) -> C++ -> Rust (other crate)
pub mod c_api {
    use crate::*;
    use core::ffi::c_char;
    use core::ffi::CStr;
    use gctex;
    use wiitrig;

    use simple_logger::SimpleLogger;

    use log::*;

    #[no_mangle]
    pub unsafe extern "C" fn rii_compute_image_size2(format: u32, width: u32, height: u32) -> u32 {
        gctex::c_api::rii_compute_image_size(format, width, height)
    }

    #[no_mangle]
    pub unsafe extern "C" fn rsl_log_init2() {
        // SimpleLogger::new().init().unwrap();
    }

    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_debug2(s: *const c_char, _len: u32) {
        // // TODO: Use len
        // let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
        // debug!("{}", &st);
    }
    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_error2(s: *const c_char, _len: u32) {
        // // TODO: Use len
        // let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
        // error!("{}", &st);
    }
    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_info2(s: *const c_char, _len: u32) {
        // // TODO: Use len
        // let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
        // info!("{}", &st);
    }
    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_trace2(s: *const c_char, _len: u32) {
        // // TODO: Use len
        // let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
        // trace!("{}", &st);
    }
    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_warn2(s: *const c_char, _len: u32) {
        // // TODO: Use len
        // let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
        // warn!("{}", &st);
    }

    #[no_mangle]
    pub unsafe extern "C" fn wii_sin2(x: f32) -> f32 {
        wiitrig::librii::wii_sin(x)
    }

    #[no_mangle]
    pub unsafe extern "C" fn wii_cos2(x: f32) -> f32 {
        wiitrig::librii::wii_cos(x)
    }
}
