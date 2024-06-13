use anyhow::bail;
use std::ffi::c_void;
use std::ffi::CString;
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
        pub fn imp_brres_read_from_bytes(result: *mut CBrres, buf: *const u8, len: u32) -> u32;
        pub fn imp_brres_write_bytes(
            result: *mut CBrres,
            json: *const i8,
            json_len: u32,
            buffer: *const c_void,
            buffer_len: u32,
        ) -> u32;
        pub fn imp_brres_free(result: *mut CBrres);

        pub fn imp_brres_read_mdl0mat_preset(
            result: *mut CBrres,
            path: *const i8,
            path_len: u32,
        ) -> u32;
    }
}

pub struct CBrresWrapper<'a> {
    pub json_metadata: &'a str,
    pub buffer_data: &'a [u8],
    result: Box<bindings::CBrres>,
}

impl<'a> CBrresWrapper<'a> {
    pub fn default() -> bindings::CBrres {
        bindings::CBrres {
            json_metadata: ptr::null(),
            len_json_metadata: 0,
            buffer_data: ptr::null(),
            len_buffer_data: 0,
            freeResult: dummy_free_result,
            opaque: ptr::null_mut(),
        }
    }

    pub fn from_bytes(buf: &[u8]) -> anyhow::Result<Self> {
        unsafe {
            let mut result: Box<bindings::CBrres> = Box::new(Self::default());

            let ok = bindings::imp_brres_read_from_bytes(
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
    pub fn write_bytes(json: &str, buffer: &[u8]) -> anyhow::Result<Self> {
        let json_cstring = CString::new(json)?;
        let json_ptr = json_cstring.as_ptr();
        let json_len = json.len() as u32;
        let buffer_ptr = buffer.as_ptr() as *const c_void;
        let buffer_len = buffer.len() as u32;

        unsafe {
            let mut result: Box<bindings::CBrres> = Box::new(Self::default());

            let ok = bindings::imp_brres_write_bytes(
                &mut *result as *mut bindings::CBrres,
                json_ptr,
                json_len,
                buffer_ptr,
                buffer_len,
            );

            let json_metadata = {
                let slice = std::slice::from_raw_parts(
                    result.json_metadata,
                    result.len_json_metadata as usize,
                );
                std::str::from_utf8(slice)?
            };

            if ok == 0 {
                anyhow::bail!("Failed to write BRRES file: {json_metadata}");
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
    pub fn read_preset_folder(json: &str) -> anyhow::Result<Self> {
        let json_cstring = CString::new(json)?;
        let json_ptr = json_cstring.as_ptr();
        let json_len = json.len() as u32;

        unsafe {
            let mut result: Box<bindings::CBrres> = Box::new(Self::default());

            let ok = bindings::imp_brres_read_mdl0mat_preset(
                &mut *result as *mut bindings::CBrres,
                json_ptr,
                json_len,
            );

            if ok == 0 {
                let json_metadata = {
                    let slice = std::slice::from_raw_parts(
                        result.json_metadata,
                        result.len_json_metadata as usize,
                    );
                    std::str::from_utf8(slice)?
                };
                anyhow::bail!("Failed to write BRRES file: {json_metadata}");
            }
            let buffer_data =
                std::slice::from_raw_parts(result.buffer_data, result.len_buffer_data as usize);

            Ok(CBrresWrapper {
                json_metadata: "",
                buffer_data,
                result,
            })
        }
    }
}

impl<'a> Drop for CBrresWrapper<'a> {
    fn drop(&mut self) {
        // unsafe {
        (self.result.freeResult)(&mut *self.result as *mut bindings::CBrres);
        // }
    }
}

extern "C" fn dummy_free_result(_: *mut bindings::CBrres) {
    // This is a dummy function to avoid calling a null function pointer.
}

// Reentrant Rust (this crate) -> C++ -> Rust (other crate)
pub mod c_api {
    use crate::*;
    use core::ffi::c_char;
    use gctex;
    use wiitrig;

    // use simple_logger::SimpleLogger;

    // use log::*;

    #[no_mangle]
    pub unsafe extern "C" fn rii_compute_image_size2(format: u32, width: u32, height: u32) -> u32 {
        gctex::c_api::rii_compute_image_size(format, width, height)
    }

    #[no_mangle]
    pub unsafe extern "C" fn rsl_log_init2() {
        // SimpleLogger::new().init().unwrap();
    }

    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_debug2(_s: *const c_char, _len: u32) {
        // // TODO: Use len
        // let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
        // debug!("{}", &st);
    }
    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_error2(_s: *const c_char, _len: u32) {
        // // TODO: Use len
        // let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
        // error!("{}", &st);
    }
    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_info2(_s: *const c_char, _len: u32) {
        // // TODO: Use len
        // let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
        // info!("{}", &st);
    }
    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_trace2(_s: *const c_char, _len: u32) {
        // // TODO: Use len
        // let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
        // trace!("{}", &st);
    }
    #[no_mangle]
    pub unsafe extern "C" fn rsl_c_warn2(_s: *const c_char, _len: u32) {
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

    use core::ffi::c_void;

    #[cfg(feature = "c_api")]
    #[no_mangle]
    pub unsafe fn brres_read_from_bytes(
        result: *mut ffi::bindings::CBrres,
        buf: *const u8,
        len: u32,
    ) -> u32 {
        ffi::bindings::imp_brres_read_from_bytes(result, buf, len)
    }
    #[cfg(feature = "c_api")]
    #[no_mangle]
    pub unsafe fn brres_write_bytes(
        result: *mut ffi::bindings::CBrres,
        json: *const i8,
        json_len: u32,
        buffer: *const c_void,
        buffer_len: u32,
    ) -> u32 {
        ffi::bindings::imp_brres_write_bytes(result, json, json_len, buffer, buffer_len)
    }
    #[cfg(feature = "c_api")]
    #[no_mangle]
    pub unsafe fn brres_free(result: *mut ffi::bindings::CBrres) {
        ffi::bindings::imp_brres_free(result)
    }
    #[cfg(feature = "c_api")]
    #[no_mangle]
    pub unsafe fn brres_read_mdl0mat_preset(
        result: *mut ffi::bindings::CBrres,
        path: *const i8,
        path_len: u32,
    ) {
        ffi::bindings::imp_brres_read_mdl0mat_preset(result, path, path_len);
    }
}
