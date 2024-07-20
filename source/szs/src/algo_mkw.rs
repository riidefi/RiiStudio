struct EGGContext {
    s_skip_table: [u16; 256],
}

impl EGGContext {
    fn new() -> Self {
        Self {
            s_skip_table: [0; 256],
        }
    }

    fn find_match(
        &mut self,
        src: &[u8],
        src_pos: usize,
        max_size: usize,
        match_offset: &mut usize,
        match_size: &mut usize,
    ) {
        // SZS backreference types:
        // (2 bytes) N >= 2:  NR RR    -> maxMatchSize=16+2,    windowOffset=4096+1
        // (3 bytes) N >= 18: 0R RR NN -> maxMatchSize=0xFF+18, windowOffset=4096+1
        let mut window = if src_pos > 4096 { src_pos - 4096 } else { 0 };
        let mut window_size = 3;
        let max_match_size = if max_size - src_pos <= 273 {
            max_size - src_pos
        } else {
            273
        };
        if max_match_size < 3 {
            *match_size = 0;
            *match_offset = 0;
            return;
        }

        let mut window_offset = 0;
        let mut found_match_offset = 0;
        while window < src_pos && {
            window_offset = self.search_window(
                &src[src_pos..src_pos + window_size],
                &src[window..src_pos + window_size],
            );
            window_offset < src_pos - window
        } {
            for _ in window_size..max_match_size {
                if src[window + window_offset + window_size] != src[src_pos + window_size] {
                    break;
                }
                window_size += 1;
            }
            if window_size == max_match_size {
                *match_offset = window + window_offset;
                *match_size = max_match_size;
                return;
            }
            found_match_offset = window + window_offset;
            window_size += 1;
            window += window_offset + 1;
        }
        *match_offset = found_match_offset;
        *match_size = if window_size > 3 { window_size - 1 } else { 0 };
    }
    fn search_window(&mut self, needle: &[u8], haystack: &[u8]) -> usize {
        let needle_size = needle.len() as isize;
        let haystack_size = haystack.len() as isize;
        if needle_size > haystack_size {
            return haystack_size as usize;
        }
        self.compute_skip_table(needle, needle_size as usize);

        // Scan forwards for the last character in the needle
        let mut it_haystack = needle_size - 1;
        loop {
            while needle[(needle_size - 1) as usize] != haystack[it_haystack as usize] {
                it_haystack += self.s_skip_table[haystack[it_haystack as usize] as usize] as isize;
                if it_haystack >= haystack_size {
                    return haystack_size as usize;
                }
            }
            it_haystack -= 1;
            let mut it_needle = needle_size - 2;

            // Scan backwards for the first difference
            let remaining_bytes = it_needle;
            let mut j = 0;
            while j <= remaining_bytes {
                if haystack[it_haystack as usize] != needle[it_needle as usize] {
                    break;
                }
                // These may be negative for the final iteration
                it_haystack -= 1;
                it_needle -= 1;
                j += 1;
            }

            if j > remaining_bytes {
                // The entire needle was found
                return (it_haystack + 1) as usize;
            }

            // The entire needle was not found, continue search
            let mut skip = self.s_skip_table[haystack[it_haystack as usize] as usize] as isize;
            if needle_size - it_needle > skip {
                skip = needle_size - it_needle;
            }
            it_haystack += skip;
        }
    }

    fn compute_skip_table(&mut self, needle: &[u8], needle_size: usize) {
        for i in 0..256 {
            self.s_skip_table[i] = needle_size as u16;
        }
        for i in 0..needle_size {
            self.s_skip_table[needle[i] as usize] = (needle_size - i - 1) as u16;
        }
    }
}

pub fn encode_boyer_moore_horspool(src: &[u8], dst: &mut [u8]) -> usize {
    let src_size = src.len();

    let mut src_pos;
    let mut group_header_pos;
    let mut dst_pos;
    let mut group_header_bit_raw;
    let mut ctx = EGGContext::new();

    dst[0] = b'Y';
    dst[1] = b'a';
    dst[2] = b'z';
    dst[3] = b'0';
    dst[4] = 0;
    dst[5] = (src_size >> 16) as u8;
    dst[6] = (src_size >> 8) as u8;
    dst[7] = src_size as u8;
    dst[16] = 0;

    src_pos = 0;
    group_header_bit_raw = 0x80;
    group_header_pos = 16;
    dst_pos = 17;
    while src_pos < src_size {
        let mut match_offset = 0;
        let mut first_match_len = 0;
        ctx.find_match(
            src,
            src_pos,
            src_size,
            &mut match_offset,
            &mut first_match_len,
        );
        if first_match_len > 2 {
            let mut second_match_offset = 0;
            let mut second_match_len = 0;
            ctx.find_match(
                src,
                src_pos + 1,
                src_size,
                &mut second_match_offset,
                &mut second_match_len,
            );
            if first_match_len + 1 < second_match_len {
                dst[group_header_pos] |= group_header_bit_raw;
                group_header_bit_raw >>= 1;
                dst[dst_pos] = src[src_pos];
                dst_pos += 1;
                src_pos += 1;
                if group_header_bit_raw == 0 {
                    group_header_bit_raw = 0x80;
                    group_header_pos = dst_pos;
                    dst[dst_pos] = 0;
                    dst_pos += 1;
                }
                first_match_len = second_match_len;
                match_offset = second_match_offset;
            }
            match_offset = src_pos - match_offset - 1;
            if first_match_len < 18 {
                match_offset |= (first_match_len - 2) << 12;
                dst[dst_pos] = (match_offset >> 8) as u8;
                dst[dst_pos + 1] = match_offset as u8;
                dst_pos += 2;
            } else {
                dst[dst_pos] = (match_offset >> 8) as u8;
                dst[dst_pos + 1] = match_offset as u8;
                dst[dst_pos + 2] = (first_match_len - 18) as u8;
                dst_pos += 3;
            }
            src_pos += first_match_len;
        } else {
            dst[group_header_pos] |= group_header_bit_raw;
            dst[dst_pos] = src[src_pos];
            dst_pos += 1;
            src_pos += 1;
        }

        group_header_bit_raw >>= 1;
        if group_header_bit_raw == 0 {
            group_header_bit_raw = 0x80;
            group_header_pos = dst_pos;
            dst[dst_pos] = 0;
            dst_pos += 1;
        }
    }

    dst_pos
}
