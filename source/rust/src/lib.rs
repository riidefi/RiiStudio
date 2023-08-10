use simple_logger::SimpleLogger;

use log::*;

use std::io::Cursor;
use std::path::PathBuf;

use std::os::raw::c_int;

use reqwest;
use reqwest::blocking::Client;
use std::error::Error;
use std::ffi::{CStr, CString};
use std::fs::File;
use std::io::Read;
use std::io::Write;
use std::os::raw::{c_char, c_void};
use std::path::Path;

fn utf16_to_utf8(utf16_slice: &[u16]) -> Result<String, std::string::FromUtf16Error> {
    String::from_utf16(utf16_slice)
}

#[no_mangle]
pub extern "C" fn rii_utf16_to_utf8(
    utf16: *const u16,
    len: u32,
    output: *mut u8,
    output_len: u32,
) -> i32 {
    let utf16_slice = unsafe { std::slice::from_raw_parts(utf16, len as usize) };

    match utf16_to_utf8(utf16_slice) {
        Ok(utf8_string) => {
            let utf8_bytes = utf8_string.as_bytes();
            let bytes_to_copy = std::cmp::min(output_len as usize, utf8_bytes.len());

            unsafe {
                std::ptr::copy_nonoverlapping(utf8_bytes.as_ptr(), output, bytes_to_copy);
            }

            0 // Indicate success
        }
        Err(_) => -1, // Indicate error
    }
}

pub fn download_string_rust(url: &str, user_agent: &str) -> Result<String, Box<dyn Error>> {
    let client = reqwest::blocking::Client::new();

    let res = client
        .get(url)
        .header(reqwest::header::USER_AGENT, user_agent)
        .send()? // This will block until complete
        .text()?;

    Ok(res)
}

pub type ProgressFunc = extern "C" fn(*mut c_void, f64, f64, f64, f64) -> i32;

pub fn download_file_rust(
    dest_path: &str,
    url: &str,
    user_agent: &str,
    progress_func: Box<ProgressFunc>,
    progress_data: *mut c_void,
) -> Result<(), Box<dyn std::error::Error>> {
    let path = Path::new(dest_path);
    let mut file = File::create(&path)
        .map_err(|err| format!("Couldn't create {}: {}", path.display(), err))?;

    let client = Client::builder().user_agent(user_agent).build()?;
    let mut response = client.get(url).send()?;

    let total_size = response
        .headers()
        .get(reqwest::header::CONTENT_LENGTH)
        .and_then(|ct_len| ct_len.to_str().ok())
        .and_then(|ct_len| ct_len.parse::<f64>().ok())
        .unwrap_or(0.0);

    let mut buffer = vec![0; 4096]; // 4KB buffer
    let mut current_size = 0.0;

    loop {
        let bytes_read = response.read(&mut buffer)?;
        if bytes_read == 0 {
            break;
        }
        file.write_all(&buffer[..bytes_read])?;
        current_size += bytes_read as f64;
        if progress_func(progress_data, total_size, current_size, 0.0, 0.0) != 0 {
            return Err("Progress function returned non-zero.".into());
        }
    }

    Ok(())
}

#[no_mangle]
pub extern "C" fn download_string(
    url: *const c_char,
    user_agent: *const c_char,
    err: *mut c_int,
) -> *mut c_char {
    let c_url = unsafe { CStr::from_ptr(url).to_str().unwrap() };
    let c_user_agent = unsafe { CStr::from_ptr(user_agent).to_str().unwrap() };

    match download_string_rust(c_url, c_user_agent) {
        Ok(result) => {
            unsafe {
                *err = 0;
            }
            CString::new(result).unwrap().into_raw()
        }
        Err(error) => {
            unsafe {
                *err = 1;
            }
            CString::new(error.to_string()).unwrap().into_raw()
        }
    }
}

#[no_mangle]
pub extern "C" fn download_file(
    dest_path: *const c_char,
    url: *const c_char,
    user_agent: *const c_char,
    progress_func: ProgressFunc,
    progress_data: *mut c_void,
) -> *mut c_char {
    let c_dest_path = unsafe { CStr::from_ptr(dest_path).to_str().unwrap() };
    let c_url = unsafe { CStr::from_ptr(url).to_str().unwrap() };
    let c_user_agent = unsafe { CStr::from_ptr(user_agent).to_str().unwrap() };

    let result = download_file_rust(
        c_dest_path,
        c_url,
        c_user_agent,
        Box::new(progress_func),
        progress_data,
    );
    let response = match result {
        Ok(_) => String::from("Success"),
        Err(e) => e.to_string(),
    };

    CString::new(response).unwrap().into_raw()
}

#[no_mangle]
pub extern "C" fn free_string(s: *mut c_char) {
    unsafe {
        if s.is_null() {
            return;
        }
        let _ = CString::from_raw(s);
    };
}

pub fn rsl_extract_zip(from_file: &str, to_folder: &str) -> Result<(), String> {
    let archive: Vec<u8> = std::fs::read(from_file).expect("Failed to read {from_file}");
    match zip_extract::extract(Cursor::new(&archive), &PathBuf::from(to_folder), false) {
        Ok(_) => Ok(()),
        Err(_) => Err("Failed to extract".to_string()),
    }
}

#[no_mangle]
pub unsafe fn c_rsl_extract_zip(from_file: *const c_char, to_folder: *const c_char) -> i32 {
    if from_file.is_null() || to_folder.is_null() {
        return -1;
    }
    let from_cstr = CStr::from_ptr(from_file);
    let to_cstr = CStr::from_ptr(to_folder);

    let from_str = String::from_utf8_lossy(from_cstr.to_bytes());
    let to_str = String::from_utf8_lossy(to_cstr.to_bytes());

    println!("Extracting zip (from {from_str} to {to_str})");

    match rsl_extract_zip(&from_str, &to_str) {
        Ok(_) => 0,
        Err(err) => {
            println!("Failed: {err}");
            -1
        }
    }
}

#[no_mangle]
pub fn rsl_log_init() {
    SimpleLogger::new().init().unwrap();
}

#[no_mangle]
pub unsafe fn rsl_c_debug(s: *const c_char, _len: u32) {
    // TODO: Use len
    let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
    debug!("{}", &st);
}
#[no_mangle]
pub unsafe fn rsl_c_error(s: *const c_char, _len: u32) {
    // TODO: Use len
    let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
    error!("{}", &st);
}
#[no_mangle]
pub unsafe fn rsl_c_info(s: *const c_char, _len: u32) {
    // TODO: Use len
    let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
    info!("{}", &st);
}
/*
#[no_mangle]
pub unsafe fn rsl_c_log(s: *const c_char, _len: u32) {
  // TODO: Use len
  let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
  log!("{}", &st);
}
*/
#[no_mangle]
pub unsafe fn rsl_c_trace(s: *const c_char, _len: u32) {
    // TODO: Use len
    let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
    trace!("{}", &st);
}
#[no_mangle]
pub unsafe fn rsl_c_warn(s: *const c_char, _len: u32) {
    // TODO: Use len
    let st = String::from_utf8_lossy(CStr::from_ptr(s).to_bytes());
    warn!("{}", &st);
}
