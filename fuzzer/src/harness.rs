use crate::ast::Grammar;
use crate::generator::Generator;
use rand::rng;
use std::io::Write;
use std::sync::{
    atomic::{AtomicUsize, Ordering},
    Arc,
};
use tempfile::NamedTempFile;
use uuid::Uuid;

#[derive(Debug)]
pub struct FuzzStats {
    total_runs: AtomicUsize,
    total_crashes: AtomicUsize,
    start_time: std::time::Instant,
}

impl FuzzStats {
    pub fn new() -> Self {
        FuzzStats {
            total_runs: AtomicUsize::new(0),
            total_crashes: AtomicUsize::new(0),
            start_time: std::time::Instant::now(),
        }
    }

    pub fn increment(&self, n: usize) {
        self.total_runs.fetch_add(n, Ordering::SeqCst);
    }

    pub fn increment_crash(&self) {
        self.total_crashes.fetch_add(1, Ordering::SeqCst);
    }

    pub fn get_total_crashes(&self) -> usize {
        self.total_crashes.load(Ordering::SeqCst)
    }

    pub fn get_total_runs(&self) -> usize {
        self.total_runs.load(Ordering::SeqCst)
    }

    pub fn reset(&self) {
        self.total_runs.store(0, Ordering::SeqCst);
        self.total_crashes.store(0, Ordering::SeqCst);
    }

    pub fn get_stats(&self) -> String {
        format!(
            "Total runs: {} | Total crashes: {} | Runs/sec: {:.2}",
            self.get_total_runs(),
            self.get_total_crashes(),
            self.get_total_runs() as f64 / self.start_time.elapsed().as_secs_f64()
        )
    }
}

fn run_cyoto_compiler(program_str: &str, cyoto_path: &str) -> Result<(), String> {
    let mut file = NamedTempFile::new().map_err(|e| e.to_string())?;
    file.write_all(program_str.as_bytes())
        .map_err(|e| e.to_string())?;
    file.flush().map_err(|e| e.to_string())?;

    let output = std::process::Command::new(cyoto_path)
        .arg(file.path())
        .output()
        .map_err(|e| e.to_string())?;

    if output.status.code() == Some(139) {
        return Err("Segmentation fault detected".to_string());
    }

    if let Some(code) = output.status.code() {
        if code > 1 {
            return Err(format!(
                "Compiler exited with code {}: {}",
                code,
                String::from_utf8_lossy(&output.stderr)
            ));
        }
    }

    let mut last_line = String::new();
    if let Some(last) = String::from_utf8_lossy(&output.stderr).lines().last() {
        last_line = last.to_string();
    }

    if last_line.contains("bad any_cast") || last_line.contains("unordered_map::at") {
        return Err(format!("crash: {}", last_line));
    }

    Ok(())
}

pub fn fuzz_worker(
    thread_id: usize,
    grammar: Arc<Grammar>,
    stats: Arc<FuzzStats>,
    max_depth: usize,
    num_iters: usize,
    output_dir: Arc<String>,
) {
    let mut rng = rng();
    let mut generator = Generator::new(&grammar, &mut rng, max_depth);
    let mut curr_iters = 0;
    const BATCH: usize = 10;
    loop {
        if curr_iters >= num_iters {
            break;
        }

        curr_iters += 1;
        if curr_iters % BATCH == 0 {
            stats.increment(BATCH);
        }

        let gen_op = generator.generate_ast();
        match gen_op {
            Ok(_) => {}
            Err(e) => {
                eprintln!("Thread {}: Error generating AST: {}", thread_id, e);
                curr_iters -= 1;
                continue;
            }
        }

        let st = generator.stringify_op(&gen_op.unwrap().definition);

        if st.is_empty() {
            curr_iters -= 1;
            continue;
        }

        match run_cyoto_compiler(&st, "../build/cyoto") {
            Ok(_) => {}
            Err(e) => {
                eprintln!("Thread {}: Error running Cyoto compiler: {}", thread_id, e);
                stats.increment_crash();
                let id = Uuid::new_v4();
                let mut file =
                    std::fs::File::create(format!("{}/crash_{}.txt", output_dir, id.to_string()))
                        .expect("Failed to create crash file");
                file.write_all(st.as_bytes())
                    .expect("Failed to write to crash file");
                continue;
            }
        }
    }
}
