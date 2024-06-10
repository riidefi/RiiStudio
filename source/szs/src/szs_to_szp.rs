// This isn't yet benchmarked against the C++ implementation, so for now both stay.
// That said, SZP (YAY0) is not a common path.

use std::convert::TryInto;
use std::vec::Vec;

#[derive(Default)]
struct SZPData {
    decompressed_size: u32,
    byte_chunk_and_count_modifiers_table: Vec<u8>,
    link_table: Vec<u16>,
    bitstream: Vec<bool>,
}

fn create_u32_stream(bitstream: Vec<bool>) -> Vec<u32> {
    let mut u32stream = Vec::new();
    let mut counter = 0;
    let mut tmp = 0;
    for bit in bitstream {
        if bit {
            tmp |= 0x80000000 >> counter;
        }
        counter += 1;
        if counter == 32 {
            counter = 0;
            u32stream.push(tmp);
            tmp = 0;
        }
    }
    if counter > 0 {
        u32stream.push(tmp);
    }
    u32stream
}

fn write_szp_data(szp: &SZPData) -> Vec<u8> {
    let u32stream = create_u32_stream(szp.bitstream.clone());

    let u32_data_offset = 16; // Header size
    let u16_data_offset = u32_data_offset + u32stream.len() * 4;
    let u8_data_offset = u16_data_offset + szp.link_table.len() * 2;
    let file_end = u8_data_offset + szp.byte_chunk_and_count_modifiers_table.len();

    let mut result = vec![0u8; file_end];
    result[0] = b'Y';
    result[1] = b'a';
    result[2] = b'y';
    result[3] = b'0';
    result[4..8].copy_from_slice(&szp.decompressed_size.to_be_bytes());
    result[8..12].copy_from_slice(&(u16_data_offset as u32).to_be_bytes());
    result[12..16].copy_from_slice(&(u8_data_offset as u32).to_be_bytes());

    let mut it = u32_data_offset;
    for u in u32stream {
        result[it..it + 4].copy_from_slice(&u.to_be_bytes());
        it += 4;
    }

    it = u16_data_offset;
    for &u in &szp.link_table {
        result[it..it + 2].copy_from_slice(&u.to_be_bytes());
        it += 2;
    }

    result[u8_data_offset..].copy_from_slice(&szp.byte_chunk_and_count_modifiers_table);
    result
}

fn get_expanded_size_copy(src: &[u8]) -> Result<u32, String> {
    if src.len() < 8 {
        return Err("File too small to be a YAZ0 file".to_string());
    }

    if !(src[0] == b'Y' && src[1] == b'a' && src[2] == b'z' && src[3] == b'0') {
        return Err("Data is not a YAZ0 file".to_string());
    }

    Ok(u32::from_be_bytes(src[4..8].try_into().unwrap()))
}

fn szp_to_szp_upper_bound_size(szs_size: u32) -> u32 {
    szs_size + 3
}

fn szs_to_szp(src: &[u8]) -> Result<Vec<u8>, String> {
    let expanded_size = get_expanded_size_copy(src)?;

    let mut result = SZPData {
        decompressed_size: expanded_size,
        ..Default::default()
    };

    let mut in_position = 0x10;
    let mut out_position = 0;

    let take8 = |in_position: &mut usize| -> u8 {
        let byte = src[*in_position];
        *in_position += 1;
        byte
    };

    let take16 = |in_position: &mut usize| -> u16 {
        let high = src[*in_position] as u16;
        *in_position += 1;
        let low = src[*in_position] as u16;
        *in_position += 1;
        (high << 8) | low
    };

    while in_position < src.len() && out_position < result.decompressed_size as usize {
        let header = take8(&mut in_position);

        for i in 0..8 {
            if in_position >= src.len() || out_position >= result.decompressed_size as usize {
                break;
            }
            let raw = header & (1 << (7 - i)) != 0;
            result.bitstream.push(raw);
            if raw {
                result
                    .byte_chunk_and_count_modifiers_table
                    .push(take8(&mut in_position));
                out_position += 1;
                continue;
            }

            let group = take16(&mut in_position);
            let reverse = (group & 0xfff) + 1;
            let g_size = (group >> 12) as usize;
            result.link_table.push(group);

            let mut next = 0;
            if g_size == 0 {
                next = take8(&mut in_position);
                result.byte_chunk_and_count_modifiers_table.push(next);
            }
            let size = if g_size != 0 {
                g_size + 2
            } else {
                next as usize + 18
            };

            for _ in 0..size {
                out_position += 1;
            }
        }
    }

    Ok(write_szp_data(&result))
}

pub fn szs_to_szp_c(dst: &mut [u8], src: &[u8]) -> Result<u32, String> {
    let szp = szs_to_szp(src)?;
    if dst.len() < szp.len() {
        return Err(format!(
            "Dst buffer was insufficiently sized! Expected {} instead was given {} bytes to work with.",
            szp.len(),
            dst.len()
        ));
    }
    dst[..szp.len()].copy_from_slice(&szp);
    Ok(szp.len() as u32)
}
