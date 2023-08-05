fn main() {
    let target = std::env::var("TARGET").unwrap();
    if target.contains("msvc") {
        println!("cargo:rustc-link-lib=oldnames");
    }
}
