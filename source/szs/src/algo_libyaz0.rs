use memchr::memchr;
use std::convert::TryInto;

struct SearchResult {
    found: u32,
    found_len: u32,
}

fn compression_search(
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
        if let Some(pos) = memchr(c1, &src[search..src_pos]) {
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

pub fn compress_yaz(src: &[u8], level: u8, dst: &mut [u8]) -> u32 {
    let yaz0_magic: u32 = 0x59617A30; // "Yaz0" in ASCII

    dst[0..4].copy_from_slice(&yaz0_magic.to_be_bytes());
    dst[4..8].copy_from_slice(&(src.len() as u32).to_be_bytes());
    dst[8..12].copy_from_slice(&0u32.to_be_bytes());
    dst[12..16].copy_from_slice(&0u32.to_be_bytes());

    let search_range = if level == 0 {
        0
    } else if level < 9 {
        (0x10E0 * level as u32 / 9 - 0x0E0) as u32
    } else {
        0x1000
    };

    let mut src_pos = 0;
    let src_end = src.len();

    let mut dst_pos = 16;
    let mut code_byte_pos = dst_pos;

    let max_len = 18 + 255;

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
    } else if src_pos < src_end {
        let mut res = compression_search(src_pos, src, max_len, search_range);
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
                    let res_ = compression_search(src_pos + 1, src, max_len, search_range);
                    next_found = res_.found as usize;
                    next_found_len = res_.found_len as usize;
                }

                if found_len > 2 && next_found_len <= found_len {
                    let delta = src_pos - found - 1;

                    if found_len < 18 {
                        dst[dst_pos] = (delta >> 8 | (found_len - 2) << 4) as u8;
                        dst_pos += 1;
                        dst[dst_pos] = (delta & 0xFF) as u8;
                        dst_pos += 1;
                    } else {
                        dst[dst_pos] = (delta >> 8) as u8;
                        dst_pos += 1;
                        dst[dst_pos] = (delta & 0xFF) as u8;
                        dst_pos += 1;
                        dst[dst_pos] = (found_len - 18) as u8;
                        dst_pos += 1;
                    }

                    src_pos += found_len;

                    res = compression_search(src_pos, src, max_len, search_range);
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
    }

    dst_pos as u32
}
