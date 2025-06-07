# Cyoto Fuzzer

This is a grammar-based fuzzer for the Cyoto compiler. It's based on the antlr4 grammar for Kyoto defined in `kyoto/grammar/`.

# Usage

To use the fuzzer, the compiler has to be built and stored in `kyoto/build` as described in `kyoto/README.md`. The fuzzer can then be run with the following command:

```bash
cargo run --release -- \
    --lexer ../grammar/KyotoLexer.g4 \
    --parser ../grammar/KyotoParser.g4 \
    --iters 100 \
    --output crashes \
    --threads 0
```

# Options

```bash
Usage: fuzzer [OPTIONS]

Options:
  -l, --lexer <LEXER>          Path to the lexer grammar file [default: ../grammar/KyotoLexer.g4]
  -p, --parser <PARSER>        Path to the parser grammar file [default: ../grammar/KyotoParser.g4]
  -m, --max-depth <MAX_DEPTH>  Maximum depth of the AST to generate [default: 10]
  -i, --iters <ITERS>          Number of iterations (per thread) to run the fuzzer [default: 100]
  -t, --threads <THREADS>      Number of threads to use for fuzzing. If set to 0, it will use the number of available CPUs [default: 0]
  -o, --output <OUTPUT>        Output directory for crashes [default: crashes]
  -h, --help                   Print help
  -V, --version                Print version
```