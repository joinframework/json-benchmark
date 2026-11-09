# jsoncpp and POCO cache CMAKE_RUNTIME_OUTPUT_DIRECTORY to <build>/bin: keep the binaries where run_bench.sh expects them.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

function(add_json_benches library header link)
    cmake_parse_arguments(ARG "" "" "PARSE;STRINGIFY;PRETTIFY;STATISTICS" ${ARGN})
    foreach(op PARSE STRINGIFY PRETTIFY STATISTICS)
        if(ARG_${op})
            string(TOLOWER ${op} name)
            set(adapters "")
            foreach(adapter ${ARG_${op}})
                string(APPEND adapters "X(${adapter}) ")
            endforeach()
            set(target bench_${name}_${library})
            add_executable(${target} bench_${name}.cpp)
            target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/benchmarks)
            target_compile_definitions(${target} PRIVATE
                DATA_DIR="${CMAKE_SOURCE_DIR}/data"
                ADAPTER_HEADER="${header}"
                "BENCH_ADAPTERS=${adapters}"
            )
            target_link_libraries(${target} PRIVATE benchmark::benchmark ${link})
            target_compile_options(${target} PRIVATE -Wall -Wextra)
        endif()
    endforeach()
endfunction()

add_json_benches(boost adapters/boost.hpp Boost::json
    PARSE      BoostAdapter BoostFullPrecAdapter
    STRINGIFY  BoostAdapter
    STATISTICS BoostAdapter
)
add_json_benches(cjson adapters/cjson.hpp cjson::cjson
    PARSE      CJsonAdapter
    STATISTICS CJsonAdapter
)
add_json_benches(join adapters/join.hpp join::json
    PARSE      JoinAdapter
    STRINGIFY  JoinAdapter
    PRETTIFY   JoinAdapter
    STATISTICS JoinAdapter
)
add_json_benches(jsoncons adapters/jsoncons.hpp jsoncons::jsoncons
    PARSE      JsonConsAdapter JsonConsLosslessAdapter
    STRINGIFY  JsonConsAdapter JsonConsLosslessAdapter
    PRETTIFY   JsonConsAdapter JsonConsLosslessAdapter
    STATISTICS JsonConsAdapter JsonConsLosslessAdapter
)
add_json_benches(jsoncpp adapters/jsoncpp.hpp jsoncpp_static
    PARSE      JsonCppAdapter
    STRINGIFY  JsonCppAdapter
    PRETTIFY   JsonCppAdapter
    STATISTICS JsonCppAdapter
)
add_json_benches(nlohmann adapters/nlohmann.hpp nlohmann_json::nlohmann_json
    PARSE      NlohmannAdapter
    STRINGIFY  NlohmannAdapter
    PRETTIFY   NlohmannAdapter
    STATISTICS NlohmannAdapter
)
add_json_benches(poco adapters/poco.hpp Poco::JSON
    PARSE      PocoAdapter
    STRINGIFY  PocoAdapter
    PRETTIFY   PocoAdapter
    STATISTICS PocoAdapter
)
add_json_benches(rapidjson adapters/rapidjson.hpp rapidjson::rapidjson
    PARSE      RapidJsonAutoUtfAdapter RapidJsonAdapter RapidJsonFullPrecAdapter RapidJsonInsituAdapter RapidJsonIterativeAdapter
    STRINGIFY  RapidJsonAutoUtfAdapter RapidJsonAdapter RapidJsonInsituAdapter RapidJsonIterativeAdapter
    PRETTIFY   RapidJsonAutoUtfAdapter RapidJsonAdapter RapidJsonInsituAdapter RapidJsonIterativeAdapter
    STATISTICS RapidJsonAutoUtfAdapter RapidJsonAdapter RapidJsonInsituAdapter RapidJsonIterativeAdapter
)
add_json_benches(simdjson adapters/simdjson.hpp simdjson::simdjson
    PARSE      SimdJsonAdapter
    STRINGIFY  SimdJsonAdapter
    PRETTIFY   SimdJsonAdapter
    STATISTICS SimdJsonAdapter
)
add_json_benches(sonic adapters/sonic.hpp sonic::sonic
    PARSE      SonicAdapter
    STRINGIFY  SonicAdapter
    STATISTICS SonicAdapter
)
add_json_benches(taocpp adapters/taocpp.hpp taocpp::json
    PARSE      TaoCppAdapter
    STRINGIFY  TaoCppAdapter
    PRETTIFY   TaoCppAdapter
    STATISTICS TaoCppAdapter
)
add_json_benches(yyjson adapters/yyjson.hpp yyjson
    PARSE      YyjsonAdapter
    STRINGIFY  YyjsonAdapter
    PRETTIFY   YyjsonAdapter
    STATISTICS YyjsonAdapter
)
