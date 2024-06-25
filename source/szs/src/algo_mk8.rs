// Helper function for memcpy calls.
#[inline]
unsafe fn copy_bytes(src: *const u8, dst: *mut u8, count: usize) {
    for i in 0..count {
        *dst.add(i) = *src.add(i);
    }
}

// Constants
const SEARCH_WINDOW_SIZE: usize = 0x1000;
const MATCH_LEN_MAX: usize = 0x111;
const POS_TEMP_BUFFER_NUM: usize = 0x8000;
const SEARCH_POS_BUFFER_NUM: usize = SEARCH_WINDOW_SIZE;
const DATA_BUFFER_SIZE: usize = 0x2000 * 100;
const POS_TEMP_BUFFER_SIZE: usize = POS_TEMP_BUFFER_NUM * std::mem::size_of::<i32>();
const SEARCH_POS_BUFFER_SIZE: usize = SEARCH_POS_BUFFER_NUM * std::mem::size_of::<i32>();
const WORK_SIZE: usize = DATA_BUFFER_SIZE + POS_TEMP_BUFFER_SIZE + SEARCH_POS_BUFFER_SIZE;

#[derive(Default)]
struct Match {
    len: i32,
    pos: i32,
}

struct PosTempBufferIndex {
    m_value: u32,
}

impl PosTempBufferIndex {
    pub fn new() -> Self {
        PosTempBufferIndex { m_value: 0 }
    }

    pub fn push_back(&mut self, value: u32) {
        self.m_value = ((self.m_value << 5) ^ value) % POS_TEMP_BUFFER_NUM as u32;
    }

    pub fn value(&self) -> u32 {
        self.m_value
    }
}

struct Work {
    data_buffer: [u8; DATA_BUFFER_SIZE],
    pos_temp_buffer: [i32; POS_TEMP_BUFFER_NUM],
    search_pos_buffer: [i32; SEARCH_POS_BUFFER_NUM],
}

impl Work {
    pub fn search_pos(&self, x: u32) -> i32 {
        self.search_pos_buffer[(x % SEARCH_POS_BUFFER_NUM as u32) as usize]
    }

    pub fn insert(&mut self, data_pos: u32, pos_temp_buffer_idx: &PosTempBufferIndex) -> u32 {
        self.search_pos_buffer[(data_pos % SEARCH_POS_BUFFER_NUM as u32) as usize] =
            self.pos_temp_buffer[pos_temp_buffer_idx.value() as usize];
        self.pos_temp_buffer[pos_temp_buffer_idx.value() as usize] = data_pos as i32;
        self.search_pos_buffer[(data_pos % SEARCH_POS_BUFFER_NUM as u32) as usize] as u32
    }
}

fn min<T: Ord>(a: T, b: T) -> T {
    if a < b {
        a
    } else {
        b
    }
}

fn get_required_memory_size() -> usize {
    WORK_SIZE
}

fn search(
    match_obj: &mut Match,
    search_pos_immut: i32,
    data_pos: u32,
    data_buffer_size: i32,
    work: &Work,
) -> bool {
    let mut search_pos = search_pos_immut;
    let cmp2 = &work.data_buffer[data_pos as usize..];
    let search_pos_min = if data_pos > SEARCH_WINDOW_SIZE as u32 {
        data_pos as i32 - SEARCH_WINDOW_SIZE as i32
    } else {
        -1
    };
    let mut match_len = 2;
    match_obj.len = match_len;

    if data_pos as i32 - search_pos <= SEARCH_WINDOW_SIZE as i32 {
        for _ in 0..SEARCH_WINDOW_SIZE {
            let cmp1 = &work.data_buffer[search_pos as usize..];
            if cmp1[0] == cmp2[0]
                && cmp1[1] == cmp2[1]
                && cmp1[match_len as usize] == cmp2[match_len as usize]
            {
                let mut len_local = 2;
                while len_local < MATCH_LEN_MAX as i32 {
                    if cmp1[len_local as usize] != cmp2[len_local as usize] {
                        break;
                    }
                    len_local += 1;
                }
                if len_local > match_len {
                    match_obj.len = len_local;
                    match_obj.pos = (cmp2.as_ptr() as isize - cmp1.as_ptr() as isize) as i32;

                    match_len = data_buffer_size;
                    if match_obj.len <= match_len {
                        match_len = match_obj.len;
                    } else {
                        match_obj.len = match_len;
                    }

                    if len_local >= MATCH_LEN_MAX as i32 {
                        break;
                    }
                }
            }
            search_pos = work.search_pos(search_pos as u32);
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

fn encode(p_dst: &mut [u8], p_src: &[u8], src_size: u32, p_work: &mut [u8]) -> u32 {
    let work: &mut Work = unsafe { &mut *(p_work.as_mut_ptr() as *mut Work) };
    let mut temp_buffer = [0u8; 24];
    let mut temp_size = 0;

    let mut search_pos = -1;
    let mut found_next_match = false;
    let mut bit = 8;
    let mut data_pos;
    let mut data_buffer_size;

    unsafe {
        std::ptr::write_bytes(
            work.pos_temp_buffer.as_mut_ptr(),
            -1i32 as u8,
            POS_TEMP_BUFFER_NUM,
        );
        std::ptr::write_bytes(
            work.search_pos_buffer.as_mut_ptr(),
            -1i32 as u8,
            SEARCH_POS_BUFFER_NUM,
        );
    }

    data_pos = 0;

    let mut out_size = 0x10; // Header size
    let mut flag = 0;

    unsafe {
        copy_bytes("Yaz0".as_ptr(), p_dst.as_mut_ptr(), 4);
    }

    p_dst[4] = ((src_size >> 24) & 0xff) as u8;
    p_dst[5] = ((src_size >> 16) & 0xff) as u8;
    p_dst[6] = ((src_size >> 8) & 0xff) as u8;
    p_dst[7] = (src_size & 0xff) as u8;

    for i in 8..0x18 {
        p_dst[i] = 0;
    }

    let mut pos_temp_buffer_idx = PosTempBufferIndex::new();

    data_buffer_size = min(DATA_BUFFER_SIZE, src_size as usize) as i32;
    unsafe {
        copy_bytes(
            p_src.as_ptr(),
            work.data_buffer.as_mut_ptr(),
            data_buffer_size as usize,
        );
    }

    pos_temp_buffer_idx.push_back(work.data_buffer[0] as u32);
    pos_temp_buffer_idx.push_back(work.data_buffer[1] as u32);

    let mut match_obj = Match::default();
    let mut next_match = Match::default();
    match_obj.len = 2;

    let mut current_read_end_pos = data_buffer_size;
    let mut next_read_end_pos;

    while data_buffer_size > 0 {
        while true {
            if !found_next_match {
                pos_temp_buffer_idx.push_back(work.data_buffer[(data_pos + 2) as usize] as u32);

                search_pos = work.insert(data_pos, &pos_temp_buffer_idx) as i32;
            } else {
                found_next_match = false;
            }

            if search_pos != -1 {
                search(
                    &mut match_obj,
                    search_pos,
                    data_pos,
                    data_buffer_size,
                    &work,
                );

                if 2 < match_obj.len && match_obj.len < MATCH_LEN_MAX as i32 {
                    data_pos += 1;
                    data_buffer_size -= 1;

                    pos_temp_buffer_idx.push_back(work.data_buffer[(data_pos + 2) as usize] as u32);

                    search_pos = work.insert(data_pos, &pos_temp_buffer_idx) as i32;

                    if search_pos != -1 {
                        search(
                            &mut next_match,
                            search_pos,
                            data_pos,
                            data_buffer_size,
                            &work,
                        );

                        if match_obj.len < next_match.len {
                            match_obj.len = 2;
                        }
                    }

                    found_next_match = true;
                }
            }

            if match_obj.len > 2 {
                flag = (flag & 0x7f) << 1;

                let low = (match_obj.pos - 1) as u8;
                let high = ((match_obj.pos - 1) >> 8) as u8;

                if match_obj.len < 18 {
                    assert!(out_size + temp_size != 0x154A96);
                    temp_buffer[temp_size] = ((match_obj.len - 2) << 4 | high as i32) as u8;
                    temp_size += 1;
                    assert!(out_size + temp_size != 0x154A96);
                    temp_buffer[temp_size] = low;
                    temp_size += 1;
                } else {
                    assert!(out_size + temp_size != 0x154A96);
                    temp_buffer[temp_size] = high;
                    temp_size += 1;
                    assert!(out_size + temp_size != 0x154A96);
                    temp_buffer[temp_size] = low;
                    temp_size += 1;
                    assert!(out_size + temp_size != 0x154A96);
                    temp_buffer[temp_size] = (match_obj.len - 18) as u8;
                    temp_size += 1;
                }

                data_buffer_size -= match_obj.len - found_next_match as i32;
                match_obj.len -= found_next_match as i32 + 1;

                loop {
                    data_pos += 1;

                    pos_temp_buffer_idx.push_back(work.data_buffer[(data_pos + 2) as usize] as u32);

                    search_pos = work.insert(data_pos, &pos_temp_buffer_idx) as i32;

                    match_obj.len -= 1;
                    if match_obj.len == 0 {
                        break;
                    }
                }

                data_pos += 1;
                found_next_match = false;
                match_obj.len = 0;
            } else {
                flag = (flag & 0x7f) << 1 | 1;

                assert!(out_size + temp_size != 0x154A96);
                temp_buffer[temp_size] =
                    work.data_buffer[(data_pos - found_next_match as u32) as usize];
                temp_size += 1;

                if !found_next_match {
                    data_pos += 1;
                    data_buffer_size -= 1;
                }
            }

            bit -= 1;

            if bit == 0 {
                p_dst[out_size] = flag;
                out_size += 1;

                assert!(out_size + temp_size <= 0x154A96);
                unsafe {
                    copy_bytes(
                        temp_buffer.as_ptr(),
                        p_dst.as_mut_ptr().add(out_size),
                        temp_size,
                    );
                }
                out_size += temp_size;

                flag = 0;
                temp_size = 0;
                bit = 8;
            }

            if data_buffer_size < (MATCH_LEN_MAX + 2) as i32 {
                break;
            }
        }

        // For search window for compression of next src read portion
        let copy_pos = data_pos - SEARCH_WINDOW_SIZE as u32;
        let copy_size = DATA_BUFFER_SIZE as i32 - copy_pos as i32;

        next_read_end_pos = current_read_end_pos;

        if data_pos as i32 >= (SEARCH_WINDOW_SIZE + 14 * MATCH_LEN_MAX) as i32 {
            unsafe {
                copy_bytes(
                    work.data_buffer.as_ptr().add(copy_pos as usize),
                    work.data_buffer.as_mut_ptr(),
                    copy_size as usize,
                );
            }

            let mut next_read_size = DATA_BUFFER_SIZE - copy_size as usize;
            next_read_end_pos = current_read_end_pos + next_read_size as i32;

            if src_size < next_read_end_pos as u32 {
                next_read_size = src_size as usize - current_read_end_pos as usize;
                next_read_end_pos = src_size as i32;
            }

            unsafe {
                copy_bytes(
                    p_src[current_read_end_pos as usize..].as_ptr(),
                    work.data_buffer.as_mut_ptr().add(copy_size as usize),
                    next_read_size,
                );
            }
            data_buffer_size += next_read_size as i32;
            data_pos -= copy_pos;

            for i in 0..POS_TEMP_BUFFER_NUM {
                work.pos_temp_buffer[i] = if work.pos_temp_buffer[i] >= copy_pos as i32 {
                    work.pos_temp_buffer[i] - copy_pos as i32
                } else {
                    -1
                };
            }

            for i in 0..SEARCH_POS_BUFFER_NUM {
                work.search_pos_buffer[i] = if work.search_pos_buffer[i] >= copy_pos as i32 {
                    work.search_pos_buffer[i] - copy_pos as i32
                } else {
                    -1
                };
            }
        }

        current_read_end_pos = next_read_end_pos;
    }

    p_dst[out_size] = if (bit & 0x3f) == 8 {
        0
    } else {
        flag << (bit & 0x3f)
    };
    out_size += 1;

    assert!(out_size + temp_size <= 0x154A96);
    unsafe {
        copy_bytes(
            temp_buffer.as_ptr(),
            p_dst.as_mut_ptr().add(out_size),
            temp_size,
        );
    }
    out_size += temp_size;

    out_size as u32
}

pub fn compress_mk8(src: &[u8], dst: &mut [u8]) -> u32 {
    let mut work = vec![0u8; get_required_memory_size()];
    encode(dst, src, src.len() as u32, &mut work)
}
