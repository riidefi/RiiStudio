use anyhow::Result;
use argp::FromArgs;
use cli_table::format::{Border, HorizontalLine, Justify, Separator, VerticalLine};
use cli_table::{print_stdout, Cell, Table};
use std::fs;
use std::path::PathBuf;
use std::time::{Duration, Instant};
use szs::EncodeAlgo;
use tracing::info;
use tracing_subscriber::EnvFilter;

#[derive(FromArgs)]
#[argp(description = "SZS benchmark")]
struct Args {
    /// input file
    #[argp(positional)]
    input: PathBuf,
}

pub const SZS_ALGOS: [EncodeAlgo; 8] = [
    EncodeAlgo::WorstCaseEncoding,
    EncodeAlgo::MKW,
    EncodeAlgo::MkwSp,
    EncodeAlgo::CTGP,
    EncodeAlgo::Haroohie,
    EncodeAlgo::CTLib,
    EncodeAlgo::MK8,
    EncodeAlgo::LibYaz0,
];

pub fn szs_algo_from_str(s: &str) -> Result<EncodeAlgo> {
    match s {
        "worst-case-encoding" => Ok(EncodeAlgo::WorstCaseEncoding),
        "mkw" => Ok(EncodeAlgo::MKW),
        "mkw-sp" => Ok(EncodeAlgo::MkwSp),
        "ctgp" => Ok(EncodeAlgo::CTGP),
        "haroohie" => Ok(EncodeAlgo::Haroohie),
        "ct-lib" => Ok(EncodeAlgo::CTLib),
        "lib-yaz0" => Ok(EncodeAlgo::LibYaz0),
        "mk8" => Ok(EncodeAlgo::MK8),
        _ => Err(anyhow::Error::msg(format!(
            "Invalid Yaz0 algorithm: '{}'",
            s
        ))),
    }
}

pub fn szs_algo_to_str(algo: EncodeAlgo) -> &'static str {
    match algo {
        EncodeAlgo::WorstCaseEncoding => "worst-case-encoding",
        EncodeAlgo::MKW => "mkw",
        EncodeAlgo::MkwSp => "mkw-sp",
        EncodeAlgo::CTGP => "ctgp",
        EncodeAlgo::Haroohie => "haroohie",
        EncodeAlgo::CTLib => "ct-lib",
        EncodeAlgo::LibYaz0 => "lib-yaz0",
        EncodeAlgo::MK8 => "mk8",
        // _ => "invalid",
    }
}

struct BenchmarkResult {
    algo: EncodeAlgo,
    avg_time: Duration,
    ratio: f64,
    size: usize,
}

fn main() {
    tracing_subscriber::fmt()
        .with_env_filter(
            EnvFilter::builder()
                .with_default_directive(tracing::Level::INFO.into())
                .from_env_lossy(),
        )
        .init();
    let args: Args = argp::parse_args_or_exit(argp::DEFAULT);
    let data = fs::read(&args.input).expect("Failed to read file");

    const RUNS: usize = 3;
    let mut results = vec![];
    for algo in SZS_ALGOS {
        let mut times = vec![];
        let mut size = 0;
        for n in 0..RUNS {
            info!(algo = %szs_algo_to_str(algo), n, "Benchmarking");
            let start = Instant::now();
            let cmp_data = szs::encode(&data, algo).expect("Failed to encode");
            size = cmp_data.len();
            let end = Instant::now();
            times.push(end - start);
        }
        let avg_time = times.iter().sum::<Duration>() / RUNS as u32;
        let ratio = size as f64 / data.len() as f64;
        results.push(BenchmarkResult {
            algo,
            avg_time,
            ratio,
            size,
        });
    }

    results.sort_by_key(|r| r.avg_time);
    let table = results
        .iter()
        .map(|r| {
            let algo = szs_algo_to_str(r.algo).cell();
            let avg_time = format!("{:.2?}s", r.avg_time.as_secs_f64())
                .cell()
                .justify(Justify::Right);
            let ratio = format!("{:.2}%", r.ratio * 100.0)
                .cell()
                .justify(Justify::Right);
            let size = format!("{:.2} MB", r.size as f64 / 1_048_576.0)
                .cell()
                .justify(Justify::Right);
            vec![algo, avg_time, ratio, size]
        })
        .table()
        .title(vec![
            "Method".cell(),
            format!("Time (Avg {} runs)", RUNS).cell(),
            format!("Compression Rate (Avg {} runs)", RUNS).cell(),
            "File Size".to_string().cell(),
        ])
        .border(
            Border::builder()
                .left(VerticalLine::new('|'))
                .right(VerticalLine::new('|'))
                .build(),
        )
        .separator(
            Separator::builder()
                .column(Some(VerticalLine::new('|')))
                .title(Some(HorizontalLine::new('|', '|', '|', '-')))
                .row(None)
                .build(),
        );
    print_stdout(table).expect("Failed to print table");
}
