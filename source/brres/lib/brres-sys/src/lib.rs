pub mod ffi;

/// Returns a formatted string representing the version, build profile, and target of the `gctex` crate.
///
/// This function makes use of several compile-time environment variables provided by Cargo
/// to determine the version, build profile (debug or release), and target platform.
///
/// # Returns
/// A `String` that contains:
/// - The version of the `brres-sys` crate, as specified in its `Cargo.toml`.
/// - The build profile, which can either be "debug" or "release".
/// - The target platform for which the crate was compiled.
///
/// # Example
/// ```
/// let version_string = brres_sys::get_version();
/// println!("{}", version_string);
/// // Outputs: "riidefi/brres-sys: Version: 0.1.0, Profile: release, Target: x86_64-unknown-linux-gnu"
/// ```
///
pub fn get_version() -> String {
    let pkg_version = env!("CARGO_PKG_VERSION");
    let profile = if cfg!(debug_assertions) {
        "debug"
    } else {
        "release"
    };
    let target = env!("TARGET");
    format!(
        "riidefi/brres-sys: Version: {}, Profile: {}, Target: {}",
        pkg_version, profile, target
    )
}

#[cfg(test)]
mod tests {
    use crate::ffi::*;

    #[test]
    fn test_from_bytes() {
        let buffer = vec![
            0x52, 0x42, 0x55, 0x46, // magic "RBUF"
            0x00, 0x00, 0x00, 0x64, // version 100
            0x00, 0x00, 0x00, 0x01, // num_buffers 1
            0x00, 0x00, 0x00, 0x20, // file_size 32
            0x00, 0x00, 0x00, 0x20, // buffer_offset 32
            0x00, 0x00, 0x00, 0x10, // buffer_size 16
            // Buffer data (16 bytes)
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
            0x0E, 0x0F,
        ];
        let result = CBrresWrapper::from_bytes(&buffer);
        assert!(!result.is_ok());
    }

    #[test]
    fn test_write_bytes() {
        let json = r#"{}"#; // Empty archive
        let buffer = vec![
            0x52, 0x42, 0x55, 0x46, // magic "RBUF"
            0x00, 0x00, 0x00, 0x64, // version 100
            0x00, 0x00, 0x00, 0x01, // num_buffers 1
            0x00, 0x00, 0x00, 0x20, // file_size 32
            0x00, 0x00, 0x00, 0x20, // buffer_offset 32
            0x00, 0x00, 0x00, 0x10, // buffer_size 16
            // Buffer data (16 bytes)
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
            0x0E, 0x0F,
        ];
        let result = CBrresWrapper::write_bytes(json, &buffer);
        assert!(result.is_ok());
    }
}

//--------------------------------------------------------
//
// C BINDINGS BEGIN
//
//--------------------------------------------------------
#[cfg(feature = "c_api")]
pub mod c_api {
    use crate::*;

    #[no_mangle]
    pub extern "C" fn brres_get_version_unstable_api(buffer: *mut u8, length: u32) -> i32 {
        let version_info = get_version();
        let string_length = version_info.len();

        if string_length > length as usize {
            return -1;
        }

        let buffer_slice = unsafe {
            assert!(!buffer.is_null());
            std::slice::from_raw_parts_mut(buffer, length as usize)
        };

        for (i, byte) in version_info.as_bytes().iter().enumerate() {
            buffer_slice[i] = *byte;
        }

        string_length as i32
    }
}
