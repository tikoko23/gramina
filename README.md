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

# Compiling
A gramina file can be compiled using the `test` binary.
```bash
./test <filename>
```

This binary prints lexer, parser and compiler information and generates an `out.ll` file.
This file can be used with an LLVM frontend (such as clang):
```bash
clang -o test_executable ./out.ll host.c
```

### Minimal Example
```bash
# Compilation
./test test.lawn
clang -o sum ./out.ll ./host.c
./sum
```

```c
// host.c
#include <stdio.h>
#include <stdint.h>

int32_t sum(int32_t a, int32_t b);

int main(void) {
    int a = 10;
    int b = 13;
    int result = sum(a, b);

    printf("%d\n", result);

    return 0;
}

```

```lawn
// test.lawn
fn sum(int a, int b) -> int {
    return a + b;
}
```

# Documentation
Documentation of the language and the compiler frontend is currently lacking. Since the language is still in very early stages, significant changes are expected, thus documentation is not the top priority. 

# Codebase
The gramina codebase uses header generation to provide namespace-like functionality. `CMakeLists.txt` creates an appropriate target to call related files from `scripts/*.py`.
