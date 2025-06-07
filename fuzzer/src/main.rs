use std::sync::Arc;
use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering;
use std::vec;

use fuzzer::ast::Grammar;

use clap::Parser;

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// Path to the lexer grammar file
    #[arg(short, long, default_value = "../grammar/KyotoLexer.g4")]
    lexer: String,

    /// Path to the parser grammar file
    #[arg(short, long, default_value = "../grammar/KyotoParser.g4")]
    parser: String,

    /// Maximum depth of the AST to generate
    #[arg(short, long, default_value = "10")]
    max_depth: usize,

    /// Number of iterations (per thread) to run the fuzzer
    #[arg(short, long, default_value_t = 100)]
    iters: usize,

    /// Number of threads to use for fuzzing. If set to 0, it will use the number of available CPUs.
    #[arg(short, long, default_value = "0")]
    threads: usize,

    /// Output directory for crashes
    #[arg(short, long, default_value = "crashes")]
    output: String,
}

fn get_garmmar(lexer: &String, parser: &String) -> Grammar {
    println!("[*] Loading grammar from {} and {}", lexer, parser);
    let g: Grammar = Grammar::new(lexer, parser).expect("Failed to create grammar");
    g
}

fn stats_worker(stats: Arc<fuzzer::harness::FuzzStats>, running: Arc<AtomicBool>) {
    loop {
        if !running.load(Ordering::Acquire) {
            break;
        }
        std::thread::sleep(std::time::Duration::from_secs(2));
        println!("Current stats: {}", stats.get_stats());
    }
}

fn main() {
    let args = Args::parse();

    let grammar = get_garmmar(&args.lexer, &args.parser);
    let shared_grammar = Arc::new(grammar);
    let shared_stats = Arc::new(fuzzer::harness::FuzzStats::new());

    std::fs::create_dir_all(&args.output).expect("Failed to create output directory");
    let output_dir = Arc::new(args.output);

    let max_depth = args.max_depth;
    let num_iters = args.iters;
    let num_threads = if args.threads > 0 {
        args.threads
    } else {
        num_cpus::get()
    };

    let mut handles = vec![];
    println!("Starting {} threads for fuzzing", num_threads);

    for i in 0..num_threads - 1 {
        let grammar_clone = Arc::clone(&shared_grammar);
        let stats_clone = Arc::clone(&shared_stats);
        let outdir_clone = Arc::clone(&output_dir);

        handles.push(std::thread::spawn(move || {
            fuzzer::harness::fuzz_worker(
                i,
                grammar_clone,
                stats_clone,
                max_depth,
                num_iters,
                outdir_clone,
            );
        }));
    }

    let running_flag = Arc::new(AtomicBool::new(true));
    let stats_running_clone = Arc::clone(&running_flag);
    let stats_handle = std::thread::spawn(move || {
        stats_worker(shared_stats, stats_running_clone);
    });

    for handle in handles {
        handle.join().expect("Thread panicked");
    }

    running_flag.store(false, Ordering::Release);
    stats_handle.join().expect("Stats thread panicked");
}
