use criterion::{black_box, criterion_group, criterion_main, Criterion};
use std::fs::File;
use std::io::Read;

use szs;

pub fn deinterlace_benchmark(c: &mut Criterion) {
    /*
    let mut file = File::open("8-43.szs").expect("File not found");
    let mut data = Vec::new();
    file.read_to_end(&mut data).expect("Error reading file");
    c.bench_function("deinterlace without C version", |b| {
        b.iter(|| {
            szs::deinterlace(black_box(&data) /*, black_box(false)*/).expect("Deinterlace failed")
        })
    });

    // C version was slower so no reason to make available

    c.bench_function("deinterlace with C version", |b| {
        b.iter(|| {
            szs::deinterlace(black_box(&data), black_box(true)).expect("Deinterlace failed")
        })
    });
    */
}

criterion_group!(benches, deinterlace_benchmark);
criterion_main!(benches);
