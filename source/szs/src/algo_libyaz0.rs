// Reference C implementation:
// https://github.com/riidefi/RiiStudio/blob/master/source/szs/src/LibYaz.hpp

use memchr::memchr;
use std::ffi::c_void;

struct SearchResult {
    found: u32,
    found_len: u32,
}

fn c_memchr(c: u8, src: &[u8]) -> Option<usize> {
    unsafe {
        let result = libc::memchr(src.as_ptr() as *const c_void, c.into(), src.len());
        if result.is_null() {
            None
        } else {
            Some(result as usize - src.as_ptr() as usize)
        }
    }
}

fn memchr_template<const USE_C: bool>(c: u8, src: &[u8]) -> Option<usize> {
    if USE_C {
        c_memchr(c, src)
    } else {
        memchr(c, src)
    }
}

// https://github.com/riidefi/RiiStudio/blob/master/source/szs/src/LibYaz.hpp#L9
fn compression_search<const USE_LIBC: bool>(
    src_pos: usize,
    src: &[u8],
    max_len: usize,
    search_range: u32,
) -> SearchResult {
    let src_end = src.len();
    let mut result = SearchResult {
        found: 0,
        found_len: 0,
    };

    if src_pos + 2 >= src_end {
        return result;
    }
    let src_ptr = &src[src_pos];
    let search_start = if src_pos >= search_range as usize {
        src_pos - search_range as usize
    } else {
        0
    };
    let cmp_end = std::cmp::min(src_pos + max_len, src_end);

    let c1 = *src_ptr;
    let mut search = search_start;

    while search < src_pos {
        if let Some(pos) = memchr_template::<USE_LIBC>(c1, &src[search..src_pos]) {
            search += pos;
        } else {
            break;
        }

        let mut cmp1 = search + 1;
        let mut cmp2 = src_pos + 1;

        while cmp2 < cmp_end && src[cmp1] == src[cmp2] {
            cmp1 += 1;
            cmp2 += 1;
        }

        let len_ = cmp2 - src_pos;

        if result.found_len < len_ as u32 {
            result.found_len = len_ as u32;
            result.found = search as u32;
            if result.found_len == max_len as u32 {
                break;
            }
        }

        search += 1;
    }

    result
}

const YAZ0_MAGIC: u32 = 0x59617A30; // "Yaz0" in ASCII

// https://github.com/riidefi/RiiStudio/blob/master/source/szs/src/LibYaz.hpp#L63
pub fn compress_yaz<const USE_LIBC: bool>(src: &[u8], level: u8, dst: &mut [u8]) -> u32 {
    dst[0..4].copy_from_slice(&YAZ0_MAGIC.to_be_bytes());
    dst[4..8].copy_from_slice(&(src.len() as u32).to_be_bytes());
    dst[8..12].copy_from_slice(&0u32.to_be_bytes());
    dst[12..16].copy_from_slice(&0u32.to_be_bytes());

    let search_range = if level == 0 {
        0
    } else if level < 9 {
        0x10E0 * level as u32 / 9 - 0x0E0
    } else {
        0x1000
    };

    let mut src_pos = 0;
    let src_end = src.len();

    #[allow(unused_assignments)]
    let mut dst_pos = 16;
    let mut code_byte_pos;

    let max_len = 18 + 255;

    // Fast compression algorithm
    if search_range == 0 {
        while src_end - src_pos >= 8 {
            dst[dst_pos] = 0xFF;
            dst_pos += 1;
            dst[dst_pos..dst_pos + 8].copy_from_slice(&src[src_pos..src_pos + 8]);
            dst_pos += 8;
            src_pos += 8;
        }

        let delta = src_end - src_pos;
        if delta > 0 {
            dst[dst_pos] = ((1 << delta) - 1) << (8 - delta);
            dst_pos += 1;
            dst[dst_pos..dst_pos + delta].copy_from_slice(&src[src_pos..src_pos + delta]);
            dst_pos += delta;
        }

        // Less optimal but for matching sake,
        return ((src_end + (src_end >> 3) + 0x18) & !0x7) as u32;

        // return dst_pos as u32;
    }

    // Trivial case
    if src_pos >= src_end {
        return dst_pos as u32;
    }

    let mut res = compression_search::<USE_LIBC>(src_pos, src, max_len, search_range);
    let mut found = res.found as usize;
    let mut found_len = res.found_len as usize;

    let mut next_found = 0;
    let mut next_found_len = 0;

    while src_pos < src_end {
        code_byte_pos = dst_pos;
        dst[dst_pos] = 0;
        dst_pos += 1;

        for i in (0..8).rev() {
            if src_pos >= src_end {
                break;
            }

            if src_pos + 1 < src_end {
                let res_ = compression_search::<USE_LIBC>(src_pos + 1, src, max_len, search_range);
                next_found = res_.found as usize;
                next_found_len = res_.found_len as usize;
            }

            if found_len > 2 && next_found_len <= found_len {
                let delta = src_pos - found - 1;

                if found_len < 18 {
                    dst[dst_pos] = (delta >> 8 | (found_len - 2) << 4) as u8;
                    dst[dst_pos + 1] = (delta & 0xFF) as u8;
                    dst_pos += 2;
                } else {
                    dst[dst_pos] = (delta >> 8) as u8;
                    dst[dst_pos + 1] = (delta & 0xFF) as u8;
                    dst[dst_pos + 2] = (found_len - 18) as u8;
                    dst_pos += 3;
                }

                src_pos += found_len;

                res = compression_search::<USE_LIBC>(src_pos, src, max_len, search_range);
                found = res.found as usize;
                found_len = res.found_len as usize;
            } else {
                dst[code_byte_pos] |= 1 << i;
                dst[dst_pos] = src[src_pos];
                dst_pos += 1;
                src_pos += 1;

                found = next_found;
                found_len = next_found_len;
            }
        }
    }

    dst_pos as u32
}
