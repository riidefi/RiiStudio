use std::vec;

pub fn collate_buffers(buffers: &[Vec<u8>]) -> Vec<u8> {
    let mut result = Vec::new();

    // Helper function to write a u32 value at a specific position in big endian format
    let write_u32_at = |u: u32, pos: usize, result: &mut Vec<u8>| {
        result[pos] = (u >> 24) as u8;
        result[pos + 1] = (u >> 16) as u8;
        result[pos + 2] = (u >> 8) as u8;
        result[pos + 3] = u as u8;
    };

    let mut size = 16 + 8 * buffers.len() as u32;
    let mut buffer_cursor = round_up(size, 64);
    for buf in buffers {
        size = round_up(size, 64);
        size += buf.len() as u32;
    }
    size = round_up(size, 64);
    result.resize(size as usize, 0);

    write_u32_at(
        ('R' as u32) << 24 | ('B' as u32) << 16 | ('U' as u32) << 8 | ('F' as u32) << 0,
        0,
        &mut result,
    );
    write_u32_at(100, 4, &mut result);
    write_u32_at(buffers.len() as u32, 8, &mut result);
    write_u32_at(size, 12, &mut result);

    let mut cursor = 16;
    for buf in buffers {
        write_u32_at(buffer_cursor, cursor, &mut result);
        write_u32_at(buf.len() as u32, cursor + 4, &mut result);
        cursor += 8;
        buffer_cursor += buf.len() as u32;
        buffer_cursor = round_up(buffer_cursor, 64);
    }

    assert!(buffer_cursor == size);

    buffer_cursor = round_up(16 + 8 * buffers.len() as u32, 64);
    for buf in buffers {
        result[buffer_cursor as usize..(buffer_cursor as usize + buf.len())].copy_from_slice(buf);
        buffer_cursor += buf.len() as u32;
        buffer_cursor = round_up(buffer_cursor, 64);
    }

    result
}

// Helper function to round up to the nearest multiple of alignment
fn round_up(value: u32, alignment: u32) -> u32 {
    (value + alignment - 1) & !(alignment - 1)
}

// Function to read the buffer back into a Vec<Vec<u8>>
pub fn read_buffer(data: &[u8]) -> Vec<Vec<u8>> {
    // Helper function to read a u32 value from a specific position in big endian format
    let read_u32_at = |data: &[u8], pos: usize| -> u32 {
        ((data[pos] as u32) << 24)
            | ((data[pos + 1] as u32) << 16)
            | ((data[pos + 2] as u32) << 8)
            | (data[pos + 3] as u32)
    };

    let magic = read_u32_at(data, 0);
    let version = read_u32_at(data, 4);
    let num_buffers = read_u32_at(data, 8) as usize;
    let file_size = read_u32_at(data, 12);

    assert!(
        magic == (('R' as u32) << 24 | ('B' as u32) << 16 | ('U' as u32) << 8 | ('F' as u32) << 0)
    );
    assert!(version == 100);
    assert!(file_size == data.len() as u32);

    let mut buffers = Vec::with_capacity(num_buffers);
    let mut cursor = 16;
    for _ in 0..num_buffers {
        let buffer_offset = read_u32_at(data, cursor) as usize;
        let buffer_size = read_u32_at(data, cursor + 4) as usize;
        cursor += 8;

        let buffer = data[buffer_offset..buffer_offset + buffer_size].to_vec();
        buffers.push(buffer);
    }

    buffers
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn buffer_code() {
        // Example usage
        let buffers = vec![vec![1, 2, 3], vec![4, 5, 6]];
        let collated = collate_buffers(&buffers);
        let read_buffers = read_buffer(&collated);

        assert_eq!(buffers, read_buffers);
        println!("Buffers successfully collated and read back!");
    }
}
