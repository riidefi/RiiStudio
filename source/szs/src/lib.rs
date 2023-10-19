use core::ffi::c_char;
use core::slice;

#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

pub fn is_compressed(src: &[u8]) -> bool {
    if src.len() < 8 {
        return false;
    }
    src.starts_with(b"Yaz0")
}

pub fn decoded_size(src: &[u8]) -> u32 {
    if src.len() < 8 || !src.starts_with(b"Yaz0") {
        return 0;
    }
    ((src[4] as u32) << 24) | ((src[5] as u32) << 16) | ((src[6] as u32) << 8) | (src[7] as u32)
}

pub enum EncodeAlgoError {
    Error(String),
}

pub fn encoded_upper_bound(len: u32) -> u32 {
    unsafe { bindings::impl_rii_worst_encoding_size(len) }
}

#[repr(u32)]
pub enum EncodeAlgo {
    WorstCaseEncoding = 0,
    Nintendo,
    MkwSp,
    CTGP,
    Haroohie,
    CTLib,
    LibYaz0,
    Mk8,
}

// In-place encode
pub fn encode_inplace(
    dst: &mut [u8],
    src: &[u8],
    algo: EncodeAlgo,
) -> Result<u32, EncodeAlgoError> {
    let mut used_len: u32 = 0;

    let result = unsafe {
        bindings::impl_rii_encodeAlgo(
            dst.as_mut_ptr() as *mut _,
            dst.len() as u32,
            src.as_ptr() as *const _,
            src.len() as u32,
            &mut used_len,
            algo as u32,
        )
    };

    if result.is_null() {
        Ok(used_len)
    } else {
        let error_msg = unsafe {
            std::ffi::CStr::from_ptr(result)
                .to_string_lossy()
                .into_owned()
        };
        Err(EncodeAlgoError::Error(error_msg))
    }
}

pub fn encode(src: &[u8], algo: EncodeAlgo) -> Result<Vec<u8>, EncodeAlgoError> {
    // Calculate the upper bound for encoding.
    let max_len = encoded_upper_bound(src.len() as u32);

    // Allocate a buffer based on the calculated upper bound.
    let mut dst: Vec<u8> = vec![0; max_len as usize];

    match encode_inplace(&mut dst, src, algo) {
        Ok(encoded_len) => {
            // Shrink the dst to the actual size.
            dst.truncate(encoded_len as usize);
            Ok(dst)
        }
        Err(err) => Err(err),
    }
}

pub fn decode_inplace(dst: &mut [u8], src: &[u8]) -> Result<(), EncodeAlgoError> {
    let result = unsafe {
        bindings::impl_riiszs_decode(
            dst.as_mut_ptr() as *mut _,
            dst.len() as u32,
            src.as_ptr() as *const _,
            src.len() as u32,
        )
    };

    if result.is_null() {
        Ok(())
    } else {
        let error_msg = unsafe {
            std::ffi::CStr::from_ptr(result)
                .to_string_lossy()
                .into_owned()
        };
        Err(EncodeAlgoError::Error(error_msg))
    }
}

pub fn decode(src: &[u8]) -> Result<Vec<u8>, EncodeAlgoError> {
    // Calculate the required size for decoding.
    let req_len = decoded_size(src);

    // Allocate a buffer based on the calculated required size.
    let mut dst: Vec<u8> = vec![0; req_len as usize];

    match decode_inplace(&mut dst, src) {
        Ok(_) => Ok(dst),
        Err(err) => Err(err),
    }
}

pub fn deinterlace_into(dst: &mut [u8], src: &[u8]) -> Result<u32, EncodeAlgoError> {
    let mut used_len: u32 = 0;

    let result = unsafe {
        bindings::impl_rii_deinterlace(
            dst.as_mut_ptr() as *mut _,
            dst.len() as u32,
            src.as_ptr() as *const _,
            src.len() as u32,
            &mut used_len,
        )
    };

    if result.is_null() {
        Ok(used_len)
    } else {
        let error_msg = unsafe {
            std::ffi::CStr::from_ptr(result)
                .to_string_lossy()
                .into_owned()
        };
        Err(EncodeAlgoError::Error(error_msg))
    }
}

pub fn deinterlaced_upper_bound(len: u32) -> u32 {
    return len + 3;
}

pub fn deinterlace(src: &[u8]) -> Result<Vec<u8>, EncodeAlgoError> {
    // Calculate the upper bound for conversion.
    let max_len = deinterlaced_upper_bound(src.len() as u32);

    // Allocate a buffer based on the calculated upper bound.
    let mut dst: Vec<u8> = vec![0; max_len as usize];

    match deinterlace_into(&mut dst, src) {
        Ok(encoded_len) => {
            // Shrink the dst to the actual size.
            dst.truncate(encoded_len as usize);
            Ok(dst)
        }
        Err(err) => Err(err),
    }
}

//--------------------------------------------------------
//
// C BINDINGS BEGIN
//
//--------------------------------------------------------

#[no_mangle]
pub extern "C" fn szs_get_version_unstable_api(buffer: *mut u8, length: u32) -> i32 {
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

#[no_mangle]
pub extern "C" fn riiszs_encoded_upper_bound(len: u32) -> u32 {
    encoded_upper_bound(len)
}

#[no_mangle]
pub extern "C" fn riiszs_encode_algo_fast(
    dst: *mut u8,
    dst_len: u32,
    src: *const u8,
    src_len: u32,
    result: *mut u32,
    algo: EncodeAlgo, // u32
) -> *const c_char {
    let dst_slice = unsafe { std::slice::from_raw_parts_mut(dst, dst_len as usize) };
    let src_slice = unsafe { std::slice::from_raw_parts(src, src_len as usize) };

    match encode_inplace(dst_slice, src_slice, algo) {
        Ok(used_len) => {
            unsafe {
                *result = used_len;
            }
            std::ptr::null()
        }
        Err(EncodeAlgoError::Error(msg)) => {
            let c_string = std::ffi::CString::new(msg).unwrap();
            // Leak the CString into a raw pointer, so we don't deallocate it
            c_string.into_raw()
        }
    }
}

#[no_mangle]
pub extern "C" fn riiszs_decode(
    dst: *mut u8,
    dst_len: u32,
    src: *const u8,
    src_len: u32,
) -> *const c_char {
    let dst_slice = unsafe { std::slice::from_raw_parts_mut(dst, dst_len as usize) };
    let src_slice = unsafe { std::slice::from_raw_parts(src, src_len as usize) };

    match decode_inplace(dst_slice, src_slice) {
        Ok(()) => std::ptr::null(),
        Err(EncodeAlgoError::Error(msg)) => {
            let c_string = std::ffi::CString::new(msg).unwrap();
            // Leak the CString into a raw pointer, so we don't deallocate it
            c_string.into_raw()
        }
    }
}

// For freeing the error message from C side when done
#[no_mangle]
pub extern "C" fn riiszs_free_error_message(err_ptr: *mut c_char) {
    unsafe {
        if !err_ptr.is_null() {
            let _ = std::ffi::CString::from_raw(err_ptr);
        }
    }
}

#[no_mangle]
pub extern "C" fn riiszs_is_compressed(src: *const u8, len: u32) -> bool {
    let data = unsafe { std::slice::from_raw_parts(src, len as usize) };
    is_compressed(data)
}

#[no_mangle]
pub extern "C" fn riiszs_decoded_size(src: *const u8, len: u32) -> u32 {
    let data = unsafe { std::slice::from_raw_parts(src, len as usize) };
    decoded_size(data)
}

#[no_mangle]
pub extern "C" fn riiszs_deinterlaced_upper_bound(len: u32) -> u32 {
    deinterlaced_upper_bound(len)
}

#[no_mangle]
pub extern "C" fn riiszs_deinterlace_into(
    dst: *mut u8,
    dst_len: u32,
    src: *const u8,
    src_len: u32,
    result: *mut u32,
) -> *const c_char {
    let dst_slice = unsafe { std::slice::from_raw_parts_mut(dst, dst_len as usize) };
    let src_slice = unsafe { std::slice::from_raw_parts(src, src_len as usize) };

    match deinterlace_into(dst_slice, src_slice) {
        Ok(used_len) => {
            unsafe {
                *result = used_len;
            }
            std::ptr::null()
        }
        Err(EncodeAlgoError::Error(msg)) => {
            let c_string = std::ffi::CString::new(msg).unwrap();
            // Leak the CString into a raw pointer, so we don't deallocate it
            c_string.into_raw()
        }
    }
}
