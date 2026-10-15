#include <benchmark/benchmark.h>

#include <sys/resource.h>

#include ADAPTER_HEADER
#include "common/utils.hpp"

static const std::string canada = load_file (DATA_DIR "/canada.json");
static const std::string citm = load_file (DATA_DIR "/citm_catalog.json");
static const std::string twitter = load_file (DATA_DIR "/twitter.json");

static void set_minflt (benchmark::State& state, const rusage& before)
{
    rusage after;
    getrusage (RUSAGE_SELF, &after);
    state.counters["minflt"] = benchmark::Counter (static_cast<double> (after.ru_minflt - before.ru_minflt),
                                                   benchmark::Counter::kAvgIterations);
}

template <typename Adapter>
static void BM_Statistics (benchmark::State& state, const std::string* json)
{
    auto doc = Adapter::parse (*json);
    if (!doc)
    {
        state.SkipWithError ("Parse failed");
        return;
    }
    rusage before;
    getrusage (RUSAGE_SELF, &before);
    for (auto _ : state)
    {
        auto stat = Adapter::statistics (*doc);
        benchmark::DoNotOptimize (stat);
    }
    set_minflt (state, before);
    state.SetBytesProcessed (state.iterations () * static_cast<int64_t> (json->size ()));
    state.SetLabel (std::string (Adapter::name));
}

#define REGISTER_STATISTICS(Adapter, tag, file)            \
    BENCHMARK_CAPTURE (BM_Statistics<Adapter>, tag, &file) \
        ->Name ("Statistics/" #tag "/" #Adapter)           \
        ->Unit (benchmark::kMillisecond)

#define X(Adapter)                                 \
    REGISTER_STATISTICS (Adapter, Canada, canada); \
    REGISTER_STATISTICS (Adapter, Citm, citm);     \
    REGISTER_STATISTICS (Adapter, Twitter, twitter);

BENCH_ADAPTERS

BENCHMARK_MAIN ();
