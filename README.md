# json-benchmark

Comparative benchmarks of JSON libraries in C++, measuring parse, stringify, prettify, and DOM statistics performance across three real-world datasets (Canada, CitmCatalog, Twitter). Includes a conformance suite covering parse validation, double precision, string decoding, and roundtrip correctness.

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

Performance time reported per benchmark is the median across all three datasets combined (ms).

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
    --benchmark_repetitions=3 \
    --benchmark_min_warmup_time=0.5 \
    --benchmark_report_aggregates_only=true
```

Conformance only:

```bash
./build/benchmarks/conformance/conformance --output results/conformance.json
```

> [!NOTE]
> Default: 3 repetitions, median is reported. Override with `REPETITIONS=10 bash scripts/run_bench.sh`.

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

![Read](plots/performance_parse.png)

![Stringify](plots/performance_stringify.png)

![Prettify](plots/performance_prettify.png)

**Parse**: simdjson leads at 6 ms, more than 1.8× faster than the next best (`rapidjson/insitu` at 11 ms). Among the general-purpose DOM libraries, RapidJSON's in-situ mode leads thanks to its zero-copy string handling, followed closely by the default and iterative variants (13–14 ms) and Boost.JSON (15 ms). Join (26 ms) sits between RapidJSON and the slower libraries. taocpp (37 ms) is competitive. jsoncons (77 ms) and nlohmann (83 ms) are slower but similar; POCO (129 ms) and JsonCpp (159 ms) trail significantly.

**Stringify**: Boost.JSON leads at 10 ms. All RapidJSON variants cluster tightly at ~15 ms. taocpp (18 ms) and simdjson (18 ms, minified output only) follow. nlohmann (21 ms) and Join (22 ms) are close; jsoncons (25 ms) is slightly behind. POCO (63 ms) and JsonCpp (165 ms) are the slowest.

**Prettify**: RapidJSON variants lead at 19 ms across all five modes. Nlohmann (27 ms) and Join (28 ms) follow, with taocpp (29 ms) close behind. Boost (42 ms) and jsoncons (53 ms) are slower. JsonCpp (173 ms) and POCO (198 ms) trail by a wide margin.

**Statistics**: RapidJSON's five variants cluster at ~1 ms — their contiguous memory pool layout maximises cache efficiency. jsoncons (1.6 ms), simdjson (1.7 ms), Boost (1.9 ms), and taocpp (1.9 ms) all follow closely. Join (3.3 ms) and nlohmann (3.5 ms) are about 2× slower than RapidJSON. JsonCpp (16 ms) and POCO (22 ms) are again the outliers.

### Conformance

![Conformance](plots/conformance.png)

**Parse Validation**: most libraries pass all 34 cases. JsonCpp fails 8 (it is lenient on some constructs that the JSON spec rejects, such as comments).

**Parse Double**: the sharpest differentiator. Join, jsoncons, nlohmann, taocpp, and `rapidjson/full-prec` achieve perfect precision (66/66). RapidJSON's default, iterative, insitu, and auto-utf modes all fail 18 cases — they trade precision for speed using a fast-path float parser. Boost passes 54/66, POCO and simdjson 57/66.

**Parse String**: all libraries pass all 9 cases.

**Roundtrip**: Join, taocpp, and `rapidjson/full-prec` achieve a perfect 27/27. The remaining RapidJSON variants (default/iterative/insitu/auto-utf) also reach 27/27. nlohmann gets 26/27; jsoncons 25/27. POCO (24/27), simdjson (26/27), and JsonCpp (22/27) have more failures. Boost trails at 19/27 — its serialiser does not reproduce the exact decimal representation of floats from the input.

### Conclusion

- **simdjson** is the fastest parser by a wide margin (~1.8× faster than RapidJSON insitu) and has strong conformance scores, but it is a parse-only library. Use it when you only need to read JSON at maximum speed and do not need to modify or re-serialise the document.
- **RapidJSON** is the strongest all-rounder: near the top in every performance category, 100% conformance with `full-prec`, and a rich feature set (five parser modes benchmarked here cover the most common trade-offs). `insitu` mode is the fastest general-purpose DOM parser; `full-prec` restores bitwise-accurate double parsing at modest cost.
- **Boost.JSON** is the fastest serialiser (10 ms) and has solid parse performance (15 ms). Its roundtrip failures on floats are worth noting if exact decimal reproduction matters.
- **nlohmann/json** is a conformant general-purpose library (perfect double precision, 26/27 roundtrip) at the cost of parse throughput (83 ms, ~7× slower than RapidJSON insitu). A good default choice when ergonomics and correctness outweigh raw speed.
- **Join** matches nlohmann's double precision and achieves a perfect roundtrip score (27/27), while parsing ~3.2× faster (26 ms). A good middle ground between RapidJSON's performance and nlohmann's ergonomics.
- **taocpp/json** achieves perfect conformance (136/136) and competitive performance across all categories — parse (37 ms), prettify (29 ms), stringify (18 ms), statistics (1.9 ms). A strong option when correctness is paramount.
- **jsoncons** achieves near-perfect conformance (134/136) with competitive statistics performance (1.6 ms, best among non-RapidJSON DOM libraries). Parse speed (77 ms) is similar to nlohmann.
- **JsonCpp** and **POCO** are consistently the slowest in every performance category and offer no conformance advantage. They are best avoided in performance-sensitive code.

## License

Licensed under the [MIT License](LICENSE).
