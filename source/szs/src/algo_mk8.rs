use std::cmp::min;
use std::cmp::PartialOrd;
use std::convert::TryInto;

// Type aliases for ease of use.
type S32 = i32;
type U32 = u32;
type U8 = u8;
type S64 = i64;

// Constants ported from the C++ code.
const SEARCH_WINDOW_SIZE: S32 = 0x1000;
const MATCH_LEN_MAX: S32 = 0x111;
const POS_TEMP_BUFFER_NUM: S32 = 0x8000;
const SEARCH_POS_BUFFER_NUM: S32 = SEARCH_WINDOW_SIZE;
const DATA_BUFFER_SIZE: S32 = 0x2000 * 100;

// Helper function, seadMathMin in the original C++ code.
fn sead_math_min<T: PartialOrd>(a: T, b: T) -> T {
    if a < b { a } else { b }
}

// Structs ported from the C++ code.
struct Match {
    len: S32,
    pos: S32,
}

struct PosTempBufferIndex {
    m_value: U32,
}

impl PosTempBufferIndex {
    fn new() -> Self {
        Self { m_value: 0 }
    }

    fn push_back(&mut self, value: U32) {
        self.m_value = ((self.m_value << 5) ^ value) % POS_TEMP_BUFFER_NUM as u32;
    }

    fn value(&self) -> U32 {
        self.m_value
    }
}

struct TempBuffer {
    temp_buffer: [U8; 24],
    temp_size: U32,
}

impl TempBuffer {
    fn new() -> Self {
        Self {
            temp_buffer: [0; 24],
            temp_size: 0,
        }
    }

    fn put(&mut self, x: U8) {
        self.temp_buffer[self.temp_size as usize] = x;
        self.temp_size += 1;
    }
}

struct Work {
    data_buffer: [U8; DATA_BUFFER_SIZE as usize],
    pos_temp_buffer: [S32; POS_TEMP_BUFFER_NUM as usize],
    search_pos_buffer: [S32; SEARCH_POS_BUFFER_NUM as usize],
}

impl Work {
    fn search_pos(&self, x: U32) -> S32 {
        self.search_pos_buffer[(x % SEARCH_POS_BUFFER_NUM as u32) as usize]
    }

    fn search_pos_mut(&mut self, x: U32) -> &mut S32 {
        &mut self.search_pos_buffer[(x % SEARCH_POS_BUFFER_NUM as u32) as usize]
    }

    fn insert(&mut self, data_pos: U32, pos_temp_buffer_idx: &PosTempBufferIndex) -> U32 {
        let idx = (data_pos % SEARCH_POS_BUFFER_NUM as u32) as usize;
        let pos_temp_value = pos_temp_buffer_idx.value() as usize;
        self.search_pos_buffer[idx] = self.pos_temp_buffer[pos_temp_value];
        self.pos_temp_buffer[pos_temp_value] = data_pos as S32;
        self.search_pos_buffer[idx] as U32
    }
}

fn get_required_memory_size() -> U32 {
    std::mem::size_of::<Work>() as U32
}

fn search(
    match_info: &mut Match,
    search_pos_immut: S32,
    data_pos: U32,
    data_buffer_size: S32,
    work: &Work
) -> bool {
    let mut search_pos = search_pos_immut;
    let cmp2 = &work.data_buffer[data_pos as usize..];

    let search_pos_min =
        if data_pos > SEARCH_WINDOW_SIZE as U32 {
            data_pos.saturating_sub(SEARCH_WINDOW_SIZE as U32) as S32
        } else {
            -1
        };

    let mut match_len = 2;
    match_info.len = match_len;

    if data_pos as S32 - search_pos <= SEARCH_WINDOW_SIZE {
        for _ in 0..SEARCH_WINDOW_SIZE {
            let cmp1 = &work.data_buffer[search_pos as usize..];

            if cmp1[0] == cmp2[0]
                && cmp1[1] == cmp2[1]
                && cmp1[match_len as usize] == cmp2[match_len as usize]
            {
                let mut len_local = 2;

                while len_local < MATCH_LEN_MAX {
                    if cmp1[len_local as usize] != cmp2[len_local as usize] {
                        break;
                    }
                    len_local += 1;
                }

                if len_local > match_len {
                    match_info.len = len_local;
                    match_info.pos = data_pos as S32 - search_pos;

                    match_len = data_buffer_size;
                    if (match_info.len as S64) <= match_len as S64 {
                        match_len = match_info.len;
                    } else {
                        match_info.len = match_len;
                    }

                    if len_local >= MATCH_LEN_MAX {
                        break;
                    }
                }
            }

            search_pos = work.search_pos(search_pos as U32);
            if search_pos <= search_pos_min {
                break;
            }
        }

        if match_len >= 3 {
            return true;
        }
    }

    false
}

fn encode(p_dst: &mut [U8], p_src: &[U8], src_size: U32, p_work: &mut [U8]) -> U32 {
    let mut work = unsafe { &mut *(p_work.as_mut_ptr() as *mut Work) };
    let mut tmp = TempBuffer::new();
    let mut search_pos : i32 = -1;
    let mut found_next_match = false;
    let mut bit = 8;

    let mut data_pos : u32;
    let mut data_buffer_size : i32;

    work.pos_temp_buffer.iter_mut().for_each(|x| *x = -1);
    work.search_pos_buffer.iter_mut().for_each(|x| *x = -1);

    data_pos = 0;

    let mut out_size = 0x10; // Header size
    let mut flag: U32 = 0;

    // Yaz0 header
    p_dst[..4].copy_from_slice(b"Yaz0");
    p_dst[4] = (src_size >> 24) as u8;
    p_dst[5] = (src_size >> 16) as u8;
    p_dst[6] = (src_size >> 8) as u8;
    p_dst[7] = (src_size >> 0) as u8;
    p_dst[8..16].fill(0);

    let mut pos_temp_buffer_idx = PosTempBufferIndex::new();

    data_buffer_size = sead_math_min(DATA_BUFFER_SIZE, src_size as S32);
    work.data_buffer[..data_buffer_size as usize].copy_from_slice(&p_src[..data_buffer_size as usize]);

    pos_temp_buffer_idx.push_back(work.data_buffer[0] as U32);
    pos_temp_buffer_idx.push_back(work.data_buffer[1] as U32);

    let mut match_info = Match { len: 2, pos: 0 };
    let mut next_match = Match { len: 0, pos: 0 };

    let mut current_read_end_pos = data_buffer_size;
    let mut next_read_end_pos;

    while data_buffer_size > 0 {
        loop {
            if !found_next_match {
                pos_temp_buffer_idx.push_back(work.data_buffer[(data_pos + 2) as usize] as U32);

                search_pos = work.insert(data_pos, &pos_temp_buffer_idx) as S32;
            } else {
                found_next_match = false;
            }

            if search_pos != -1 {
                search(&mut match_info, search_pos, data_pos, data_buffer_size, &work);

                if 2 < match_info.len && match_info.len < MATCH_LEN_MAX {
                    data_pos += 1;
                    data_buffer_size -= 1;

                    pos_temp_buffer_idx.push_back(work.data_buffer[(data_pos + 2) as usize] as U32);

                    search_pos = work.insert(data_pos, &pos_temp_buffer_idx) as S32;

                    if search_pos != -1 {
                        search(&mut next_match, search_pos, data_pos, data_buffer_size, &work);

                        if match_info.len < next_match.len {
                            match_info.len = 2;
                        }
                    }

                    found_next_match = true;
                }
            }

            if match_info.len > 2 {
                flag = (flag & 0x7f) << 1;

                let low = (match_info.pos - 1) as U8;
                let high = ((match_info.pos - 1) >> 8) as U8;

                if match_info.len < 18 {
                    tmp.put(((match_info.len - 2) << 4) as U8 | high);
                    tmp.put(low);
                } else {
                    tmp.put(high);
                    tmp.put(low);
                    tmp.put((match_info.len - 18) as U8);
                }

                data_buffer_size -= match_info.len - found_next_match as S32;
                match_info.len -= found_next_match as S32 + 1;

                loop {
                    data_pos += 1;

                    pos_temp_buffer_idx.push_back(work.data_buffer[(data_pos + 2) as usize] as U32);

                    search_pos = work.insert(data_pos, &pos_temp_buffer_idx) as S32;
                    match_info.len -= 1;

                    if match_info.len == 0 {
                        break;
                    }
                }

                data_pos += 1;
                found_next_match = false;
                match_info.len = 0;
            } else {
                flag = (flag & 0x7f) << 1 | 1;

                tmp.put(work.data_buffer[(data_pos - (found_next_match as u32)) as usize] as U8);

                if !found_next_match {
                    data_pos += 1;
                    data_buffer_size -= 1;
                }
            }

            bit -= 1;

            if bit == 0 {
                p_dst[out_size as usize] = flag as U8;
                out_size += 1;

                p_dst[out_size as usize..(out_size + tmp.temp_size) as usize]
                    .copy_from_slice(&tmp.temp_buffer[..tmp.temp_size as usize]);
                out_size += tmp.temp_size;

                flag = 0;
                tmp.temp_size = 0;
                bit = 8;
            }

            if data_buffer_size < MATCH_LEN_MAX + 2 {
                break;
            }
        }

        let copy_pos = data_pos - SEARCH_WINDOW_SIZE as u32;
        let copy_size = DATA_BUFFER_SIZE - copy_pos as i32;

        next_read_end_pos = current_read_end_pos;

        if data_pos >= (SEARCH_WINDOW_SIZE + 14 * MATCH_LEN_MAX) as u32 {
            work.data_buffer.copy_within(copy_pos as usize..DATA_BUFFER_SIZE as usize, 0);

            let mut next_read_size = DATA_BUFFER_SIZE - copy_size;
            next_read_end_pos = current_read_end_pos + next_read_size;
            if src_size < next_read_end_pos as U32 {
                next_read_size = (src_size - current_read_end_pos as U32) as i32;
                next_read_end_pos = src_size as S32;
            }
            work.data_buffer[copy_size as usize..(copy_size + next_read_size) as usize]
                .copy_from_slice(&p_src[current_read_end_pos as usize..next_read_end_pos as usize]);

            data_buffer_size += next_read_size as S32;
            data_pos -= copy_pos;

            for i in 0..POS_TEMP_BUFFER_NUM as usize {
                work.pos_temp_buffer[i] = if work.pos_temp_buffer[i] >= copy_pos as i32 {
                    work.pos_temp_buffer[i] - copy_pos as i32
                } else {
                    -1
                };
            }

            for i in 0..SEARCH_POS_BUFFER_NUM as usize {
                work.search_pos_buffer[i] = if work.search_pos_buffer[i] >= copy_pos as i32 {
                    work.search_pos_buffer[i] - copy_pos as i32
                } else {
                    -1
                };
            }
        }

        current_read_end_pos = next_read_end_pos;
    }

    p_dst[out_size as usize] = if (bit & 0x3f) == 8 { 0 } else { (flag << (bit & 0x3f)) as U8 };
    out_size += 1;

    p_dst[out_size as usize..(out_size + tmp.temp_size) as usize]
        .copy_from_slice(&tmp.temp_buffer[..tmp.temp_size as usize]);
    out_size += tmp.temp_size;

    out_size
}

pub fn compress_mk8(src: &[U8], dst: &mut [U8]) -> U32 {
    let mut work: Vec<U8> = vec![0; get_required_memory_size() as usize];
    encode(dst, src, src.len() as U32, &mut work)
}
