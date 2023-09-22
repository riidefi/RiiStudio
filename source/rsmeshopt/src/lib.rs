use core::slice;

pub mod librii {
    #[allow(non_upper_case_globals)]
    #[allow(non_camel_case_types)]
    #[allow(non_snake_case)]
    pub mod bindings {
        include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
    }

    #[inline]
    pub fn stripify_bound(x: usize) -> usize {
        (x / 3) * 5
    }

    pub fn stripify(algo: u32, indices: &[u32], positions: &[f32], restart: u32) -> Vec<u32> {
        let mut dst = Vec::new();
        dst.resize(stripify_bound(indices.len()), 0);
        let result_size = unsafe {
            bindings::c_stripify(
                dst.as_mut_ptr(),
                algo,
                indices.as_ptr(),
                indices.len() as u32,
                positions.as_ptr(),
                (positions.len() / 3) as u32, // Assuming vec3s are three f32s
                restart,
            )
        };
        dst.resize(result_size as usize, 0);
        dst
    }

    pub fn make_fans(indices: &[u32], restart: u32, min_len: u32, max_runs: u32) -> Vec<u32> {
        let mut dst = Vec::new();
        dst.resize(stripify_bound(indices.len()), 0);
        let result_size = unsafe {
            bindings::c_makefans(
                dst.as_mut_ptr(),
                indices.as_ptr(),
                indices.len() as u32,
                restart,
                min_len,
                max_runs,
            )
        };
        dst.resize(result_size as usize, 0);
        dst
    }
}

#[no_mangle]
pub unsafe extern "C" fn rii_stripify(
    dst: *mut u32,
    algo: u32,
    indices: *const u32,
    num_indices: u32,
    positions: *const f32,
    num_positions: u32,
    restart: u32,
) -> u32 {
    let indices_slice = std::slice::from_raw_parts(indices, num_indices as usize);
    let positions_slice = std::slice::from_raw_parts(positions, (num_positions * 3) as usize); // Assuming vec3s are three f32s
    let result_vec = librii::stripify(algo, indices_slice, positions_slice, restart);

    // Copy the result to the destination
    let dst_slice = std::slice::from_raw_parts_mut(dst, result_vec.len());
    dst_slice.copy_from_slice(&result_vec);

    result_vec.len() as u32
}

#[no_mangle]
pub unsafe extern "C" fn rii_makefans(
    dst: *mut u32,
    indices: *const u32,
    num_indices: u32,
    restart: u32,
    min_len: u32,
    max_runs: u32,
) -> u32 {
    let indices_slice = std::slice::from_raw_parts(indices, num_indices as usize);
    let result_vec = librii::make_fans(indices_slice, restart, min_len, max_runs);

    // Copy the result to the destination
    let dst_slice = std::slice::from_raw_parts_mut(dst, result_vec.len());
    dst_slice.copy_from_slice(&result_vec);

    result_vec.len() as u32
}

#[no_mangle]
pub extern "C" fn rsmeshopt_get_version_unstable_api(buffer: *mut u8, length: u32) -> i32 {
    let pkg_version = env!("CARGO_PKG_VERSION");
    let profile = if cfg!(debug_assertions) {
        "debug"
    } else {
        "release"
    };
    let target = env!("TARGET");
    let version_info = format!(
        "Version: {}, Profile: {}, Target: {}",
        pkg_version, profile, target
    );

    let string_length = version_info.len();

    if string_length > length as usize {
        return -1;
    }

    let buffer_slice = unsafe {
        assert!(!buffer.is_null());
        slice::from_raw_parts_mut(buffer, length as usize)
    };

    for (i, byte) in version_info.as_bytes().iter().enumerate() {
        buffer_slice[i] = *byte;
    }

    string_length as i32
}
