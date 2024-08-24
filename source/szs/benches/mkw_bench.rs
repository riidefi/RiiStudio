use criterion::{black_box, criterion_group, criterion_main, Criterion};
use std::fs::File;
use std::io::Read;

use szs;

pub fn mkw_benches(c: &mut Criterion) {
    let mut file = File::open("../../tests/samples_szs/old_koopa_64.arc").expect("File not found");
    let mut data = Vec::new();
    file.read_to_end(&mut data).expect("Error reading file");

    c.bench_function("C encoder", |b| {
        b.iter(|| {
            szs::encode(black_box(&data), black_box(szs::EncodeAlgo::LibYaz0))
                .expect("encode failed")
        })
    });
    c.bench_function("Rust encoder (`memchr` crate)", |b| {
        b.iter(|| {
            szs::encode(
                black_box(&data),
                black_box(szs::EncodeAlgo::LibYaz0_RustMemchr),
            )
            .expect("encode failed")
        })
    });
    c.bench_function("Rust encoder (`libc` crate)", |b| {
        b.iter(|| {
            szs::encode(
                black_box(&data),
                black_box(szs::EncodeAlgo::LibYaz0_RustLibc),
            )
            .expect("encode failed")
        })
    });
}

pub fn mk8_benches2(c: &mut Criterion) {
    let mut file = File::open("../../tests/samples_szs/8-43.arc").expect("File not found");
    let mut data = Vec::new();
    file.read_to_end(&mut data).expect("Error reading file");

    
    c.bench_function("Rust encoder", |b| {
        b.iter(|| {
            szs::encode(black_box(&data), black_box(szs::EncodeAlgo::MK8_Rust))
                .expect("encode failed")
        })
    });
    c.bench_function("C encoder", |b| {
        b.iter(|| {
            szs::encode(black_box(&data), black_box(szs::EncodeAlgo::MK8)).expect("encode failed")
        })
    });
}

pub fn configure() -> Criterion {
    Criterion::default()
}
criterion_group! {
    name = benches;
    config = configure();
    targets = mk8_benches2
}
criterion_main!(benches);
