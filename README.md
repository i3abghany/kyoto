<h1><img src="kyoto.svg" alt="Kyoto logo" height="50" valign="middle"> Kyoto</h1>

This is the repository for the Kyoto Programming Language and its compiler, Cyoto. Kyoto is a statically typed, compiled language that is still in early design stages. The compiler is written in C++ and utilizes LLVM for analysis and code generation.

## Building

Cyoto requires the following dependencies:

- LLVM >= 20
- CMake >= 3.20
- Java >= 8
- antlr == 4.13.2
- Boost >= 1.74.0
- GCC >= 11 (for C++23 support)

To install the dependencies on Ubuntu, run the following commands:

```bash
$ sudo apt-get install llvm-20-dev cmake default-jre libboost-all-dev gcc-11 g++-11 libfmt-dev libzstd-dev
```

Antlr 4.13.2 can be installed from the official website's mirror as follows:

```bash
$ wget https://www.antlr.org/download/antlr-4.13.2-complete.jar -O /path/to/install
$ chmod +x /path/to/install/antlr-4.13.2-complete.jar
$ export PATH="/path/to/install:$PATH"
```

If llvm-20 is not available in the package manager, it can be installed using the script provided on the official website as follows:

```bash
$ wget https://apt.llvm.org/llvm.sh
$ chmod +x llvm.sh
$ sudo ./llvm.sh 20 all
```

To build the compiler, run the following commands:

```bash
$ mkdir build
$ cd build
$ cmake ..
$ make cyoto -j$(nproc)
```

## Usage

To compile a Kyoto source file, run the following command:

```bash
$ ./cyoto --help

Usage: ./cyoto [OPTIONS] INPUT_FILE
The Kyoto Programming Language Compiler:
  -h [ --help ]                 Print this help message
  -r [ --run ]                  Run the program in `lli` after compilation
  -o [ --output ] arg (=out.ll) Output file for the LLVM IR
```

# License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
