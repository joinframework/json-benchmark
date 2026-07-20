#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${1:-$PROJECT_DIR/build}"
RESULTS_DIR="$PROJECT_DIR/results"
PLOTS_DIR="$PROJECT_DIR/plots"

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel $(( $(nproc) > 1 ? $(nproc) - 1 : 1 ))

mkdir -p "$RESULTS_DIR"

echo ""
echo "=== Conformance ==="
"$BUILD_DIR/benchmarks/conformance/conformance" --output "$RESULTS_DIR/conformance.json"

REPETITIONS="${REPETITIONS:-10}"

echo ""
echo "=== Running benchmarks ==="
for bench in bench_parse bench_stringify bench_prettify bench_statistics; do
    name="${bench#bench_}"
    echo "  $name..."
    "$BUILD_DIR/benchmarks/performance/$bench" \
        --benchmark_format=json \
        --benchmark_out="$RESULTS_DIR/$name.json" \
        --benchmark_repetitions="$REPETITIONS" \
        --benchmark_min_warmup_time=0.5 \
        --benchmark_report_aggregates_only=true \
        --benchmark_counters_tabular=true
done

echo ""
echo "=== Generating plots ==="
rm -f "$PLOTS_DIR"/*.png
python3 "$PLOTS_DIR/plot.py" "$RESULTS_DIR"/*.json
