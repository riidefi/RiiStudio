extern crate cc;
extern crate bindgen;

use std::env;
use std::path::PathBuf;
use glob::glob;

fn main() {
    let mut build = cc::Build::new();
    build.cpp(true);
    build.include(".");
    build.file("src/bindings.cpp");
    for entry in glob("llvm/**/*.cpp").expect("Failed to read glob pattern") {
        match entry {
            Ok(path) => {
                println!("Adding file: {:?}", path);
                build.file(path);
            },
            Err(e) => println!("{:?}", e),
        }
    }
    build.compile("libwrite_to_file.a");

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
