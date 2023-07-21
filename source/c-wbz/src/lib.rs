use std::ffi::CStr;
use std::io::Cursor;
use std::os::raw::{c_char, c_void};
use std::path::Path;
use std::slice;

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

#[repr(C)]
pub struct Buffer {
    begin: *mut c_void,
    len: u32,
}

#[no_mangle]
pub extern "C" fn wbzrs_decode_wbz(
    wbz_buffer: *const c_void,
    wbz_len: u32,
    autoadd_path: *const c_char,
    out_buffer: *mut Buffer,
) -> i32 {
    let c_str_path = unsafe {
        assert!(!autoadd_path.is_null());

        CStr::from_ptr(autoadd_path)
    };

    let path_str = match c_str_path.to_str() {
        Ok(s) => s,
        Err(_) => return ErrorCode::ERR_INVALID_STRING as i32,
    };

    let path = Path::new(path_str);

    let data_slice = unsafe { slice::from_raw_parts(wbz_buffer as *const u8, wbz_len as usize) };

    match wbz_converter::decode_wbz(Cursor::new(data_slice), path) {
        Ok(result) => {
            // Allocates memory for the result and copies the result into it
            let len = result.len();
            let begin = unsafe { libc::malloc(len) };
            if begin.is_null() {
                return ErrorCode::ERR_FILE_OPERATION_FAILED as i32;
            }
            unsafe {
                std::ptr::copy_nonoverlapping(result.as_ptr(), begin as *mut u8, len);
                (*out_buffer).begin = begin;
                (*out_buffer).len = len as u32;
            }

            ErrorCode::ERR_OK as i32
        }
        Err(e) => error_to_code(e) as i32,
    }
}

#[no_mangle]
pub extern "C" fn wbzrs_decode_wu8(
    wu8_buffer: *mut c_void,
    wu8_len: u32,
    autoadd_path: *const c_char,
) -> i32 {
    let c_str_path = unsafe {
        assert!(!autoadd_path.is_null());

        CStr::from_ptr(autoadd_path)
    };

    let path_str = match c_str_path.to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    let path = Path::new(path_str);

    let data_slice = unsafe { slice::from_raw_parts_mut(wu8_buffer as *mut u8, wu8_len as usize) };

    match wbz_converter::decode_wu8(data_slice, path) {
        Ok(_) => ErrorCode::ERR_OK as i32,
        Err(e) => error_to_code(e) as i32,
    }
}
