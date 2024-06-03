use std::ffi::CStr;
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
        pub fn brres_read_from_bytes(result: *mut CBrres, buf: *const u8, len: u32);
    }
}

pub struct CBrresWrapper<'a> {
    pub json_metadata: &'a str,
    pub buffer_data: &'a [u8],
    result: Box<bindings::CBrres>,
}

impl<'a> CBrresWrapper<'a> {
    pub fn from_bytes(buf: &[u8]) -> Self {
        unsafe {
            let mut result: Box<bindings::CBrres> = Box::new(bindings::CBrres {
                json_metadata: ptr::null(),
                len_json_metadata: 0,
                buffer_data: ptr::null(),
                len_buffer_data: 0,
                freeResult: dummy_free_result,
                opaque: ptr::null_mut(),
            });

            bindings::brres_read_from_bytes(
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

            let buffer_data =
                std::slice::from_raw_parts(result.buffer_data, result.len_buffer_data as usize);

            CBrresWrapper {
                json_metadata,
                buffer_data,
                result,
            }
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
