# Kyoto

This is the repository for the Kyoto Compiler for the Kyoto Programming Language. Kyoto is a statically typed, compiled language that is still in early design stages. The compiler is written in C++ and utilizes LLVM for analysis and code generation.

## Building

The Kyoto Compiler requires the following dependencies:

- LLVM >= 19
- CMake >= 3.20
- Java >= 8
- antlr == 4.13.2
- Boost >= 1.74.0
- GCC >= 11 (for C++23 support)

To install the dependencies on Ubuntu, run the following commands:

```bash
$ sudo apt-get install llvm-19-dev cmake default-jre libboost1.74-all-dev gcc-11 g++-11 libfmt-dev
```

Antlr 4.13.2 can be installed from the official website's mirror as follows:

```bash
$ wget https://www.antlr.org/download/antlr-4.13.2-complete.jar -O /path/to/install
$ export PATH="/path/to/install:$PATH"
```

To build the compiler, run the following commands:

```bash
$ mkdir build
$ cd build
$ cmake ..
$ make kyoto -j$(nproc)
```

## Usage

To compile a Kyoto source file, run the following command:

```bash
$ ./kyoto --help

Usage: ./kyoto [OPTIONS] INPUT_FILE
The Kyoto Programming Language Compiler:
  -h [ --help ]                 Print this help message
  -r [ --run ]                  Run the program in `lli` after compilation
  -o [ --output ] arg (=out.ll) Output file for the LLVM IR
```
