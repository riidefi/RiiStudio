use std::ffi::CStr;
use std::os::raw::c_char;
use unzpack::Unzpack;
use simple_logger::SimpleLogger;

use log::*;

/*
use std::fs::OpenOptions;
use std::os::windows::prelude::*;
*/

pub fn rsl_extract_zip(from_file: &str, to_folder: &str) -> Result<(), String> {
  // This fails for some reason with ERROR_SHARING_VIOLATION
  // Yet doing a simple fopen in C++ works..

  /*let outpath = "C:\\Users\\rii\\Documents\\dev\\RiiStudio\\out\\build\\Clang-x64-DIST\\source\\frontend\\assimp-vc141-mt.dll";
  OpenOptions::new()
                .write(true)
                .truncate(true)
                .access_mode(0x40000000)
                .share_mode(7)
                .open(&outpath).unwrap();
                */
  match Unzpack::extract( from_file, to_folder) {
     Ok(_) => Ok(()),
     Err(_) => Err("Failed to extract".to_string()),
   }
}

#[no_mangle]
pub unsafe fn c_rsl_extract_zip(from_file: *const c_char, to_folder: *const c_char) -> i32 {
  if from_file.is_null() || to_folder.is_null() {
    return -1
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
    },
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
