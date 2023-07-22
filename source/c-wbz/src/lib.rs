use std::ffi::CStr;
use std::io::Cursor;
use std::os::raw::c_char;
use std::path::Path;
use std::slice;

use buffer::Buffer;

mod buffer;

#[repr(i32)]
#[allow(non_camel_case_types)]
pub enum ErrorCode {
    ERR_OK = 0,
    ERR_BZIP,
    ERR_FILE_TOO_BIG,
    ERR_FILE_OPERATION_FAILED,
    ERR_INVALID_WBZ_MAGIC,
    ERR_INVALID_WU8_MAGIC,
    ERR_INVALID_STRING,
    ERR_INVALID_BOOL,
}

fn guard_null<T>(ptr: *const T) -> *const T {
    if ptr.is_null() {
        eprintln!("Null ptr passed to C-WBZ!");
        std::process::abort();
    }

    ptr
}

fn guard_null_mut<T>(ptr: *mut T) -> *mut T {
    guard_null(ptr.cast_const()).cast_mut()
}

fn error_to_code(error: wbz_converter::Error) -> ErrorCode {
    match error {
        wbz_converter::Error::BZip(_) => ErrorCode::ERR_BZIP,
        wbz_converter::Error::FileTooBig(_) => ErrorCode::ERR_FILE_TOO_BIG,
        wbz_converter::Error::FileOperationFailed(_) => ErrorCode::ERR_FILE_OPERATION_FAILED,
        wbz_converter::Error::InvalidWBZMagic { .. } => ErrorCode::ERR_INVALID_WBZ_MAGIC,
        wbz_converter::Error::InvalidWU8Magic { .. } => ErrorCode::ERR_INVALID_WU8_MAGIC,
        wbz_converter::Error::InvalidString(_) => ErrorCode::ERR_INVALID_STRING,
        wbz_converter::Error::InvalidBool(_) => ErrorCode::ERR_INVALID_BOOL,
    }
}

#[no_mangle]
pub extern "C" fn wbzrs_error_to_string(errc: ErrorCode) -> *const c_char {
    let message = match errc {
        ErrorCode::ERR_OK => "No error",
        ErrorCode::ERR_BZIP => "BZip decompression failed",
        ErrorCode::ERR_FILE_TOO_BIG => "The file provided is above 4GB in size",
        ErrorCode::ERR_FILE_OPERATION_FAILED => "Underlying error when reading from file",
        ErrorCode::ERR_INVALID_WBZ_MAGIC => "WBZ file did not contain valid magic",
        ErrorCode::ERR_INVALID_WU8_MAGIC => "WU8 file did not contain valid magic",
        ErrorCode::ERR_INVALID_STRING => "Invalid string error",
        ErrorCode::ERR_INVALID_BOOL => "Invalid boolean value",
    };
    message.as_ptr() as *const c_char
}

/// # Safety
///
/// - wbz_buffer and wbz_len must be valid for [`slice::from_raw_parts`].
/// - autoadd_path must be valid for [`CStr::from_ptr`].
/// - out_buffer's pointer must be freed via [`wbzrs_free_buffer`].
#[no_mangle]
pub unsafe extern "C" fn wbzrs_decode_wbz(
    wbz_buffer: *const u8,
    wbz_len: u32,
    autoadd_path: *const c_char,
    out_buffer: *mut Buffer,
) -> i32 {
    guard_null(wbz_buffer);
    guard_null(autoadd_path);
    guard_null_mut(out_buffer);

    let c_str_path = unsafe {CStr::from_ptr(autoadd_path)};
    let path = match c_str_path.to_str() {
        Ok(s) => Path::new(s),
        Err(_) => return ErrorCode::ERR_INVALID_STRING as i32,
    };

    let data_slice = unsafe { slice::from_raw_parts(wbz_buffer, wbz_len as usize) };
    let u8_buffer = match wbz_converter::decode_wbz(Cursor::new(data_slice), path) {
        Ok(result) => result,
        Err(e) => return error_to_code(e) as i32,
    };

    unsafe {
        *out_buffer = Buffer::from(u8_buffer);
    }

    ErrorCode::ERR_OK as i32
}

/// # Safety
/// - wu8_buffer and wu8_len must be valid for [`slice::from_raw_parts_mut`].
/// - autoadd_path must be valid for [`CStr::from_ptr`].
#[no_mangle]
pub unsafe extern "C" fn wbzrs_decode_wu8(
    wu8_buffer: *mut u8,
    wu8_len: u32,
    autoadd_path: *const c_char,
) -> i32 {
    guard_null_mut(wu8_buffer);
    guard_null(autoadd_path);

    let c_str_path = unsafe {CStr::from_ptr(autoadd_path)};
    let path = match c_str_path.to_str() {
        Ok(s) => Path::new(s),
        Err(_) => return -1,
    };

    let data_slice = unsafe { slice::from_raw_parts_mut(wu8_buffer, wu8_len as usize) };
    match wbz_converter::decode_wu8(data_slice, path) {
        Ok(_) => ErrorCode::ERR_OK as i32,
        Err(e) => error_to_code(e) as i32,
    }
}

/// # Safety
/// - buffer must been returned via this library.
#[no_mangle]
pub unsafe extern "C" fn wbzrs_free_buffer(buffer: *mut Buffer) {
    let buffer = unsafe {*buffer};
    buffer.into_vec();
}
