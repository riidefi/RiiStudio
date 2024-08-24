use std::assert;
use std::cmp;
use std::mem::size_of;
use std::slice;

const SEARCH_WINDOW_SIZE: usize = 0x1000;
const MATCH_LEN_MAX: usize = 0x111;

const POS_TEMP_BUFFER_NUM: usize = 0x8000;
const SEARCH_POS_BUFFER_NUM: usize = SEARCH_WINDOW_SIZE;

// In-game: 1x (~8KB)
// Official tooling: 10x presumably
// Better results attained with 100x (~1MB)
const DATA_BUFFER_SIZE: usize = 0x2000 * 100;

const POS_TEMP_BUFFER_SIZE: usize = POS_TEMP_BUFFER_NUM * size_of::<i32>();
const SEARCH_POS_BUFFER_SIZE: usize = SEARCH_POS_BUFFER_NUM * size_of::<i32>();
const WORK_SIZE: usize = DATA_BUFFER_SIZE + POS_TEMP_BUFFER_SIZE + SEARCH_POS_BUFFER_SIZE;

struct Match {
    len: i32,
    pos: i32,
}

// The compressor uses a chained hash table to find duplicated strings,
//  using a hash function that operates on 3-byte sequences.  At any
//  given point during compression, let XYZ be the next 3 input bytes to
//  be examined (not necessarily all different, of course).
struct TripletHasher {
    value: u32,
}

impl TripletHasher {
    fn new() -> Self {
        TripletHasher { value: 0 }
    }

    fn push_back(&mut self, value: u32) {
        self.value = ((self.value << 5) ^ value) % POS_TEMP_BUFFER_NUM as u32;
    }

    fn get_hash(&self) -> u32 {
        self.value
    }
}

struct TempBuffer {
    temp_buffer: [u8; 24],
    temp_size: usize,
}

impl TempBuffer {
    fn new() -> Self {
        TempBuffer {
            temp_buffer: [0; 24],
            temp_size: 0,
        }
    }

    fn put(&mut self, x: u8) {
        self.temp_buffer[self.temp_size] = x;
        self.temp_size += 1;
    }
}

struct Work {
    data_buffer: [u8; DATA_BUFFER_SIZE],
    pos_temp_buffer: [i32; POS_TEMP_BUFFER_NUM],
    search_pos_buffer: [i32; SEARCH_POS_BUFFER_NUM],
}

impl Work {
    fn search_pos(&self, x: u32) -> i32 {
        self.search_pos_buffer[x as usize % SEARCH_POS_BUFFER_NUM]
    }

    fn search_pos_mut(&mut self, x: u32) -> &mut i32 {
        &mut self.search_pos_buffer[x as usize % SEARCH_POS_BUFFER_NUM]
    }

    fn append_to_hash_chain(&mut self, data_pos: u32, pos_temp_buffer_idx: &TripletHasher) -> i32 {
        let search_pos_idx = data_pos as usize % SEARCH_POS_BUFFER_NUM;
        self.search_pos_buffer[search_pos_idx] =
            self.pos_temp_buffer[pos_temp_buffer_idx.get_hash() as usize];
        self.pos_temp_buffer[pos_temp_buffer_idx.get_hash() as usize] = data_pos as i32;
        self.search_pos_buffer[search_pos_idx]
    }
}

const fn get_required_memory_size() -> u32 {
    WORK_SIZE as u32
}

use std::iter;

// Finds the longest common prefix
//
// https://users.rust-lang.org/t/how-to-find-common-prefix-of-two-byte-slices-effectively/25815/3
// https://github.com/memorysafety/zlib-rs/blob/main/zlib-rs/src/deflate/compare256.rs#L108
fn mismatch(xs: &[u8], ys: &[u8]) -> usize {
    mismatch_chunks::<128>(xs, ys)
}

fn mismatch_chunks<const N: usize>(xs: &[u8], ys: &[u8]) -> usize {
    let off = iter::zip(xs.chunks_exact(N), ys.chunks_exact(N))
        .take_while(|(x, y)| x == y)
        .count()
        * N;
    off + iter::zip(&xs[off..], &ys[off..])
        .take_while(|(x, y)| x == y)
        .count()
}

fn search(
    match_: &mut Match,
    search_pos_immut: i32,
    data_pos: u32,
    data_buffer_size: i32,
    work: &Work,
) -> bool {
    let mut search_pos = search_pos_immut;
    let cmp2 = &work.data_buffer[data_pos as usize..DATA_BUFFER_SIZE];

    assert!(search_pos >= 0);

    // Maximum backtrace amount, either the maximum backtrace for a SZS token (12
    // bits => 0x1000) or how much actually precedes the buffer.
    let search_pos_min = if data_pos > SEARCH_WINDOW_SIZE as u32 {
        data_pos as i32 - SEARCH_WINDOW_SIZE as i32
    } else {
        -1
    };

    let mut best_match_len = 2;
    match_.len = best_match_len;

    // There are no deletions from the hash chains; the algorithm
    //  simply discards matches that are too old.
    if search_pos > data_pos as i32 || data_pos as i32 > search_pos + SEARCH_WINDOW_SIZE as i32 {
        return false;
    }

    // The compressor searches the hash chains starting with the most recent
    //  strings, to favor small distances and thus take advantage of the
    //  Huffman encoding.  The hash chains are singly linked.
    //
    //  To avoid a worst-case situation, very long hash
    //  chains are arbitrarily truncated at a certain length, determined by a
    //  run-time parameter [SEARCH_WINDOW_SIZE in this case].
    for _chain_depth in 0..SEARCH_WINDOW_SIZE {
        let cmp1 = &work.data_buffer[search_pos as usize..DATA_BUFFER_SIZE];
        // Before calling some heavy vectorized comparison function on the entire
        // buffer, discard easy values:
        // - Hash conflicts (likely the first two bytes will be different)
        // - The last byte (The least likely byte to match given the longer the
        //   prefix the greater number of matches)
        //   (based on the best so far)
        let is_match_candidate = cmp1[0] == cmp2[0]
            && cmp1[1] == cmp2[1]
            && cmp1[best_match_len as usize] == cmp2[best_match_len as usize];

        if is_match_candidate {
            let mut candidate_match_len = 2;

            // Search for longest common prefix
            
            /*
            while candidate_match_len < MATCH_LEN_MAX as i32 {
            if cmp1[candidate_match_len as usize] != cmp2[candidate_match_len as usize] {
            break;
            }
            candidate_match_len += 1;
            }
            */
            let remaining_len = MATCH_LEN_MAX as usize - candidate_match_len as usize;
            let mut candidate_match_len = candidate_match_len
                + mismatch(
                    &cmp1[candidate_match_len as usize
                        ..candidate_match_len as usize + remaining_len],
                    &cmp2[candidate_match_len as usize
                        ..candidate_match_len as usize + remaining_len],
                ) as i32;
                
            /*
            */

            if candidate_match_len > best_match_len {
                match_.len = candidate_match_len;
                match_.pos = data_pos as i32 - search_pos;

                // Truncation based on the data buffer size
                if match_.len > data_buffer_size {
                    match_.len = data_buffer_size;
                }
                best_match_len = match_.len;

                // We can't do any better
                if candidate_match_len >= MATCH_LEN_MAX as i32 {
                    break;
                }
            }
        }

        search_pos = work.search_pos(search_pos as u32);
        // There are no deletions from the hash chains; the algorithm
        //  simply discards matches that are too old.
        if search_pos <= search_pos_min {
            break;
        }
    }

    best_match_len >= 3
}

fn encode(p_dst: &mut [u8], p_src: &[u8], src_size: u32, p_work: &mut [u8]) -> u32 {
    let mut work = unsafe { &mut *(p_work.as_mut_ptr() as *mut Work) };
    let mut tmp = TempBuffer::new();

    let mut search_pos = -1;
    let mut found_next_match = false;
    let mut bit = 8;

    let mut data_pos: u32;
    let mut data_buffer_size: i32;

    work.pos_temp_buffer.fill(-1);
    work.search_pos_buffer.fill(-1);

    data_pos = 0;

    let mut out_size = 0x10; // Header size
    let mut flag: u32 = 0;

    p_dst[..4].copy_from_slice(b"Yaz0");
    p_dst[4] = (src_size >> 24) as u8;
    p_dst[5] = (src_size >> 16) as u8;
    p_dst[6] = (src_size >> 8) as u8;
    p_dst[7] = (src_size >> 0) as u8;
    p_dst[8..16].fill(0);

    let mut xyz_hasher = TripletHasher::new();

    data_buffer_size = cmp::min(DATA_BUFFER_SIZE as u32, src_size) as i32;
    work.data_buffer[..data_buffer_size as usize]
        .copy_from_slice(&p_src[..data_buffer_size as usize]);

    xyz_hasher.push_back(work.data_buffer[0] as u32);
    xyz_hasher.push_back(work.data_buffer[1] as u32);

    let mut match_info = Match { len: 2, pos: 0 };
    let mut next_match = Match { len: 0, pos: 0 };

    let mut current_read_end_pos = data_buffer_size;
    let mut next_read_end_pos;

    while data_buffer_size > 0 {
        // [TODO: Potentially allow for this]
        // If speed is most important, the compressor inserts new
        //  strings in the hash table only when no match was found, or when the
        //  match is not "too long".  This degrades the compression ratio but
        //  saves time since there are both fewer insertions and fewer searches.
        if !found_next_match {
            xyz_hasher.push_back(work.data_buffer[(data_pos + 2) as usize] as u32);
            search_pos = work.append_to_hash_chain(data_pos, &xyz_hasher);
        }
        found_next_match = false;

        // If the hash chain is not empty, indicating that
        //  the sequence XYZ (or, if we are unlucky, some other 3 bytes with the
        //  same hash function value) has occurred recently, the compressor
        //  compares all strings on the XYZ hash chain with the actual input data
        //  sequence starting at the current point, and selects the longest
        //  match.
        if search_pos != -1 {
            search(
                &mut match_info,
                search_pos,
                data_pos,
                data_buffer_size,
                &work,
            );

            if match_info.len > 2 && match_info.len < MATCH_LEN_MAX as i32 {
                data_pos += 1;
                data_buffer_size -= 1;

                // To improve overall compression, the compressor optionally defers
                //  the selection of matches ("lazy matching"): after a match of
                //  length N has been found, the compressor searches for a longer
                //  match starting at the next input byte.  If it finds a longer
                //  match, it truncates the previous match to a length of one (thus
                //  producing a single literal byte) and then emits the longer match.
                //  Otherwise, it emits the original match, and, as described above,
                //  advances N bytes before continuing.
                //
                //  Run-time parameters also control this "lazy match" procedure.  If
                //  compression ratio is most important, the compressor attempts a
                //  complete second search regardless of the length of the first
                //  match.
                xyz_hasher.push_back(work.data_buffer[(data_pos + 2) as usize] as u32);
                search_pos = work.append_to_hash_chain(data_pos, &xyz_hasher);

                if search_pos != -1 {
                    search(
                        &mut next_match,
                        search_pos,
                        data_pos,
                        data_buffer_size,
                        &work,
                    );
                    if match_info.len < next_match.len {
                        match_info.len = 2;
                    }
                }
                found_next_match = true;
            }
        }

        // First, the compressor examines the hash chain for XYZ.  If the chain is
        //  empty, the compressor simply writes out X as a literal byte and advances
        //  one byte in the input.
        if match_info.len <= 2 {
            flag = (flag & 0x7f) << 1 | 1;

            tmp.put(work.data_buffer[(data_pos as i32 - found_next_match as i32) as usize]);

            if !found_next_match {
                data_pos += 1;
                data_buffer_size -= 1;
            }
        } else {
            flag = (flag & 0x7f) << 1;

            let low = (match_info.pos - 1) as u8;
            let high = ((match_info.pos - 1) >> 8) as u8;

            if match_info.len < 18 {
                tmp.put(((match_info.len - 2) << 4) as u8 | high);
                tmp.put(low);
            } else {
                tmp.put(high);
                tmp.put(low);
                tmp.put((match_info.len - 18) as u8);
            }

            data_buffer_size -= match_info.len - found_next_match as i32;
            match_info.len -= found_next_match as i32 + 1;

            // [TODO: Potentially allow for this]
            // If speed is most important, the compressor inserts new
            //  strings in the hash table only when no match was found, or when the
            //  match is not "too long".  This degrades the compression ratio but
            //  saves time since there are both fewer insertions and fewer searches.
            loop {
                data_pos += 1;

                xyz_hasher.push_back(work.data_buffer[(data_pos + 2) as usize] as u32);

                search_pos = work.append_to_hash_chain(data_pos, &xyz_hasher);
                match_info.len -= 1;
                if match_info.len == 0 {
                    break;
                }
            }

            data_pos += 1;
            found_next_match = false;
            match_info.len = 0;
        }

        bit -= 1;

        if bit == 0 {
            p_dst[out_size as usize] = flag as u8;
            out_size += 1;

            p_dst[out_size as usize..out_size as usize + tmp.temp_size]
                .copy_from_slice(&tmp.temp_buffer[..tmp.temp_size]);
            out_size += tmp.temp_size as u32;

            flag = 0;
            tmp.temp_size = 0;
            bit = 8;
        }

        if data_buffer_size < (MATCH_LEN_MAX + 2) as i32 {
            // Refill the data buffer
            let copy_pos = data_pos as i32 - SEARCH_WINDOW_SIZE as i32;
            let copy_size = DATA_BUFFER_SIZE as i32 - copy_pos;

            next_read_end_pos = current_read_end_pos;

            if data_pos >= (SEARCH_WINDOW_SIZE + 14 * MATCH_LEN_MAX) as u32 {
                work.data_buffer
                    .copy_within(copy_pos as usize..DATA_BUFFER_SIZE, 0);

                let mut next_read_size = DATA_BUFFER_SIZE as i32 - copy_size;
                next_read_end_pos = current_read_end_pos + next_read_size;
                if src_size < next_read_end_pos as u32 {
                    next_read_size = src_size as i32 - current_read_end_pos;
                    next_read_end_pos = src_size as i32;
                }
                work.data_buffer[copy_size as usize..copy_size as usize + next_read_size as usize]
                    .copy_from_slice(
                        &p_src[current_read_end_pos as usize
                            ..current_read_end_pos as usize + next_read_size as usize],
                    );
                data_buffer_size += next_read_size;
                data_pos -= copy_pos as u32;

                for i in 0..POS_TEMP_BUFFER_NUM {
                    work.pos_temp_buffer[i] = if work.pos_temp_buffer[i] >= copy_pos {
                        work.pos_temp_buffer[i] - copy_pos
                    } else {
                        -1
                    };
                }

                for i in 0..SEARCH_POS_BUFFER_NUM {
                    work.search_pos_buffer[i] = if work.search_pos_buffer[i] >= copy_pos {
                        work.search_pos_buffer[i] - copy_pos
                    } else {
                        -1
                    };
                }
            }

            current_read_end_pos = next_read_end_pos;
        }
    }

    // Flush remaining bits
    flag = if (bit & 0x3f) == 8 {
        0
    } else {
        flag << (bit & 0x3f)
    };

    p_dst[out_size as usize] = flag as u8;
    out_size += 1;
    p_dst[out_size as usize..out_size as usize + tmp.temp_size]
        .copy_from_slice(&tmp.temp_buffer[..tmp.temp_size]);
    out_size += tmp.temp_size as u32;
    tmp.temp_size = 0;

    out_size
}

pub fn compress_mk8(src: &[u8], dst: &mut [u8]) -> u32 {
    let mut work = vec![0u8; get_required_memory_size() as usize];
    encode(dst, src, src.len() as u32, &mut work)
}
