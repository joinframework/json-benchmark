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
static void BM_Stringify (benchmark::State& state, const std::string* json)
{
    auto doc = Adapter::parse (*json);
    if (!doc)
    {
        state.SkipWithError ("Parse failed");
        return;
    }
    for (auto _ : state)
    {
        auto result = Adapter::stringify (*doc);
        benchmark::DoNotOptimize (result);
    }
    state.SetBytesProcessed (state.iterations () * static_cast<int64_t> (json->size ()));
    state.SetLabel (std::string (Adapter::name));
}

#define REGISTER_STRINGIFY(Adapter, tag, file)            \
    BENCHMARK_CAPTURE (BM_Stringify<Adapter>, tag, &file) \
        ->Name ("Stringify/" #tag "/" #Adapter)           \
        ->Unit (benchmark::kMillisecond)

#define REGISTER_STRINGIFY_DATASET(tag, file)                  \
    REGISTER_STRINGIFY (BoostAdapter, tag, file);              \
    REGISTER_STRINGIFY (JoinAdapter, tag, file);               \
    REGISTER_STRINGIFY (JsonConsAdapter, tag, file);           \
    REGISTER_STRINGIFY (JsonCppAdapter, tag, file);            \
    REGISTER_STRINGIFY (NlohmannAdapter, tag, file);           \
    REGISTER_STRINGIFY (PocoAdapter, tag, file);               \
    REGISTER_STRINGIFY (RapidJsonAutoUtfAdapter, tag, file);   \
    REGISTER_STRINGIFY (RapidJsonAdapter, tag, file);          \
    REGISTER_STRINGIFY (RapidJsonFullPrecAdapter, tag, file);  \
    REGISTER_STRINGIFY (RapidJsonInsituAdapter, tag, file);    \
    REGISTER_STRINGIFY (RapidJsonIterativeAdapter, tag, file); \
    REGISTER_STRINGIFY (SimdJsonAdapter, tag, file);           \
    REGISTER_STRINGIFY (TaoCppAdapter, tag, file)

REGISTER_STRINGIFY_DATASET (Canada, canada);
REGISTER_STRINGIFY_DATASET (Citm, citm);
REGISTER_STRINGIFY_DATASET (Twitter, twitter);

BENCHMARK_MAIN ();
