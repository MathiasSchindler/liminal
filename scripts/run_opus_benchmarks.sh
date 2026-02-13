#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-$REPO_ROOT/build-opus-bench}"
RUNS="${2:-5}"

LIMINAL_BIN="$BUILD_DIR/src/liminal"
if [[ ! -x "$LIMINAL_BIN" ]]; then
  echo "error: liminal binary not found at $LIMINAL_BIN"
  echo "hint: build with: cmake -S $REPO_ROOT -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release -DENABLE_OPUS_BENCHMARK_TESTS=ON && cmake --build $BUILD_DIR"
  exit 1
fi

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || [[ "$RUNS" -lt 1 ]]; then
  echo "error: runs must be a positive integer (got '$RUNS')"
  exit 1
fi

PROGRAMS=(
  "examples/opus/t29_bench_int_hotloop.lim"
  "examples/opus/t30_bench_function_calls.lim"
  "examples/opus/c12_bench_event_aggregation.lim"
  "examples/opus/c13_bench_rule_routing.lim"
)

printf "Running Opus benchmarks with %s (runs=%s)\n\n" "$LIMINAL_BIN" "$RUNS"
printf "%-40s %10s %10s %10s\n" "Program" "min(s)" "median(s)" "max(s)"
printf "%-40s %10s %10s %10s\n" "----------------------------------------" "----------" "----------" "----------"

for prog in "${PROGRAMS[@]}"; do
  times=()
  for ((i=1; i<=RUNS; i++)); do
    t="$({ /usr/bin/time -f '%e' "$LIMINAL_BIN" run "$REPO_ROOT/$prog" >/dev/null; } 2>&1)"
    times+=("$t")
  done

  sorted="$(printf '%s\n' "${times[@]}" | sort -n)"
  min_time="$(printf '%s\n' "$sorted" | head -n1)"
  max_time="$(printf '%s\n' "$sorted" | tail -n1)"
  median_index=$(( (RUNS + 1) / 2 ))
  median_time="$(printf '%s\n' "$sorted" | sed -n "${median_index}p")"

  printf "%-40s %10s %10s %10s\n" "$(basename "$prog" .lim)" "$min_time" "$median_time" "$max_time"
done
