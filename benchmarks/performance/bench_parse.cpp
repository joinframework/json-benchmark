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
static void BM_Parse (benchmark::State& state, const std::string* json)
{
    for (auto _ : state)
    {
        auto doc = Adapter::parse (*json);
        benchmark::DoNotOptimize (doc);
        state.PauseTiming ();
        doc.reset ();
        state.ResumeTiming ();
    }
    state.SetBytesProcessed (state.iterations () * static_cast<int64_t> (json->size ()));
    state.SetLabel (std::string (Adapter::name));
}

#define REGISTER_PARSE(Adapter, tag, file) \
    BENCHMARK_CAPTURE (BM_Parse<Adapter>, tag, &file)->Name ("Parse/" #tag "/" #Adapter)->Unit (benchmark::kMillisecond)

#define REGISTER_PARSE_DATASET(tag, file)                  \
    REGISTER_PARSE (BoostAdapter, tag, file);              \
    REGISTER_PARSE (JoinAdapter, tag, file);               \
    REGISTER_PARSE (JsonConsAdapter, tag, file);           \
    REGISTER_PARSE (JsonCppAdapter, tag, file);            \
    REGISTER_PARSE (NlohmannAdapter, tag, file);           \
    REGISTER_PARSE (PocoAdapter, tag, file);               \
    REGISTER_PARSE (RapidJsonAutoUtfAdapter, tag, file);   \
    REGISTER_PARSE (RapidJsonAdapter, tag, file);          \
    REGISTER_PARSE (RapidJsonFullPrecAdapter, tag, file);  \
    REGISTER_PARSE (RapidJsonInsituAdapter, tag, file);    \
    REGISTER_PARSE (RapidJsonIterativeAdapter, tag, file); \
    REGISTER_PARSE (SimdJsonAdapter, tag, file);           \
    REGISTER_PARSE (TaoCppAdapter, tag, file)

REGISTER_PARSE_DATASET (Canada, canada);
REGISTER_PARSE_DATASET (Citm, citm);
REGISTER_PARSE_DATASET (Twitter, twitter);

BENCHMARK_MAIN ();
