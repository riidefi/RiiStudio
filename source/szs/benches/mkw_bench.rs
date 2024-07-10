use criterion::{black_box, criterion_group, criterion_main, Criterion};
use std::fs::File;
use std::io::Read;

use szs;

pub fn mkw_benches(c: &mut Criterion) {
    let mut file = File::open("../../tests/samples/old_koopa_64.arc").expect("File not found");
    let mut data = Vec::new();
    file.read_to_end(&mut data).expect("Error reading file");

    c.bench_function("C encoder", |b| {
        b.iter(|| {
            szs::encode(black_box(&data), black_box(szs::EncodeAlgo::LibYaz0))
                .expect("encode failed")
        })
    });
    c.bench_function("Rust encoder", |b| {
        b.iter(|| {
            szs::encode(black_box(&data), black_box(szs::EncodeAlgo::LIBYAZ0_RUST))
                .expect("encode failed")
        })
    });
}

criterion_group!(benches, mkw_benches);
criterion_main!(benches);
