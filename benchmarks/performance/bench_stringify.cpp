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
static void BM_Stringify (benchmark::State& state, const std::string* json)
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
        auto result = Adapter::stringify (*doc);
        benchmark::DoNotOptimize (result);
    }
    set_minflt (state, before);
    state.SetBytesProcessed (state.iterations () * static_cast<int64_t> (json->size ()));
    state.SetLabel (std::string (Adapter::name));
}

#define REGISTER_STRINGIFY(Adapter, tag, file)            \
    BENCHMARK_CAPTURE (BM_Stringify<Adapter>, tag, &file) \
        ->Name ("Stringify/" #tag "/" #Adapter)           \
        ->Unit (benchmark::kMillisecond)

#define X(Adapter)                                \
    REGISTER_STRINGIFY (Adapter, Canada, canada); \
    REGISTER_STRINGIFY (Adapter, Citm, citm);     \
    REGISTER_STRINGIFY (Adapter, Twitter, twitter);

BENCH_ADAPTERS

BENCHMARK_MAIN ();
