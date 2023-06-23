#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
pub mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
#[no_mangle]
pub extern "C" fn rsl_init_llvm(
    argc: *mut ::std::os::raw::c_int,
    argv: *mut *mut *const ::std::os::raw::c_char,
    installPipeSignalExitHandler: ::std::os::raw::c_int,
) -> *mut bindings::llvm_InitLLVM {
    println!("bindings::init_llvm");
    unsafe {
        bindings::init_llvm(argc, argv, installPipeSignalExitHandler)
    }
}
#[no_mangle]
pub extern "C" fn rsl_deinit_llvm(llvm: *mut bindings::llvm_InitLLVM) {
    println!("bindings::deinit_llvm");
    unsafe {
        bindings::deinit_llvm(llvm)
    }
}

