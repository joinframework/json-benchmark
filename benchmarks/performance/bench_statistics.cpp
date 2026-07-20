#include <benchmark/benchmark.h>

#include "adapters/boost.hpp"
#include "adapters/join.hpp"
#include "adapters/jsoncons.hpp"
#include "adapters/jsoncpp.hpp"
#include "adapters/nlohmann.hpp"
#include "adapters/poco.hpp"
#include "adapters/rapidjson.hpp"
#include "adapters/simdjson.hpp"
#include "adapters/taocpp.hpp"
#include "common/utils.hpp"

static const std::string canada = load_file (DATA_DIR "/canada.json");
static const std::string citm = load_file (DATA_DIR "/citm_catalog.json");
static const std::string twitter = load_file (DATA_DIR "/twitter.json");

template <typename Adapter>
static void BM_Statistics (benchmark::State& state, const std::string* json)
{
    auto doc = Adapter::parse (*json);
    if (!doc)
    {
        state.SkipWithError ("Parse failed");
        return;
    }
    for (auto _ : state)
    {
        auto stat = Adapter::statistics (*doc);
        benchmark::DoNotOptimize (stat);
    }
    state.SetBytesProcessed (state.iterations () * static_cast<int64_t> (json->size ()));
    state.SetLabel (std::string (Adapter::name));
}

#define REGISTER_STATISTICS(Adapter, tag, file)            \
    BENCHMARK_CAPTURE (BM_Statistics<Adapter>, tag, &file) \
        ->Name ("Statistics/" #tag "/" #Adapter)           \
        ->Unit (benchmark::kMillisecond)

#define REGISTER_STATISTICS_DATASET(tag, file)                  \
    REGISTER_STATISTICS (BoostAdapter, tag, file);              \
    REGISTER_STATISTICS (JoinAdapter, tag, file);               \
    REGISTER_STATISTICS (JsonConsAdapter, tag, file);           \
    REGISTER_STATISTICS (JsonCppAdapter, tag, file);            \
    REGISTER_STATISTICS (NlohmannAdapter, tag, file);           \
    REGISTER_STATISTICS (PocoAdapter, tag, file);               \
    REGISTER_STATISTICS (RapidJsonAutoUtfAdapter, tag, file);   \
    REGISTER_STATISTICS (RapidJsonAdapter, tag, file);          \
    REGISTER_STATISTICS (RapidJsonFullPrecAdapter, tag, file);  \
    REGISTER_STATISTICS (RapidJsonInsituAdapter, tag, file);    \
    REGISTER_STATISTICS (RapidJsonIterativeAdapter, tag, file); \
    REGISTER_STATISTICS (SimdJsonAdapter, tag, file);           \
    REGISTER_STATISTICS (TaoCppAdapter, tag, file)

REGISTER_STATISTICS_DATASET (Canada, canada);
REGISTER_STATISTICS_DATASET (Citm, citm);
REGISTER_STATISTICS_DATASET (Twitter, twitter);

BENCHMARK_MAIN ();
