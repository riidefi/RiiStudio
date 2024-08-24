use anyhow;
use clap::{Arg, Command};
use std::fs;
use std::str::FromStr;
use std::time::Instant;
use szs;
use szs::{encode, EncodeAlgo, Error};

fn to_encode_algo(s: &str) -> anyhow::Result<EncodeAlgo> {
    match s {
        "worst-case-encoding" => Ok(EncodeAlgo::WorstCaseEncoding),
        "mkw" => Ok(EncodeAlgo::MKW_Rust),
        "mkw (C++)" => Ok(EncodeAlgo::MKW_ReferenceCVersion),
        "mkw-sp" => Ok(EncodeAlgo::MkwSp),
        "ctgp" => Ok(EncodeAlgo::CTGP),
        "haroohie" => Ok(EncodeAlgo::Haroohie),
        "ct-lib" => Ok(EncodeAlgo::CTLib),
        "lib-yaz0" => Ok(EncodeAlgo::LibYaz0_ReferenceCVersion),
        "lib-yaz0-rustlibc" => Ok(EncodeAlgo::LibYaz0_RustLibc),
        "lib-yaz0-rustmemchr" => Ok(EncodeAlgo::LibYaz0_RustMemchr),
        "mk8" => Ok(EncodeAlgo::MK8),
        "mk8-rust" => Ok(EncodeAlgo::MK8_Rust),
        _ => Err(anyhow::Error::msg(format!("Invalid Yaz0 algorithm: '{}'", s))),
    }
}

fn main() {
    let matches = Command::new("szs-encoder")
        .version("1.0")
        .author("Your Name <your.email@example.com>")
        .about("Encodes data using the specified encoding algorithm")
        .arg(
            Arg::new("input")
                .short('i')
                .long("input")
                .value_name("FILE")
                .required(true),
        )
        .arg(
            Arg::new("output")
                .short('o')
                .long("output")
                .value_name("FILE")
                .required(true),
        )
        .arg(
            Arg::new("algorithm")
                .short('a')
                .long("algorithm")
                .value_name("ALGO")
                .required(true),
        )
        .get_matches();

    let input_file = matches.get_one::<String>("input").unwrap();
    let output_file = matches.get_one::<String>("output").unwrap();
    let algorithm_str = matches.get_one::<String>("algorithm").unwrap();

    let algorithm: EncodeAlgo = match to_encode_algo(algorithm_str) {
        Ok(algo) => algo,
        Err(_) => {
            eprintln!("Error: Invalid encoding algorithm '{}'", algorithm_str);
            std::process::exit(1);
        }
    };

    let input_data = match fs::read(input_file) {
        Ok(data) => data,
        Err(err) => {
            eprintln!("Error reading input file: {}", err);
            std::process::exit(1);
        }
    };

    // Measure the time taken for the compression
    let start_time = Instant::now();
    let encoded_data = match szs::encode(&input_data, algorithm) {
        Ok(data) => data,
        Err(err) => {
            eprintln!("Error encoding data: {:?}", err);
            std::process::exit(1);
        }
    };
    let duration = start_time.elapsed();
    let compression_rate = encoded_data.len() as f64 / input_data.len() as f64;

    if let Err(err) = fs::write(output_file, &encoded_data) {
        eprintln!("Error writing output file: {}", err);
        std::process::exit(1);
    }

    println!("Data encoded successfully.");
    println!("Time taken for compression: {:?}", duration);
    println!("Compression rate: {:.2}", compression_rate);
}
