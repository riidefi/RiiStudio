use core::ffi::c_char;
use core::slice;
use std::convert::TryInto;

#[allow(non_upper_case_globals)]
#[allow(non_camel_case_types)]
#[allow(non_snake_case)]
#[allow(dead_code)]
mod bindings {
    #[cfg(feature = "run_bindgen")]
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

    // For now, just manually embed these...
    #[cfg(not(feature = "run_bindgen"))]
    extern "C" {
        pub fn impl_rii_is_szs_compressed(src: *const ::std::os::raw::c_void, len: u32) -> u32;
        pub fn impl_rii_get_szs_expand_size(src: *const ::std::os::raw::c_void, len: u32) -> u32;
        pub fn impl_riiszs_decode(
            buf: *mut ::std::os::raw::c_void,
            len: u32,
            src: *const ::std::os::raw::c_void,
            src_len: u32,
        ) -> *const ::std::os::raw::c_char;
        pub fn impl_rii_worst_encoding_size(len: u32) -> u32;
        pub fn impl_rii_encodeAlgo(
            dst: *mut ::std::os::raw::c_void,
            dst_len: u32,
            src: *const ::std::os::raw::c_void,
            src_len: u32,
            used_len: *mut u32,
            algo: u32,
        ) -> *const ::std::os::raw::c_char;
        pub fn impl_rii_deinterlace(
            dst: *mut ::std::os::raw::c_void,
            dst_len: u32,
            src: *const ::std::os::raw::c_void,
            src_len: u32,
            used_len: *mut u32,
        ) -> *const ::std::os::raw::c_char;
    }
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

/// Retrieves the decoded size of an SZS (YAZ0) or SZP (YAY0) compressed byte slice.
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
    if src.len() < 8 || !(src.starts_with(b"Yaz0") || src.starts_with(b"Yay0")) {
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

/// Algorithms available for encoding.
#[repr(u32)]
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub enum EncodeAlgo {
    /// Uses the `WorstCaseEncoding` algorithm.
    ///
    /// Use Case: `INSTANT` preset.
    /// Description: This is literally the worst possible compression output, resulting in files genuinely larger than their decompressed form.
    ///
    /// Speed: A+
    /// Compression Rate: F
    WorstCaseEncoding = 0,

    /// Uses the `MKW` algorithm.
    ///
    /// Use Case: Matching decompilation projects.
    /// Description: This is the Mario Kart Wii compression algorithm reverse-engineered. In practice it's a Boyer-moore-horspool search with a second opinion mechanism.
    ///
    /// Speed: F
    /// Compression Rate: A
    MKW,

    /// Uses the `MkwSp` algorithm.
    ///
    /// Description: MKW-SP.
    ///
    /// Speed: D
    /// Compression Rate: B+
    MkwSp,

    /// Uses the `CTGP` algorithm.
    ///
    /// Use Case: CTGP work (Reverse engineered. 1:1 matching).
    CTGP,

    /// Uses the `Haroohie` algorithm.
    ///
    ///
    /// Speed: B+
    /// Compression Rate: B+
    Haroohie,

    /// Uses the `CTLib` algorithm.
    ///
    /// Use Case: `MEDIUM` preset.
    ///
    /// Speed: A-
    /// Compression Rate: B+
    CTLib,

    /// Uses the `LibYaz0` algorithm.
    ///
    /// Use Case: `ULTRA` preset.
    ///
    /// Speed: C
    /// Compression Rate: A+
    LibYaz0,

    /// Uses the `MK8` algorithm.
    ///
    /// Use Case: General `FAST` preset.
    /// Description: This is the Mario Kart 8 compression algorithm reverse-engineered. In practice it's a sliding Monte Carlo hash table.
    ///
    /// Speed: A+
    /// Compression Rate: B+
    MK8,
}

impl EncodeAlgo {
    /// Alias for the `MKW` algorithm.
    #[allow(non_upper_case_globals)]
    pub const Nintendo: EncodeAlgo = EncodeAlgo::MKW;
}

#[derive(Debug, Clone)]
pub enum Error {
    Error(String),
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            Error::Error(str) => write!(f, "{}", str),
        }
    }
}

impl std::error::Error for Error {}

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
/// * `Err(Error)`: An error encountered during the encoding process.
///
/// # Safety
///
/// This function uses unsafe code to call C bindings. Ensure that `dst` has enough capacity
/// to hold the encoded data to prevent memory corruption or undefined behavior.
///
/// # Examples
///
/// ```
/// let src = b"some data to encode";
/// let algorithm = szs::EncodeAlgo::MK8; // Mario Kart 8
///
/// // Determine the upper bound for the encoded data size
/// let max_encoded_size = szs::encoded_upper_bound(src.len() as u32) as usize;
///
/// let mut dst: Vec<u8> = vec![0; max_encoded_size];
///
/// match szs::encode_into(&mut dst, src, algorithm) {
///     Ok(encoded_len) => {
///         println!("Encoded {} bytes", encoded_len);
///         dst.truncate(encoded_len as usize);  // Adjust the vector to the actual encoded length
///     },
///     Err(err) => println!("Error: {}", err),
/// }
/// ```
pub fn encode_into(dst: &mut [u8], src: &[u8], algo: EncodeAlgo) -> Result<u32, Error> {
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
        Err(Error::Error(error_msg))
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
/// * `Err(Error)`: An error encountered during the encoding process.
///
/// # Examples
///
/// ```
/// let src = b"some data to encode";
/// let algorithm = szs::EncodeAlgo::MK8; // Mario Kart 8
///
/// match szs::encode(src, algorithm) {
///     Ok(encoded_data) => println!("Encoded data length: {}", encoded_data.len()),
///     Err(err) => println!("Error: {}", err),
/// }
/// ```
pub fn encode(src: &[u8], algo: EncodeAlgo) -> Result<Vec<u8>, Error> {
    // Calculate the upper bound for encoding.
    let max_len = encoded_upper_bound(src.len() as u32);

    // Allocate a buffer based on the calculated upper bound.
    let mut dst: Vec<u8> = vec![0; max_len as usize];

    match encode_into(&mut dst, src, algo) {
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
/// * `Err(Error)`: An error encountered during the decoding process.
///
/// # Safety
///
/// This function uses unsafe code to call C bindings. Ensure that `dst` has enough capacity
/// to hold the decoded data to prevent memory corruption or undefined behavior.
///
/// # Examples
///
/// ```
/// let src = b"some encoded data";
///
/// // Get the expected size of the decoded data
/// let expected_size = szs::decoded_size(src) as usize;
///
/// // Allocate a buffer of the appropriate size
/// let mut dst = vec![0u8; expected_size];
///
/// match szs::decode_into(&mut dst, src) {
///     Ok(_) => println!("Successfully decoded"),
///     Err(err) => println!("Decoding error: {}", err),
/// }
/// ```
pub fn decode_into(dst: &mut [u8], src: &[u8]) -> Result<(), Error> {
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
        Err(Error::Error(error_msg))
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
/// * `Err(Error)`: An error encountered during the decoding process.
///
/// # Examples
///
/// ```
/// let src = b"some encoded data";
///
/// match szs::decode(src) {
///     Ok(decoded_data) => println!("Decoded data length: {}", decoded_data.len()),
///     Err(err) => println!("Decoding error: {}", err),
/// }
/// ```
pub fn decode(src: &[u8]) -> Result<Vec<u8>, Error> {
    // Calculate the required size for decoding.
    let req_len = decoded_size(src);

    // Allocate a buffer based on the calculated required size.
    let mut dst: Vec<u8> = vec![0; req_len as usize];

    match decode_into(&mut dst, src) {
        Ok(_) => Ok(dst),
        Err(err) => Err(err),
    }
}

/// Decompresses the Yay0 (SZP) compressed `src` byte slice into the `dst` byte slice.
///
/// # Arguments
/// * `dst`: Destination buffer where the uncompressed data will be written.
/// Ensure this buffer is large enough to hold the decompressed data.
/// * `src`: Source buffer containing the Yay0-compressed data.
///
/// # Returns
/// * `Ok(())` if the decompression was successful.
/// * `Err(&'static str)` if there was an error during decompression.
///
/// # Examples
/// ```
/// let src = b"some YAY0 (SZP) encoded data";
///
/// // Calculate the expected size of the decompressed data
/// let expected_size = szs::decoded_size(src) as usize;
///
/// // Allocate a buffer of the appropriate size
/// let mut uncompressed = vec![0u8; expected_size];
///
/// match szs::decode_yay0_into(&mut uncompressed, src) {
///     Ok(_) => println!("Decompression successful!"),
///     Err(err) => println!("Error: {}", err),
/// }
/// ```
pub fn decode_yay0_into(dst: &mut [u8], src: &[u8]) -> Result<(), Error> {
    let mut index = 0;

    if &src[index..index + 4] != "Yay0".as_bytes() {
        return Err(Error::Error(
            "Invalid Identifier. Expected magic.".to_string(),
        ));
    }
    index += 4;

    let uncompressed_size = u32::from_be_bytes(src[index..index + 4].try_into().unwrap());
    index += 4;
    let mut link_table_offset =
        u32::from_be_bytes(src[index..index + 4].try_into().unwrap()) as usize;
    index += 4;
    let mut byte_chunk_and_count_modifiers_offset =
        u32::from_be_bytes(src[index..index + 4].try_into().unwrap()) as usize;
    index += 4;

    if uncompressed_size as usize > dst.len() {
        return Err(Error::Error("Destination buffer too small.".to_string()));
    }

    let mut mask_bit_counter = 0;
    let mut current_offset_in_dest_buffer = 0;
    let mut current_mask = 0;

    while current_offset_in_dest_buffer < uncompressed_size as usize {
        if mask_bit_counter == 0 {
            current_mask = u32::from_be_bytes(src[index..index + 4].try_into().unwrap());
            index += 4;
            mask_bit_counter = 32;
        }

        if (current_mask & 0x8000_0000) != 0 {
            dst[current_offset_in_dest_buffer] = src[byte_chunk_and_count_modifiers_offset];
            byte_chunk_and_count_modifiers_offset += 1;
            current_offset_in_dest_buffer += 1;
        } else {
            let link = u16::from_be_bytes(
                src[link_table_offset..link_table_offset + 2]
                    .try_into()
                    .unwrap(),
            );
            link_table_offset += 2;

            let offset = current_offset_in_dest_buffer as isize - (link & 0x0fff) as isize;
            let mut count = (link >> 12) as usize;

            if count == 0 {
                let count_modifier = src[byte_chunk_and_count_modifiers_offset];
                count = count_modifier as usize + 18;
                byte_chunk_and_count_modifiers_offset += 1;
            } else {
                count += 2;
            }

            for i in 0..count {
                dst[current_offset_in_dest_buffer + i] = dst[offset as usize + i - 1];
            }
            current_offset_in_dest_buffer += count;
        }

        current_mask <<= 1;
        mask_bit_counter -= 1;
    }

    Ok(())
}
/// Decompresses the Yay0 (SZP) compressed `src` byte slice and returns the uncompressed data in a `Vec<u8>`.
///
/// # Arguments
/// * `src`: Source buffer containing the Yay0-compressed data.
///
/// # Returns
/// * `Ok(Vec<u8>)` containing the uncompressed data if the decompression was successful.
/// * `Err(&'static str)` if there was an error during decompression.
///
/// # Examples
/// ```
/// let compressed = b"some YAY0 (SZP) encoded data";
///
/// match szs::decode_yay0(compressed) {
///     Ok(uncompressed) => println!("Decompressed data size: {}", uncompressed.len()),
///     Err(err) => println!("Error: {}", err),
/// }
/// ```
pub fn decode_yay0(src: &[u8]) -> Result<Vec<u8>, Error> {
    let uncompressed_size = u32::from_be_bytes(src[4..8].try_into().unwrap()) as usize;
    let mut dst = vec![0u8; uncompressed_size];
    decode_yay0_into(&mut dst, src)?;
    Ok(dst)
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
/// * `Err(Error)`: An error encountered during the deinterlacing process.
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
///     Err(err) => println!("Deinterlacing error: {}", err),
/// }
/// ```
pub fn deinterlace_into(dst: &mut [u8], src: &[u8]) -> Result<u32, Error> {
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
        Err(Error::Error(error_msg))
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
/// * `Err(Error)`: An error encountered during the deinterlacing process.
///
/// # Examples
///
/// ```
/// let src = b"some interlaced data";
///
/// match szs::deinterlace(src) {
///     Ok(deinterlaced_data) => println!("Deinterlaced data length: {}", deinterlaced_data.len()),
///     Err(err) => println!("Deinterlacing error: {}", err),
/// }
/// ```
pub fn deinterlace(src: &[u8]) -> Result<Vec<u8>, Error> {
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

/// Calculates the upper bound of the length for an encoded YAY0 stream given uncompressed data.
///
/// This function combines the worst-case scenario sizes of both YAZ0 encoding and YAY0 deinterlacing.
///
/// # Arguments
///
/// * `len`: The length of the uncompressed data.
///
/// # Returns
///
/// * The maximum potential length of the YAY0 encoded data.
///
/// # Examples
///
/// ```
/// let original_length = 100;
/// let upper_bound = szs::encoded_yay0_upper_bound(original_length);
/// assert!(upper_bound >= original_length);
/// ```
pub fn encoded_yay0_upper_bound(len: u32) -> u32 {
    // Calculate YAZ0 encoding size, then adjust for YAY0 deinterlacing.
    let yaz0_upper_bound = encoded_upper_bound(len);
    deinterlaced_upper_bound(yaz0_upper_bound)
}

/// Encodes and deinterlaces the source slice into YAY0 format using the given encoding algorithm and writes the result into the destination slice.
///
/// # Arguments
///
/// * `dst`: A mutable byte slice where the YAY0 encoded data will be written.
/// * `src`: A byte slice containing the data to be encoded.
/// * `algo`: The encoding algorithm to be used.
///
/// # Returns
///
/// * `Ok(u32)`: The length of the YAY0 encoded data written to `dst`.
/// * `Err(Error)`: An error encountered during the encoding or deinterlacing process.
///
/// # Examples
///
/// ```
/// let src = b"some data to encode into YAY0";
/// let mut dst: Vec<u8> = Vec::new();
/// dst.resize_with(szs::encoded_yay0_upper_bound(src.len() as u32) as usize, Default::default);
/// let algorithm = szs::EncodeAlgo::Nintendo;
///
/// match szs::encode_yay0_into(&mut dst, src, algorithm) {
///     Ok(encoded_len) => {
///         println!("YAY0 encoded {} bytes", encoded_len);
///         dst.truncate(encoded_len as usize);
///     },
///     Err(szs::Error::Error(e)) => println!("Error: {:?}", e),
/// }
/// ```
pub fn encode_yay0_into(dst: &mut [u8], src: &[u8], algo: EncodeAlgo) -> Result<u32, Error> {
    let temp_encoded = encode(src, algo)?;
    let deinterlaced_size = deinterlaced_upper_bound(temp_encoded.len() as u32);

    if deinterlaced_size > dst.len() as u32 {
        return Err(Error::Error(
            "Destination buffer not large enough".to_string(),
        ));
    }

    deinterlace_into(dst, &temp_encoded)
}

/// Encodes and deinterlaces the source slice into YAY0 format using the given encoding algorithm and returns the result.
///
/// # Arguments
///
/// * `src`: A byte slice containing the data to be encoded.
/// * `algo`: The encoding algorithm to be used.
///
/// # Returns
///
/// * `Ok(Vec<u8>)`: A `Vec<u8>` containing the YAY0 encoded data.
/// * `Err(Error)`: An error encountered during the encoding or deinterlacing process.
///
/// # Examples
///
/// ```
/// let src = b"some data to encode into YAY0";
/// let algorithm = szs::EncodeAlgo::Nintendo;
///
/// match szs::encode_yay0(src, algorithm) {
///     Ok(encoded_data) => println!("YAY0 encoded data length: {}", encoded_data.len()),
///     Err(szs::Error::Error(e)) => println!("Error: {:?}", e),
/// }
/// ```
pub fn encode_yay0(src: &[u8], algo: EncodeAlgo) -> Result<Vec<u8>, Error> {
    let temp_encoded = encode(src, algo)?;
    let mut dst: Vec<u8> = vec![0; deinterlaced_upper_bound(temp_encoded.len() as u32) as usize];

    match deinterlace_into(&mut dst, &temp_encoded) {
        Ok(deinterlaced_len) => {
            dst.truncate(deinterlaced_len as usize);
            Ok(dst)
        }
        Err(e) => Err(e),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use sha2::{Digest, Sha256};
    use std::fs;
    use std::path::Path;

    fn calculate_hash(data: &[u8]) -> String {
        let mut hasher = Sha256::new();
        hasher.update(data);
        format!("{:x}", hasher.finalize())
    }

    fn test_encode_helper(src: &[u8], algo: EncodeAlgo, expected_hash: &str, is_yay0: bool) {
        let encoded = if is_yay0 {
            encode_yay0(src, algo).unwrap()
        } else {
            encode(src, algo).unwrap()
        };
        let hash = calculate_hash(&encoded);
        assert_eq!(hash, expected_hash);
    }

    fn read_file(path: &str) -> Vec<u8> {
        fs::read(path).expect("Unable to read file")
    }

    #[test]
    fn test_encode_worst_case_szs() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::WorstCaseEncoding,
            "f13a115669f980c7ffde6373c42233c2a9eb2a86f14f75e6747604cddc2d161e",
            false,
        );
    }
    #[test]
    fn test_encode_worst_case_szp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::WorstCaseEncoding,
            "e2143bd7477ab063c5cf76e35efd75c9b5cdc03dc87af7f3f73f7f392d734832",
            true,
        );
    }

    #[test]
    fn test_encode_mkw() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::MKW,
            "4671c3aeb8e6c50237043af870bd1ed8cae20a56c9f44a5e793caa677f656774",
            false,
        );
    }
    #[test]
    fn test_encode_mkw_szp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::MKW,
            "b6d8d06846a29b966f8fc19779f886f3bcae906d492df9d75ac1ba055c6592e2",
            true,
        );
    }

    #[test]
    fn test_encode_mkw_sp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::MkwSp,
            "1e69f92435555c89e092208685480f7b4a1aa3c033fdcdb953397bd24c45a181",
            false,
        );
    }
    #[test]
    fn test_encode_mkw_sp_szp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::MkwSp,
            "0f4ded82b8a5718d0f82c98ba935ed662c8e85c50b9fb0dc3e63f89186328e1a",
            true,
        );
    }

    #[test]
    fn test_encode_ctgp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::CTGP,
            "423e6d6b89842350f2e825cfaed439f253f597a43d67dedf74c27f432d82abd5",
            false,
        );
    }
    #[test]
    fn test_encode_ctgp_szp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::CTGP,
            "7fa1c8b791f9009638edf8f4cca14480281114002004c8fb987bbf6c6c18c864",
            true,
        );
    }

    #[test]
    fn test_encode_haroohie() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::Haroohie,
            "e1042ca2e8a138c6b39b45b2c6e7aba794f949949f73eaa11a739248ad67e969",
            false,
        );
    }
    #[test]
    fn test_encode_haroohie_szp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::Haroohie,
            "ef7dc6f5ddb3141841b6b85f0dfc34a206f973c7c458fc69bbb0c72f05439880",
            true,
        );
    }
    #[test]
    fn test_encode_ctlib() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::CTLib,
            "1e66fd9b33361b07df33442268b0c2e36120538515a528edd7484889767032ad",
            false,
        );
    }
    #[test]
    fn test_encode_ctlib_szp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::CTLib,
            "38896b86b05b8329bdda5447927efa594b9d149522105c9e051cfd9c9e628c0c",
            true,
        );
    }

    #[test]
    fn test_encode_libyaz0() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::LibYaz0,
            "fb8a40ee24422c79cdb8f6525a05d463232830fc39bc29f2db6dfc6902b54827",
            false,
        );
    }
    #[test]
    fn test_encode_libyaz0_szp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::LibYaz0,
            "7ac44e628252a4d252c0f9c21ba7e28480ff825b6e6b43fb1f06093d0106e486",
            true,
        );
    }

    #[test]
    fn test_encode_mk8() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::MK8,
            "13388da38a6f09cf95a2372a8b0a93f1b87b72bf2f1ff0e898e9d3afa7a5999d",
            false,
        );
    }
    #[test]
    fn test_encode_mk8_szp() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        test_encode_helper(
            &src,
            EncodeAlgo::MK8,
            "17db5fa76ceb987b706a87fe1a0392edfc81915c605c674306614ead48b5efe8",
            true,
        );
    }

    #[test]
    fn test_decode_yaz0_mk8() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        let encoded = encode(&src, EncodeAlgo::MK8).unwrap();
        let decoded = decode(&encoded).unwrap();
        let hash = calculate_hash(&decoded);
        assert_eq!(hash, calculate_hash(&src));
    }

    #[test]
    fn test_decode_yay0_mk8() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        let encoded = encode_yay0(&src, EncodeAlgo::MK8).unwrap();
        let decoded = decode_yay0(&encoded).unwrap();
        let hash = calculate_hash(&decoded);
        assert_eq!(hash, calculate_hash(&src));
    }

    #[test]
    fn test_decode_yaz0_mkw() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        let encoded = encode(&src, EncodeAlgo::Nintendo).unwrap();
        let decoded = decode(&encoded).unwrap();
        let hash = calculate_hash(&decoded);
        assert_eq!(hash, calculate_hash(&src));
    }
    #[test]
    fn test_decode_yay0_mkw() {
        let src = read_file("../../tests/samples/old_koopa_64.arc");
        let encoded = encode_yay0(&src, EncodeAlgo::Nintendo).unwrap();
        let decoded = decode_yay0(&encoded).unwrap();
        let hash = calculate_hash(&decoded);
        assert_eq!(hash, calculate_hash(&src));
    }
}

//--------------------------------------------------------
//
// C BINDINGS BEGIN
//
//--------------------------------------------------------
#[cfg(feature = "c_api")]
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

        match encode_into(dst_slice, src_slice, algo) {
            Ok(used_len) => {
                unsafe {
                    *result = used_len;
                }
                std::ptr::null()
            }
            Err(Error::Error(msg)) => {
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

        match decode_into(dst_slice, src_slice) {
            Ok(()) => std::ptr::null(),
            Err(Error::Error(msg)) => {
                let c_string = std::ffi::CString::new(msg).unwrap();
                // Leak the CString into a raw pointer, so we don't deallocate it
                c_string.into_raw()
            }
        }
    }

    #[no_mangle]
    pub unsafe extern "C" fn riiszs_decode_yay0_into(
        dst: *mut u8,
        dst_len: u32,
        src: *const u8,
        src_len: u32,
    ) -> *const c_char {
        let dst_slice = unsafe { std::slice::from_raw_parts_mut(dst, dst_len as usize) };
        let src_slice = unsafe { std::slice::from_raw_parts(src, src_len as usize) };

        match decode_yay0_into(dst_slice, src_slice) {
            Ok(()) => std::ptr::null(),
            Err(Error::Error(msg)) => {
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
            Err(Error::Error(msg)) => {
                let c_string = std::ffi::CString::new(msg).unwrap();
                // Leak the CString into a raw pointer, so we don't deallocate it
                c_string.into_raw()
            }
        }
    }
}
