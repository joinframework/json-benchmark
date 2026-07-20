add_executable(conformance main.cpp)
target_include_directories(conformance PRIVATE ${CMAKE_SOURCE_DIR}/benchmarks)
target_compile_definitions(conformance PRIVATE DATA_DIR="${CMAKE_SOURCE_DIR}/data")
target_link_libraries(conformance PRIVATE
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
target_compile_options(conformance PRIVATE -Wall -Wextra)
