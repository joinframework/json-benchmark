set(JSON_BENCH_LIBS
    join::json
    nlohmann_json::nlohmann_json
    rapidjson::rapidjson
    jsoncpp_static
    Boost::json
    Poco::JSON
    simdjson::simdjson
    taocpp::json
    jsoncons::jsoncons
    yyjson
    sonic::sonic
    cjson::cjson
)

add_subdirectory(conformance)
add_subdirectory(performance)
