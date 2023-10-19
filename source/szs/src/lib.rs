use core::ffi::c_char;
use core::slice;

#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

/// Checks if the given byte slice is SZS (YAZ0) compressed.
///
/// # Arguments
///
/// * `src`: A byte slice that we want to check for compression signature.
///
/// # Returns
///
/// * `true` if the byte slice is potentially a valid SZS (YAZ0) stream.
/// * `false` otherwise.
///
/// # Examples
///
/// ```
/// let data: &[u8] = b"Yaz0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00compresseddata";
/// assert_eq!(szs::is_compressed(data), true);
///
/// let non_compressed_data: &[u8] = b"noncompressed";
/// assert_eq!(szs::is_compressed(non_compressed_data), false);
/// ```
pub fn is_compressed(src: &[u8]) -> bool {
    if src.len() < 8 {
        return false;
    }
    src.starts_with(b"Yaz0")
}

/// Retrieves the decoded size of an SZS (YAZ0) compressed byte slice.
///
/// This function reads the 4 bytes following the "Yaz0" signature
/// in the byte slice to determine the uncompressed data size.
///
/// # Arguments
///
/// * `src`: A byte slice that potentially represents an SZS (YAZ0) compressed stream.
///
/// # Returns
///
/// * The decoded size of the byte slice as a 32-bit unsigned integer.
///   Returns 0 if the slice is not a valid SZS (YAZ0) stream or is too short.
///
/// # Examples
///
/// ```
/// let data: &[u8] = b"Yaz0\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x00compresseddata";
/// assert_eq!(szs::decoded_size(data), 32);
///
/// let non_compressed_data: &[u8] = b"noncompressed";
/// assert_eq!(szs::decoded_size(non_compressed_data), 0);
/// ```
pub fn decoded_size(src: &[u8]) -> u32 {
    if src.len() < 8 || !src.starts_with(b"Yaz0") {
        return 0;
    }
    ((src[4] as u32) << 24) | ((src[5] as u32) << 16) | ((src[6] as u32) << 8) | (src[7] as u32)
}

/// Retrieves the maximum potential encoded size for a given uncompressed data length using SZS (YAZ0) compression.
///
/// This function is useful for buffer allocation to ensure the buffer is
/// large enough to hold the worst-case scenario of SZS (YAZ0) compressed data.
///
/// # Arguments
///
/// * `len`: The length of the uncompressed data as a 32-bit unsigned integer.
///
/// # Returns
///
/// * The maximum potential encoded size for the given uncompressed data length.
///
/// # Examples
///
/// ```
/// let uncompressed_length = 64;
/// let max_encoded_size = szs::encoded_upper_bound(uncompressed_length);
/// // Allocate a buffer based on the worst-case encoded size
/// let mut buffer: Vec<u8> = Vec::with_capacity(max_encoded_size as usize);
/// ```
pub fn encoded_upper_bound(len: u32) -> u32 {
    unsafe { bindings::impl_rii_worst_encoding_size(len) }
}

#[repr(u32)]
pub enum EncodeAlgo {
    WorstCaseEncoding = 0,
    Nintendo,
    MkwSp,
    CTGP,
    Haroohie,
    CTLib,
    LibYaz0,
    Mk8,
}

pub enum EncodeAlgoError {
    Error(String),
}

/// Performs in-place encoding of the source slice using the specified encoding algorithm.
///
/// The function calls into a potentially unsafe C binding to perform the encoding,
/// and it may return an error if the encoding process encounters any issues.
///
/// # Arguments
///
/// * `dst`: A mutable byte slice where the encoded data will be written.
///          This slice should be pre-allocated with sufficient capacity.
///
/// * `src`: A byte slice that contains the data to be encoded.
///
/// * `algo`: The encoding algorithm to be used.
///
/// # Returns
///
/// * `Ok(u32)`: The length of the encoded data written to `dst`.
///
/// * `Err(EncodeAlgoError)`: An error encountered during the encoding process.
///
/// # Safety
///
/// This function uses unsafe code to call C bindings. Ensure that `dst` has enough capacity
/// to hold the encoded data to prevent memory corruption or undefined behavior.
///
/// # Examples
///
/// ```
/// let mut dst: [u8; 512] = [0; 512];
/// let src = b"some data to encode";
/// let algorithm = szs::EncodeAlgo::Mk8; // Mario Kart 8
///
/// match szs::encode_inplace(&mut dst, src, algorithm) {
///     Ok(encoded_len) => println!("Encoded {} bytes", encoded_len),
///     Err(szs::EncodeAlgoError::Error(err)) => println!("Error: {}", err),
/// }
/// ```
pub fn encode_inplace(
    dst: &mut [u8],
    src: &[u8],
    algo: EncodeAlgo,
) -> Result<u32, EncodeAlgoError> {
    let mut used_len: u32 = 0;

    let result = unsafe {
        bindings::impl_rii_encodeAlgo(
            dst.as_mut_ptr() as *mut _,
            dst.len() as u32,
            src.as_ptr() as *const _,
            src.len() as u32,
            &mut used_len,
            algo as u32,
        )
    };

    if result.is_null() {
        Ok(used_len)
    } else {
        let error_msg = unsafe {
            std::ffi::CStr::from_ptr(result)
                .to_string_lossy()
                .into_owned()
        };
        Err(EncodeAlgoError::Error(error_msg))
    }
}

/// Encodes the source slice using the specified encoding algorithm and returns the encoded data.
///
/// This function first calculates the maximum potential size of the encoded data,
/// allocates a buffer of that size, and then encodes the data in-place.
/// After encoding, the buffer is truncated to the actual size of the encoded data.
///
/// # Arguments
///
/// * `src`: A byte slice that contains the data to be encoded.
///
/// * `algo`: The encoding algorithm to be used.
///
/// # Returns
///
/// * `Ok(Vec<u8>)`: A `Vec<u8>` containing the encoded data.
///
/// * `Err(EncodeAlgoError)`: An error encountered during the encoding process.
///
/// # Examples
///
/// ```
/// let src = b"some data to encode";
/// let algorithm = szs::EncodeAlgo::Mk8; // Mario Kart 8
///
/// match szs::encode(src, algorithm) {
///     Ok(encoded_data) => println!("Encoded data length: {}", encoded_data.len()),
///     Err(szs::EncodeAlgoError::Error(err)) => println!("Error: {}", err),
/// }
/// ```
pub fn encode(src: &[u8], algo: EncodeAlgo) -> Result<Vec<u8>, EncodeAlgoError> {
    // Calculate the upper bound for encoding.
    let max_len = encoded_upper_bound(src.len() as u32);

    // Allocate a buffer based on the calculated upper bound.
    let mut dst: Vec<u8> = vec![0; max_len as usize];

    match encode_inplace(&mut dst, src, algo) {
        Ok(encoded_len) => {
            // Shrink the dst to the actual size.
            dst.truncate(encoded_len as usize);
            Ok(dst)
        }
        Err(err) => Err(err),
    }
}

/// Decodes the source slice in-place as a SZS (YAZ0) compressed stream, writing the decoded data to the destination slice.
///
/// The function calls into a potentially unsafe C binding to perform the decoding,
/// and may return an error if the decoding process encounters any issues.
///
/// # Arguments
///
/// * `dst`: A mutable byte slice where the decoded data will be written.
///          Ensure this slice has enough capacity to hold the decoded data.
///
/// * `src`: A byte slice that contains the data to be decoded.
///
/// # Returns
///
/// * `Ok(())`: Indicates successful decoding.
///
/// * `Err(EncodeAlgoError)`: An error encountered during the decoding process.
///
/// # Safety
///
/// This function uses unsafe code to call C bindings. Ensure that `dst` has enough capacity
/// to hold the decoded data to prevent memory corruption or undefined behavior.
///
/// # Examples
///
/// ```
/// let mut dst: [u8; 512] = [0; 512];
/// let src = b"some encoded data";
///
/// match szs::decode_inplace(&mut dst, src) {
///     Ok(_) => println!("Successfully decoded"),
///     Err(szs::EncodeAlgoError::Error(err)) => println!("Decoding error: {}", err),
/// }
/// ```
pub fn decode_inplace(dst: &mut [u8], src: &[u8]) -> Result<(), EncodeAlgoError> {
    let result = unsafe {
        bindings::impl_riiszs_decode(
            dst.as_mut_ptr() as *mut _,
            dst.len() as u32,
            src.as_ptr() as *const _,
            src.len() as u32,
        )
    };

    if result.is_null() {
        Ok(())
    } else {
        let error_msg = unsafe {
            std::ffi::CStr::from_ptr(result)
                .to_string_lossy()
                .into_owned()
        };
        Err(EncodeAlgoError::Error(error_msg))
    }
}

/// Decodes the source slice in-place as a SZS (YAZ0) compressed stream and returns the decoded data.
///
/// This function first calculates the required size of the decoded data,
/// allocates a buffer of that size, and then decodes the data in-place.
///
/// # Arguments
///
/// * `src`: A byte slice that contains the data to be decoded.
///
/// # Returns
///
/// * `Ok(Vec<u8>)`: A `Vec<u8>` containing the decoded data.
///
/// * `Err(EncodeAlgoError)`: An error encountered during the decoding process.
///
/// # Examples
///
/// ```
/// let src = b"some encoded data";
///
/// match szs::decode(src) {
///     Ok(decoded_data) => println!("Decoded data length: {}", decoded_data.len()),
///     Err(szs::EncodeAlgoError::Error(err)) => println!("Decoding error: {}", err),
/// }
/// ```
pub fn decode(src: &[u8]) -> Result<Vec<u8>, EncodeAlgoError> {
    // Calculate the required size for decoding.
    let req_len = decoded_size(src);

    // Allocate a buffer based on the calculated required size.
    let mut dst: Vec<u8> = vec![0; req_len as usize];

    match decode_inplace(&mut dst, src) {
        Ok(_) => Ok(dst),
        Err(err) => Err(err),
    }
}

/// Calculates the upper bound of the length for a deinterlaced SZP (YAY0) stream given an interlaced SZS (YAZ0) stream.
///
/// Given a length, this function estimates the maximum potential size of the deinterlaced data,
/// accounting for any necessary padding.
///
/// # Arguments
///
/// * `len`: The length of the data to be deinterlaced.
///
/// # Returns
///
/// * The maximum potential length of the deinterlaced data.
///
/// # Examples
///
/// ```
/// let original_length = 100;
/// let upper_bound = szs::deinterlaced_upper_bound(original_length);
/// assert!(upper_bound >= original_length);
/// ```
pub fn deinterlaced_upper_bound(len: u32) -> u32 {
    // u8stream padding when converted to u32stream
    len + 3
}

/// Converts an interlaced SZS (YAZ0) stream into a deinterlaced SZP (YAY0) stream.
///
/// The function performs the deinterlacing by calling a potentially unsafe C binding.
/// It may return an error if the process encounters any issues.
///
/// # Arguments
///
/// * `dst`: A mutable byte slice where the deinterlaced data will be written.
///          Ensure this slice has enough capacity to hold the deinterlaced data.
///
/// * `src`: A byte slice that contains the interlaced data to be converted.
///
/// # Returns
///
/// * `Ok(u32)`: The length of the deinterlaced data written to `dst`.
///
/// * `Err(EncodeAlgoError)`: An error encountered during the deinterlacing process.
///
/// # Safety
///
/// This function uses unsafe code to call C bindings. Ensure that `dst` has enough capacity
/// to hold the deinterlaced data to prevent memory corruption or undefined behavior.
///
/// # Examples
///
/// ```
/// let mut dst: [u8; 512] = [0; 512];
/// let src = b"some interlaced data";
///
/// match szs::deinterlace_into(&mut dst, src) {
///     Ok(deinterlaced_len) => println!("Deinterlaced {} bytes", deinterlaced_len),
///     Err(szs::EncodeAlgoError::Error(err)) => println!("Deinterlacing error: {}", err),
/// }
/// ```
pub fn deinterlace_into(dst: &mut [u8], src: &[u8]) -> Result<u32, EncodeAlgoError> {
    let mut used_len: u32 = 0;

    let result = unsafe {
        bindings::impl_rii_deinterlace(
            dst.as_mut_ptr() as *mut _,
            dst.len() as u32,
            src.as_ptr() as *const _,
            src.len() as u32,
            &mut used_len,
        )
    };

    if result.is_null() {
        Ok(used_len)
    } else {
        let error_msg = unsafe {
            std::ffi::CStr::from_ptr(result)
                .to_string_lossy()
                .into_owned()
        };
        Err(EncodeAlgoError::Error(error_msg))
    }
}

/// Converts an interlaced SZS (YAZ0) stream into a deinterlaced SZP (YAY0) stream and returns it.
///
/// This function first calculates the upper bound of the length of the deinterlaced data,
/// allocates a buffer of that size, and then performs the deinterlacing into that buffer.
///
/// # Arguments
///
/// * `src`: A byte slice that contains the interlaced data to be converted.
///
/// # Returns
///
/// * `Ok(Vec<u8>)`: A `Vec<u8>` containing the deinterlaced data.
///
/// * `Err(EncodeAlgoError)`: An error encountered during the deinterlacing process.
///
/// # Examples
///
/// ```
/// let src = b"some interlaced data";
///
/// match szs::deinterlace(src) {
///     Ok(deinterlaced_data) => println!("Deinterlaced data length: {}", deinterlaced_data.len()),
///     Err(szs::EncodeAlgoError::Error(err)) => println!("Deinterlacing error: {}", err),
/// }
/// ```
pub fn deinterlace(src: &[u8]) -> Result<Vec<u8>, EncodeAlgoError> {
    // Calculate the upper bound for conversion.
    let max_len = deinterlaced_upper_bound(src.len() as u32);

    // Allocate a buffer based on the calculated upper bound.
    let mut dst: Vec<u8> = vec![0; max_len as usize];

    match deinterlace_into(&mut dst, src) {
        Ok(encoded_len) => {
            // Shrink the dst to the actual size.
            dst.truncate(encoded_len as usize);
            Ok(dst)
        }
        Err(err) => Err(err),
    }
}

//--------------------------------------------------------
//
// C BINDINGS BEGIN
//
//--------------------------------------------------------
#[allow(clippy::missing_safety_doc)]
pub mod c_api {
    use crate::*;
    #[no_mangle]
    pub unsafe extern "C" fn szs_get_version_unstable_api(buffer: *mut u8, length: u32) -> i32 {
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

    #[no_mangle]
    pub unsafe extern "C" fn riiszs_encoded_upper_bound(len: u32) -> u32 {
        encoded_upper_bound(len)
    }

    #[no_mangle]
    pub unsafe extern "C" fn riiszs_encode_algo_fast(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        result: *mut u32,
        algo: EncodeAlgo, // u32
    ) -> *const c_char {
        let dst_slice = unsafe { std::slice::from_raw_parts_mut(dst, dst_len as usize) };
        let src_slice = unsafe { std::slice::from_raw_parts(src, src_len as usize) };

        match encode_inplace(dst_slice, src_slice, algo) {
            Ok(used_len) => {
                unsafe {
                    *result = used_len;
                }
                std::ptr::null()
            }
            Err(EncodeAlgoError::Error(msg)) => {
                let c_string = std::ffi::CString::new(msg).unwrap();
                // Leak the CString into a raw pointer, so we don't deallocate it
                c_string.into_raw()
            }
        }
    }

    #[no_mangle]
    pub unsafe extern "C" fn riiszs_decode(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
    ) -> *const c_char {
        let dst_slice = unsafe { std::slice::from_raw_parts_mut(dst, dst_len as usize) };
        let src_slice = unsafe { std::slice::from_raw_parts(src, src_len as usize) };

        match decode_inplace(dst_slice, src_slice) {
            Ok(()) => std::ptr::null(),
            Err(EncodeAlgoError::Error(msg)) => {
                let c_string = std::ffi::CString::new(msg).unwrap();
                // Leak the CString into a raw pointer, so we don't deallocate it
                c_string.into_raw()
            }
        }
    }

    // For freeing the error message from C side when done
    #[no_mangle]
    pub unsafe extern "C" fn riiszs_free_error_message(err_ptr: *mut c_char) {
        unsafe {
            if !err_ptr.is_null() {
                let _ = std::ffi::CString::from_raw(err_ptr);
            }
        }
    }

    #[no_mangle]
    pub unsafe extern "C" fn riiszs_is_compressed(src: *const u8, len: u32) -> bool {
        let data = unsafe { std::slice::from_raw_parts(src, len as usize) };
        is_compressed(data)
    }

    #[no_mangle]
    pub unsafe extern "C" fn riiszs_decoded_size(src: *const u8, len: u32) -> u32 {
        let data = unsafe { std::slice::from_raw_parts(src, len as usize) };
        decoded_size(data)
    }

    #[no_mangle]
    pub unsafe extern "C" fn riiszs_deinterlaced_upper_bound(len: u32) -> u32 {
        deinterlaced_upper_bound(len)
    }

    #[no_mangle]
    pub unsafe extern "C" fn riiszs_deinterlace_into(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
        result: *mut u32,
    ) -> *const c_char {
        let dst_slice = unsafe { std::slice::from_raw_parts_mut(dst, dst_len as usize) };
        let src_slice = unsafe { std::slice::from_raw_parts(src, src_len as usize) };

        match deinterlace_into(dst_slice, src_slice) {
            Ok(used_len) => {
                unsafe {
                    *result = used_len;
                }
                std::ptr::null()
            }
            Err(EncodeAlgoError::Error(msg)) => {
                let c_string = std::ffi::CString::new(msg).unwrap();
                // Leak the CString into a raw pointer, so we don't deallocate it
                c_string.into_raw()
            }
        }
    }
}
