#include <benchmark/benchmark.h>

#include "adapters/boost.hpp"
#include "adapters/join.hpp"
#include "adapters/jsoncons.hpp"
#include "adapters/jsoncpp.hpp"
#include "adapters/nlohmann.hpp"
#include "adapters/poco.hpp"
#include "adapters/rapidjson.hpp"
#include "adapters/taocpp.hpp"
#include "common/utils.hpp"

static const std::string canada = load_file (DATA_DIR "/canada.json");
static const std::string citm = load_file (DATA_DIR "/citm_catalog.json");
static const std::string twitter = load_file (DATA_DIR "/twitter.json");

template <typename Adapter>
static void BM_Prettify (benchmark::State& state, const std::string* json)
{
    auto doc = Adapter::parse (*json);
    if (!doc)
    {
        state.SkipWithError ("Parse failed");
        return;
    }
    for (auto _ : state)
    {
        auto result = Adapter::prettify (*doc);
        benchmark::DoNotOptimize (result);
    }
    state.SetBytesProcessed (state.iterations () * static_cast<int64_t> (json->size ()));
    state.SetLabel (std::string (Adapter::name));
}

#define REGISTER_PRETTIFY(Adapter, tag, file)            \
    BENCHMARK_CAPTURE (BM_Prettify<Adapter>, tag, &file) \
        ->Name ("Prettify/" #tag "/" #Adapter)           \
        ->Unit (benchmark::kMillisecond)

#define REGISTER_PRETTIFY_DATASET(tag, file)                  \
    REGISTER_PRETTIFY (BoostAdapter, tag, file);              \
    REGISTER_PRETTIFY (JoinAdapter, tag, file);               \
    REGISTER_PRETTIFY (JsonConsAdapter, tag, file);           \
    REGISTER_PRETTIFY (JsonCppAdapter, tag, file);            \
    REGISTER_PRETTIFY (NlohmannAdapter, tag, file);           \
    REGISTER_PRETTIFY (PocoAdapter, tag, file);               \
    REGISTER_PRETTIFY (RapidJsonAutoUtfAdapter, tag, file);   \
    REGISTER_PRETTIFY (RapidJsonAdapter, tag, file);          \
    REGISTER_PRETTIFY (RapidJsonFullPrecAdapter, tag, file);  \
    REGISTER_PRETTIFY (RapidJsonInsituAdapter, tag, file);    \
    REGISTER_PRETTIFY (RapidJsonIterativeAdapter, tag, file); \
    REGISTER_PRETTIFY (TaoCppAdapter, tag, file)

REGISTER_PRETTIFY_DATASET (Canada, canada);
REGISTER_PRETTIFY_DATASET (Citm, citm);
REGISTER_PRETTIFY_DATASET (Twitter, twitter);

BENCHMARK_MAIN ();
