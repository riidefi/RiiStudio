extern crate bindgen;
extern crate cc;

use std::env;
use std::path::PathBuf;

fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);

    #[cfg(unix)]
    build.flag("-std=c++17");
    let compiler = build.get_compiler();
    if !compiler.is_like_gnu() && !compiler.is_like_clang() {
        #[cfg(not(debug_assertions))]
        build.flag("-MT");
    }

    #[cfg(target_arch = "x86_64")]
    build.flag("-DARCH_X64=1");

    build.include(".").include("src");
    build.file("src/bindings.cpp");
    build.compile("impl_avir.a");

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
