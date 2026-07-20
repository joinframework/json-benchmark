set(JSON_BENCH_LIBS
    benchmark::benchmark_main
    join::json
    nlohmann_json::nlohmann_json
    rapidjson::rapidjson
    jsoncpp_static
    Boost::json
    Poco::JSON
    simdjson::simdjson
    taocpp::json
    jsoncons::jsoncons
)

function(add_json_bench target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/benchmarks)
    target_compile_definitions(${target} PRIVATE DATA_DIR="${CMAKE_SOURCE_DIR}/data")
    target_link_libraries(${target} PRIVATE ${JSON_BENCH_LIBS})
    target_compile_options(${target} PRIVATE -Wall -Wextra)
endfunction()

add_subdirectory(conformance)
add_subdirectory(performance)
