# CI Instructions

GitHub Actions workflow: `.github/workflows/ci.yml`

### Matrix
- Compilers: GCC, Clang
- Configurations: Debug (sanitizers), Release

### Steps
1. Checkout
2. Install deps (cmake, gcc/clang, gcovr)
3. Configure & build Debug with sanitizers
4. Run `ctest --output-on-failure`
5. Configure & build Release
6. Optional coverage: `ENABLE_COVERAGE=ON` + `gcovr`

### Local CI Simulation
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
(cd build && ctest --output-on-failure)
cmake -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel
```
