# json-benchmark

Comparative benchmarks of JSON libraries in C++, measuring parse, stringify, prettify, and DOM statistics performance across three real-world datasets (Canada, CitmCatalog, Twitter). Includes a conformance suite covering parse validation, double precision, string decoding, and roundtrip correctness.

Largely inspired by [nativejson-benchmark](https://github.com/miloyip/nativejson-benchmark) by Milo Yip.

## Table of contents

- [Libraries](#libraries)
- [Datasets](#datasets)
- [Build](#build)
- [Running benchmarks](#running-benchmarks)
- [Plots](#plots)
- [What the benchmarks measure](#what-the-benchmarks-measure)
  - [Parse](#parse)
  - [Stringify](#stringify)
  - [Prettify](#prettify)
  - [Statistics](#statistics)
  - [Conformance](#conformance)
- [Notes](#notes)
- [Results](#results)
  - [Performance](#performance)
  - [Conformance](#conformance-1)
  - [Conclusion](#conclusion)
- [License](#license)

## Libraries

| Library | Parse | Stringify | Prettify | Statistics | Conformance |
|---------|-------|-----------|----------|------------|-------------|
| [Boost.JSON](https://www.boost.org/doc/libs/release/libs/json/) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [Join](https://github.com/joinframework/join) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [jsoncons](https://github.com/danielaparker/jsoncons) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [JsonCpp](https://github.com/open-source-parsers/jsoncpp) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [nlohmann/json](https://github.com/nlohmann/json) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [POCO JSON](https://pocoproject.org/) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [RapidJSON](https://github.com/Tencent/rapidjson) (auto-utf) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [RapidJSON](https://github.com/Tencent/rapidjson) (default) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [RapidJSON](https://github.com/Tencent/rapidjson) (full-prec) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [RapidJSON](https://github.com/Tencent/rapidjson) (insitu) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [RapidJSON](https://github.com/Tencent/rapidjson) (iterative) | ✓ | ✓ | ✓ | ✓ | ✓ |
| [simdjson](https://github.com/simdjson/simdjson) | ✓ | ✓ | — | ✓ | ✓ |
| [taocpp/json](https://github.com/taocpp/json) | ✓ | ✓ | ✓ | ✓ | ✓ |

All dependencies are fetched automatically via CMake FetchContent.

**simdjson** is included in parse, stringify, and statistics. Its DOM exposes `operator<<` for minified serialisation, though this is not the library's primary use case. It is excluded from prettify as it has no native indentation support.

**jsoncons** and **taocpp/json** are header-only. taocpp/json requires PEGTL (also header-only) as a mandatory dependency, fetched automatically.

RapidJSON is benchmarked in five configurations to isolate the impact of its parser options:
- **default** — standard UTF-8, fast float parsing
- **full-prec** — `kParseFullPrecisionFlag`, bitwise-accurate doubles at the cost of throughput
- **iterative** — `kParseIterativeFlag`, stack-allocated iterative descent (no system stack recursion)
- **insitu** — `kParseInsituFlag`, in-place string unescaping directly in the source buffer
- **auto-utf** — `kParseAutoUTF8Flag`, auto-detect UTF-8/16/32 encoding

## Datasets

| File | Size | Characteristics |
|------|------|-----------------|
| `canada.json` | ~2.2 MB | Geometry data — float-heavy (coordinate pairs), large arrays |
| `citm_catalog.json` | ~1.7 MB | Event catalog — deeply nested objects, mixed types |
| `twitter.json` | ~0.6 MB | Social feed — Unicode strings, many small objects |

The overall time reported per benchmark is the sum of median times across all three datasets (ms). Each individual measurement is already the median across repetitions.

## Build

Configure:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

Compile:

```bash
cmake --build build -j$(nproc)
```

## Running benchmarks

Run everything and generate plots:

```bash
bash scripts/run_bench.sh
```

Single binary:

```bash
./build/benchmarks/performance/bench_parse \
    --benchmark_format=json \
    --benchmark_out=results/parse.json \
    --benchmark_repetitions=10 \
    --benchmark_min_warmup_time=0.5 \
    --benchmark_report_aggregates_only=true
```

Conformance only:

```bash
./build/benchmarks/conformance/conformance --output results/conformance.json
```

> [!NOTE]
> Default: 10 repetitions, median is reported. Override with e.g. `REPETITIONS=3 bash scripts/run_bench.sh`.

## Plots

Install plotting dependencies:

```bash
pip install -r plots/requirements.txt
```

Generate the charts:

```bash
python3 plots/plot.py results/*.json
```

PNGs are written to `plots/`:
- `performance_parse.png` — parse (overall + per dataset) and DOM statistics
- `performance_stringify.png` — stringify (overall + per dataset)
- `performance_prettify.png` — prettify (overall + per dataset)
- `conformance.png` — overall score followed by one subplot per category (parse validation, parse double, parse string, roundtrip)

## What the benchmarks measure

### Parse

**Goal**: time to parse a JSON document into an in-memory DOM.

The benchmark calls `Adapter::parse(json)` and measures only the parsing time. DOM destruction is excluded from timing (`PauseTiming` / `ResumeTiming`): some libraries free memory as a single pool (`RapidJSON`), others call `free()` per node (`nlohmann`, `Join`). Excluding destruction matches nativejson-benchmark's convention and isolates the parsing cost.

**What is measured**: tokenisation, UTF-8 validation, number parsing, and DOM construction.

**What is not measured**: DOM traversal, serialisation, input I/O.

---

### Stringify

**Goal**: time to serialise a pre-parsed DOM back to a compact (minified) JSON string.

The document is parsed once before the timing loop; each iteration serialises the same DOM.

**What is measured**: DOM traversal and JSON text generation, including number-to-string conversion and string escaping.

---

### Prettify

**Goal**: time to serialise a DOM to indented JSON (4-space indent).

Same protocol as stringify. Measures the added cost of indentation logic on top of standard serialisation.

---

### Statistics

**Goal**: time to traverse the entire DOM and accumulate counts of each node type (objects, arrays, strings, numbers, booleans, nulls).

The document is parsed once before the timing loop. Each iteration walks every node in the tree.

**What is measured**: DOM traversal cost — cache efficiency of the library's node layout, virtual dispatch or type-switch overhead, and branch prediction under the mixed node types of real-world documents.

---

### Conformance

Four correctness categories, run once (no timing):

1. **Parse Validation** (`jsonchecker/`): accept all `pass*.json` files, reject all `fail*.json` files. 34 cases.
2. **Parse Double**: parse a JSON array containing a single number and compare the result bitwise against the expected `double`. 66 cases, sourced from nativejson-benchmark's `roundtrip/` and edge-case suite.
3. **Parse String**: parse a JSON array containing a single string and compare the decoded bytes exactly. 9 cases covering escape sequences and Unicode surrogates.
4. **Roundtrip**: parse a JSON document, stringify it, and compare the output byte-for-byte against the input. 27 cases.

## Notes

- `-march=native`: CPU-specific SIMD optimisations. Results are not portable across machines.
- Timing uses Google Benchmark's default wall-clock timer. simdjson's parse benchmark benefits from its SIMD path being warmed up by the static parser instance reused across iterations.
- `--benchmark_min_warmup_time=0.5` stabilises CPU frequency scaling before measurement.
- The median (not the mean) is reported across repetitions, making results more robust to scheduler spikes.
- Result JSONs are gitignored (`results/`); plots are committed under `plots/`.

## Results

Measured on a development machine. Re-run `scripts/run_bench.sh` to reproduce on other hardware; results are not portable across machines (`-march=native`).

### Performance

![Parse](plots/performance_parse.png)

![Stringify](plots/performance_stringify.png)

![Prettify](plots/performance_prettify.png)

**Parse**: **simdjson** leads at 6 ms, more than 1.8× faster than the next best (**RapidJSON** insitu at 12 ms). Among the general-purpose DOM libraries, **RapidJSON**'s in-situ mode leads thanks to its zero-copy string handling, followed closely by the default and iterative variants (14–15 ms) and **Boost.JSON** (16 ms). **Join** (29 ms) sits between **RapidJSON** and the slower libraries. **taocpp/json** (38 ms) is competitive. **jsoncons** (76 ms) and **nlohmann/json** (91 ms) are slower but similar; **POCO** (136 ms) and **JsonCpp** (142 ms) trail significantly.

**Stringify**: **Boost.JSON** leads at 11 ms. All **RapidJSON** variants cluster tightly at ~18 ms. **simdjson** (21 ms, minified output only) and **taocpp/json** (23 ms) follow. **Join** (23 ms) and **nlohmann/json** (24 ms) are close; **jsoncons** (27 ms) is slightly behind. **POCO** (71 ms) and **JsonCpp** (173 ms) are the slowest.

**Prettify**: **RapidJSON** variants lead at ~19 ms across all five modes. **Boost.JSON** (27 ms) follows closely. **Join** (29 ms), **nlohmann/json** (30 ms), and **taocpp/json** (30 ms) are close behind. **jsoncons** (55 ms) is slower. **JsonCpp** (180 ms) and **POCO** (217 ms) trail by a wide margin.

**Statistics**: **RapidJSON**'s five variants cluster at ~1 ms — their contiguous memory pool layout maximises cache efficiency. **Boost.JSON** (1.6 ms), **jsoncons** (1.7 ms), **simdjson** (1.7 ms), and **taocpp/json** (2.0 ms) all follow closely. **Join** (3.5 ms) and **nlohmann/json** (3.8 ms) are about 3× slower than **RapidJSON**. **JsonCpp** (17 ms) and **POCO** (19 ms) are again the outliers.

Raw throughput is only part of the picture. A parser that silently accepts malformed input, produces slightly wrong floating-point values, or mangles escape sequences can introduce subtle bugs that are hard to reproduce and trace. The conformance suite measures what matters in production: does the library correctly reject invalid JSON, parse numbers to the right `double`, decode strings exactly as the spec requires, and reproduce the original representation on roundtrip?

### Conformance

![Conformance](plots/conformance.png)

**Parse Validation**: most libraries pass all 34 cases. **JsonCpp** fails 8 (it is lenient on some constructs that the JSON spec rejects, such as comments).

**Parse Double**: the sharpest differentiator. **Join**, **jsoncons**, **nlohmann/json**, **taocpp/json**, and **RapidJSON** (`full-prec`) achieve perfect precision (66/66). **RapidJSON**'s default, iterative, insitu, and auto-utf modes all fail 18 cases — they trade precision for speed using a fast-path float parser. **Boost.JSON** passes 54/66, **POCO** and **simdjson** 57/66.

**Parse String**: all libraries pass all 9 cases.

**Roundtrip**: **Join**, **taocpp/json**, and **RapidJSON** (`full-prec`) achieve a perfect 27/27. The remaining **RapidJSON** variants (default/iterative/insitu/auto-utf) also reach 27/27. **nlohmann/json** gets 26/27; **jsoncons** 25/27. **POCO** (24/27), **simdjson** (26/27), and **JsonCpp** (22/27) have more failures. **Boost.JSON** trails at 19/27 — its serialiser does not reproduce the exact decimal representation of floats from the input.

### Conclusion

The best libraries are those that are both fast and correct. Several stand out on both axes:

- **RapidJSON** offers the best overall speed profile: `insitu` (12 ms) and `default` (14 ms) are the fastest general-purpose DOM parsers, with all five variants leading in both stringify (~18 ms) and prettify (~19 ms). `full-prec` adds bitwise-accurate double parsing at a parse cost (22 ms) but still achieves perfect conformance (136/136). When float precision is not critical, `insitu` or `default` mode give maximum throughput at the cost of 18 double-precision cases.
- **Join** achieves perfect conformance (136/136) — perfect double precision and a perfect roundtrip score — while parsing nearly as fast as RapidJSON's default mode (29 ms). It is the strongest all-rounder when both correctness and performance matter.
- **taocpp/json** also reaches perfect conformance (136/136) with competitive performance across all categories (parse 38 ms, stringify 23 ms, prettify 30 ms). A solid choice when correctness is non-negotiable and RapidJSON's API is not a fit.
- **simdjson** is the fastest parser by a wide margin (~1.8× faster than RapidJSON insitu at 6 ms) with strong conformance scores, but it is a read-only library. Use it when you only need to read JSON at maximum speed and do not need to modify or re-serialise the document.
- **nlohmann/json** offers perfect double precision and near-perfect roundtrip (26/27) in a widely used, ergonomic API, at the cost of parse throughput (91 ms). A reasonable default when correctness and ease of use outweigh raw speed.
- **jsoncons** achieves near-perfect conformance (134/136) with competitive statistics performance (1.7 ms). Parse speed (76 ms) is comparable to nlohmann.
- **Boost.JSON** is the fastest serialiser (11 ms), ranks second in prettify (27 ms, just behind RapidJSON), and has solid parse performance (16 ms). Its roundtrip score (19/27) is a concern if exact decimal reproduction of floats matters.
- **JsonCpp** and **POCO** are consistently the slowest in every performance category and offer no conformance advantage. They are best avoided in performance-sensitive code.

## License

Licensed under the [MIT License](LICENSE).
