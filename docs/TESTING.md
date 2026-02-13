# Testing

## Run Tests
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
(cd build && ctest --output-on-failure)
```

## Opus smoke tests (t* and c* only)
The advanced Opus oracle programs (`o*`) are excluded from this smoke set.

Enable Opus smoke tests at configure time:
```bash
cmake -B build-opus -DCMAKE_BUILD_TYPE=Debug -DENABLE_OPUS_SMOKE_TESTS=ON
cmake --build build-opus
```

Run only Opus tests:
```bash
(cd build-opus && ctest -L opus --output-on-failure)
```

Run only atomic or combined subsets:
```bash
(cd build-opus && ctest -L opus_atomic --output-on-failure)
(cd build-opus && ctest -L opus_combined --output-on-failure)
```

## Opus benchmark-focused tests
These benchmark programs are deterministic `t`/`c` workloads intended to expose
performance bottlenecks in loops, function calls, record access, and case routing.

Enable benchmark tests:
```bash
cmake -B build-opus-bench -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_OPUS_BENCHMARK_TESTS=ON
cmake --build build-opus-bench
```

Run only benchmark tests:
```bash
(cd build-opus-bench && ctest -L opus_bench --output-on-failure)
```

Run min/median/max timing summary (5 runs per program):
```bash
./scripts/run_opus_benchmarks.sh
```

Custom build directory or run count:
```bash
./scripts/run_opus_benchmarks.sh ./build-opus-bench 7
```

For rough wall-clock comparisons across revisions:
```bash
/usr/bin/time -f '%E real, %U user, %S sys' \
  ./build-opus-bench/src/liminal run examples/opus/t29_bench_int_hotloop.lim
```

## Test Harness
- Custom C harness in `tests/test_harness.[ch]`
- Assertions: `ASSERT_TRUE`, `ASSERT_EQ_STR`, `ASSERT_CONTAINS`
- CLI tests: `liminal_cli_tests`
- Lexer tests: `liminal_lexer_tests` (golden fixtures in `tests/fixtures/`)
- Parser tests: `liminal_parser_tests` (AST snapshots in `tests/fixtures/`)
- Typecheck tests: `liminal_typecheck_tests`
- Runtime tests: `liminal_runtime_tests`
- Schema tests: `liminal_schema_tests`
- Optional fuzz target: `lexer_fuzz` (`ENABLE_FUZZING=ON`)

## Snapshots
CLI tests capture stdout and compare against expected substrings.

## CI
GitHub Actions runs the test suite on every push/PR (see `.github/workflows/ci.yml`).

## Oracles: live, record, replay
- Env:
  - `LIMINAL_ORACLE_PROVIDER` = `mock` (default) | `ollama`
  - `LIMINAL_ORACLE_MODE` = `live` (default) | `record` | `replay`
  - `LIMINAL_ORACLE_RECORDING` = path to JSONL recordings (default `oracle_recordings.jsonl`)
- Examples tests run in **replay** mode by default using `tests/recordings/examples.jsonl`
- Consult tests cover retry/hint/fallback with mock oracle
- Record new fixtures:
  ```bash
  LIMINAL_ORACLE_PROVIDER=ollama LIMINAL_ORACLE_MODE=record \
    LIMINAL_ORACLE_RECORDING=tests/recordings/examples.jsonl \
    ctest -R examples_ask_basic
  ```
- Replay fixtures:
  ```bash
  LIMINAL_ORACLE_MODE=replay LIMINAL_ORACLE_RECORDING=tests/recordings/examples.jsonl ctest -R examples
  ```
- Live tests:
  - `live_examples_*` always registered (label `live`); skipped unless `LIMINAL_LIVE=1`
  - Run: `ctest -L live` (set `LIMINAL_LIVE=1`)
  - Live responses are non-deterministic; tests assert exit code only
