# Build Instructions

## Prerequisites
- CMake >= 3.16
- GCC or Clang

## Configure & Build
Debug build with sanitizers (default):
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Release build:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Sanitizers
Enabled by default in Debug builds. Disable via:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=OFF
```

## Coverage
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build
(cd build && ctest)
# generate report (example using gcovr)
gcovr -r . --xml -o coverage.xml --html -o coverage.html
```

## Compile Commands
```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```
`build/compile_commands.json` can be symlinked for editors.
