extern crate bindgen;
extern crate cc;

use std::env;
use std::path::PathBuf;

fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);

    #[cfg(unix)]
    build.flag("-std=c++17");

    #[cfg(target_arch = "x86_64")]
    build.flag("-DARCH_X64=1");

    build.include(".").include("src");
    build.file("src/bindings.cpp");
    build.file("src/DolphinAccessor.cpp");
    build.file("src/Common/MemoryCommon.cpp");
    build.file("src/Windows/WindowsDolphinProcess.cpp");
    build.file("src/Linux/LinuxDolphinProcess.cpp");
    build.compile("impl_dme.a");

    let bindings = bindgen::Builder::default()
        .header("src/bindings.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings");
}
