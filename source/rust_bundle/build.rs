use std::fs;
use std::path::Path;

#[cfg(windows)]
use std::process::Command;


fn copy_dir<P: AsRef<Path>, Q: AsRef<Path>>(src: P, dst: Q) -> std::io::Result<()> {
    for entry in fs::read_dir(src)? {
        let entry = entry?;
        let path = entry.path();
        if entry.file_type()?.is_dir() {
            copy_dir(&path, &dst.as_ref().join(path.file_name().unwrap()))?;
        } else {
            fs::copy(&path, &dst.as_ref().join(path.file_name().unwrap()))?;
        }
    }
    Ok(())
}

fn link_lib(directory: &str, lib: &str) {
    println!("cargo:rustc-link-search=native={}", directory);
    println!("cargo:rustc-link-lib=static={}", lib);
}

fn main() {
    #[cfg(windows)]
    let build_dir = "..\\..\\out\\build\\Clang-x64-DIST\\";
    #[cfg(unix)]
    let build_dir = "../../build/";
    #[cfg(windows)]
    let source_dir = "..\\..\\source\\";
    #[cfg(unix)]
    let source_dir = "../../source/";

    let out_dir = format!("{}source/", build_dir);
    let deps_dir = format!("{}_deps/", build_dir);

    #[cfg(windows)]
    let cargo_dir = format!("{}cargo\\build\\x86_64-pc-windows-msvc\\release\\", build_dir);
    #[cfg(unix)]
    let cargo_dir = format!("{}cargo/build/aarch64-apple-darwin/debug/", build_dir);

    #[cfg(unix)]
    {
        println!("cargo:rustc-link-search=native={}", "/opt/homebrew/Cellar/freetype/2.13.1/lib/");
        println!("cargo:rustc-link-lib=static={}", "freetype");
        println!("cargo:rustc-link-search=native={}", "/opt/homebrew/Cellar/glfw/3.3.8/lib/");
        println!("cargo:rustc-link-lib=static={}", "glfw");
        println!("cargo:rustc-link-search=native={}", "/opt/homebrew/Cellar/assimp/5.2.5/lib/");
        println!("cargo:rustc-link-lib=static={}", "assimp");
        println!("cargo:rustc-link-lib=static={}", "dl");
        println!("cargo:rustc-link-lib=static={}", "c++");
        println!("cargo:rustc-link-lib=framework={}", "CoreFoundation");
    }
 

    let libs = vec![
        (format!("{}{}", out_dir, "frontend"), "lib_frontend"),
        (format!("{}{}", out_dir, "core"), "core"),
        (format!("{}{}", out_dir, "LibBadUIFramework"), "LibBadUIFramework"),
        (format!("{}{}", out_dir, "librii"), "librii"),
        (format!("{}{}", out_dir, "rsmeshopt"), "rsmeshopt"),
        (format!("{}{}", out_dir, "oishii"), "oishii"),
        (format!("{}{}", out_dir, "plate"), "plate"),
        (format!("{}{}", out_dir, "plugins"), "plugins"),
        (format!("{}{}", out_dir, "rsl"), "rsl"),
        (format!("{}{}", out_dir, "updater"), "updater"),
        (format!("{}{}", out_dir, "vendor"), "vendor"),

        #[cfg(windows)]
        (format!("{}{}", source_dir, "vendor"), "freetype"),
        #[cfg(windows)]
        (format!("{}{}", source_dir, "plate\\vendor\\glfw\\lib-vc2017"), "glfw3dll"),
        #[cfg(windows)]
        (format!("{}{}", source_dir, "vendor\\assimp"), "assimp-vc141-mt"),

        #[cfg(unix)]
        (format!("{}{}", deps_dir, "fmt-build"), "fmtd"),
        #[cfg(windows)]
        (format!("{}{}", deps_dir, "fmt-build"), "fmt"),
        (format!("{}{}", deps_dir, "meshoptimizer-build"), "meshoptimizer"),
    ];

    for (dir, lib) in libs {
        link_lib(&dir, &lib);
    }

    // gendef.exe llvm_sighandler.dll
    // llvm-dlltool.exe -D llvm_sighandler.dll -d llvm_sighandler.def -l llvm_sighandler.lib -m i386:x86-64

    #[cfg(windows)]
    let dlls = vec!["gctex.dll", "riistudio_rs.dll", "llvm_sighandler.dll", "avir_rs.dll", "dolphin_memory_engine_rs.dll"];

    #[cfg(windows)]
    for dll in &dlls {
        // Run gendef.exe
        let output = Command::new(format!("{}\\gendef.exe", cargo_dir))
            .arg(format!("{}\\{}", &cargo_dir, dll))
            .current_dir(&cargo_dir)
            .output()
            .expect("Failed to execute gendef.exe");

        // Check the output for any error
        if !output.status.success() {
            panic!("gendef.exe failed: {}", String::from_utf8_lossy(&output.stderr));
        }

        // Prepare .def and .lib file names
        let dll_path = Path::new(dll);
        let dll_stem = dll_path.file_stem().unwrap().to_str().unwrap();
        let def_file = format!("{}\\{}.def", cargo_dir, dll_stem);
        let lib_file = format!("{}\\{}.lib", cargo_dir, dll_stem);

        // Run llvm-dlltool.exe
        let output = Command::new(format!("{}\\llvm-dlltool.exe", cargo_dir))
            .args(&["-D", dll, "-d", &def_file, "-l", &lib_file, "-m", "i386:x86-64"])
            .output()
            .expect("Failed to execute llvm-dlltool.exe");

        if !output.status.success() {
            panic!("llvm-dlltool.exe failed: {}", String::from_utf8_lossy(&output.stderr));
        }
    }

    println!("cargo:rustc-link-search=native={}", cargo_dir);
    println!("cargo:rustc-link-lib=dylib=gctex");
    println!("cargo:rustc-link-lib=dylib=riistudio_rs");
    println!("cargo:rustc-link-lib=dylib=llvm_sighandler");
    println!("cargo:rustc-link-lib=dylib=avir_rs");
    println!("cargo:rustc-link-lib=dylib=c_discord_rich_presence");
    println!("cargo:rustc-link-lib=dylib=dolphin_memory_engine_rs");

    #[cfg(windows)]
    {
        println!("cargo:rustc-link-lib=static=opengl32");
        println!("cargo:rustc-link-lib=static=Crypt32");
        println!("cargo:rustc-link-lib=static=Secur32");
        println!("cargo:rustc-link-lib=static=Ncrypt");
        println!("cargo:rustc-link-lib=static=Shell32");
    }

    // Get the directory that the build script is running in.

    // The source directories for fonts, lang, and icon.png.
    let font_src = format!("{}{}", source_dir, "../fonts");
    let lang_src = format!("{}{}", source_dir, "../lang");
    let icon_src = format!("{}{}", source_dir, "frontend/rc/icon.png");

    let out_dir = "target/debug";

    // The destination directories for fonts, lang, and icon.png.
    let font_dest = Path::new(&out_dir).join("fonts");
    let lang_dest = Path::new(&out_dir).join("lang");
    let icon_dest = Path::new(&out_dir).join("icon.png");

    // Copy the directories and files.
    fs::create_dir_all(&font_dest).unwrap();
    fs::create_dir_all(&lang_dest).unwrap();
    fs::copy(icon_src, &icon_dest).unwrap();
    copy_dir(font_src, &font_dest).unwrap();
    copy_dir(lang_src, &lang_dest).unwrap();
}
