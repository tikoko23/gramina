# Disclaimer
> Gramina is currently in a very early stage.
> Expect lots of significant and breaking changes.
> Wide support is not the primary focus for the current state of the project.
> Use at your own risk!

# Introduction
Gramina is a low-level programming language aimed to provide simple tools and ways to expand upon them.

## Windows / MacOS Support
Gramina is developed, built and tested on GNU/Linux.
Support for Windows and MacOS is considered, and will be worked on in the future.

# Building
Ensure `livLLVM-19.so` is available on the system library path.
The CMake configuration defines the following targets:
- `gracommon` (static library)
- `graparse` (static library)
- `gracompile` (static library)
- `test` (command line utility)

```bash
cd /path/to/gramina/source

mkdir build
cd build

cmake ..
cmake --build .
```

# Usage
A gramina file can be compiled using the `test` binary.
```bash
./test <filename>
```

This binary prints lexer, parser and compiler information and generates an `out.ll` file.
This file can be used with an LLVM frontend (such as clang):
```bash
clang -o test_executable ./out.ll host.c
```

## Running examples
An example consist of a `.lawn` and a `.c` file, with the same basename. Source code for the examples can be found in `examples/`.
To run an example, ensure `clang` is in your `PATH` environment variable.

```bash
# Creates `examples/<name>.out` and `examples/<name>.ll`
scripts/run_example.sh <name>
```

To clean the generated binaries and IR inside `examples/`, run the following script:
```bash
scripts/clean_examples.sh
```

# Documentation
Documentation of the language and the compiler frontend is currently lacking. Since the language is still in very early stages, significant changes are expected, thus documentation is not the top priority.

# Codebase
The gramina codebase uses header generation to provide namespace-like functionality. `CMakeLists.txt` creates an appropriate which calls related files from `scripts/*.py`.
