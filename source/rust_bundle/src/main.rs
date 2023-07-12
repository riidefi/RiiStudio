extern crate libc;
use libc::c_char;
use std::ffi::CString;
use std::os::raw::c_int;

extern "C" {
    fn RiiStudio_main(argc: c_int, argv: *const *const c_char) -> c_int;
}

fn main() {
    let args: Vec<CString> = std::env::args().map(|arg| CString::new(arg).unwrap()).collect();
    let c_args: Vec<*const c_char> = args.iter().map(|arg| arg.as_ptr()).collect();

    println!("Invoking from Rust!");

    unsafe {
        RiiStudio_main(c_args.len() as c_int, c_args.as_ptr());
    }
}
