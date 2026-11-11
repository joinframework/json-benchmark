include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.10.0
    GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING     OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL     OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

FetchContent_Declare(
    join
    GIT_REPOSITORY https://github.com/joinframework/join.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(join)
add_library(join_json STATIC
    ${join_SOURCE_DIR}/core/src/error.cpp
    ${join_SOURCE_DIR}/data/src/sax_error.cpp
    ${join_SOURCE_DIR}/data/src/json.cpp
)
add_library(join::json ALIAS join_json)
target_include_directories(join_json PUBLIC
    ${join_SOURCE_DIR}/core/include
    ${join_SOURCE_DIR}/data/include
)
target_compile_features(join_json PUBLIC cxx_std_20)

FetchContent_Declare(
    dconv
    GIT_REPOSITORY https://github.com/joinframework/dconv.git
    GIT_TAG        v1.2.2
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(dconv)
target_include_directories(join_json PUBLIC ${dconv_SOURCE_DIR}/dconv/include)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG        297637bc0a0f00dc504dbc1a47ebdaa577f8ae19
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(rapidjson)
add_library(rapidjson INTERFACE)
add_library(rapidjson::rapidjson ALIAS rapidjson)
target_include_directories(rapidjson SYSTEM INTERFACE ${rapidjson_SOURCE_DIR}/include)

# RapidJSON only uses SIMD when told to: enable the best instruction set -march=native provides.
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS -march=native)
check_cxx_source_compiles("#ifndef __SSE4_2__\n#error\n#endif\nint main () {}" JSON_BENCH_HAS_SSE42)
check_cxx_source_compiles("#ifndef __SSE2__\n#error\n#endif\nint main () {}" JSON_BENCH_HAS_SSE2)
check_cxx_source_compiles("#ifndef __ARM_NEON\n#error\n#endif\nint main () {}" JSON_BENCH_HAS_NEON)
unset(CMAKE_REQUIRED_FLAGS)
if(JSON_BENCH_HAS_SSE42)
    target_compile_definitions(rapidjson INTERFACE RAPIDJSON_SSE42)
elseif(JSON_BENCH_HAS_SSE2)
    target_compile_definitions(rapidjson INTERFACE RAPIDJSON_SSE2)
elseif(JSON_BENCH_HAS_NEON)
    target_compile_definitions(rapidjson INTERFACE RAPIDJSON_NEON)
endif()

FetchContent_Declare(
    jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG        1.9.8
    GIT_SHALLOW    TRUE
)
set(JSONCPP_WITH_TESTS               OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_PKGCONFIG_SUPPORT   OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_CMAKE_PACKAGE       OFF CACHE BOOL "" FORCE)
set(BUILD_OBJECT_LIBS                OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(jsoncpp)

FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.92.0/boost-1.92.0-cmake.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BOOST_INCLUDE_LIBRARIES json)
set(BOOST_ENABLE_CMAKE      ON)
FetchContent_MakeAvailable(Boost)

FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG        v5.1.1
    GIT_SHALLOW    TRUE
)
set(SIMDJSON_BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(simdjson)

FetchContent_Declare(
    taocpp_pegtl
    GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
    GIT_TAG        3.2.9
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(taocpp_pegtl)
add_library(taocpp_pegtl_iface INTERFACE)
add_library(taocpp::pegtl ALIAS taocpp_pegtl_iface)
target_include_directories(taocpp_pegtl_iface SYSTEM INTERFACE ${taocpp_pegtl_SOURCE_DIR}/include)

FetchContent_Declare(
    taocpp_json
    GIT_REPOSITORY https://github.com/taocpp/json.git
    GIT_TAG        1.0.0-beta.14
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(taocpp_json)
add_library(taocpp_json_iface INTERFACE)
add_library(taocpp::json ALIAS taocpp_json_iface)
target_include_directories(taocpp_json_iface SYSTEM INTERFACE ${taocpp_json_SOURCE_DIR}/include)
target_link_libraries(taocpp_json_iface INTERFACE taocpp::pegtl)

FetchContent_Declare(
    jsoncons
    GIT_REPOSITORY https://github.com/danielaparker/jsoncons.git
    GIT_TAG        v1.10.0
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(jsoncons)
add_library(jsoncons_iface INTERFACE)
add_library(jsoncons::jsoncons ALIAS jsoncons_iface)
target_include_directories(jsoncons_iface SYSTEM INTERFACE ${jsoncons_SOURCE_DIR}/include)

FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG        0.13.0
    GIT_SHALLOW    TRUE
)
set(YYJSON_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yyjson)

FetchContent_Declare(
    sonic_cpp
    GIT_REPOSITORY https://github.com/bytedance/sonic-cpp.git
    GIT_TAG        v1.0.2
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(sonic_cpp)
add_library(sonic_iface INTERFACE)
add_library(sonic::sonic ALIAS sonic_iface)
target_include_directories(sonic_iface SYSTEM INTERFACE ${sonic_cpp_SOURCE_DIR}/include)

FetchContent_Declare(
    cjson
    GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
    GIT_TAG        v1.7.19
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(cjson)
add_library(cjson STATIC ${cjson_SOURCE_DIR}/cJSON.c)
add_library(cjson::cjson ALIAS cjson)
target_include_directories(cjson SYSTEM PUBLIC ${cjson_SOURCE_DIR})

FetchContent_Declare(
    poco
    GIT_REPOSITORY https://github.com/pocoproject/poco.git
    GIT_TAG        poco-1.15.4-release
    GIT_SHALLOW    TRUE
)
set(ENABLE_TESTS                    OFF CACHE BOOL "" FORCE)
set(ENABLE_SAMPLES                  OFF CACHE BOOL "" FORCE)
set(ENABLE_ENCODINGS                OFF CACHE BOOL "" FORCE)
set(ENABLE_ENCODINGS_COMPILER       OFF CACHE BOOL "" FORCE)
set(ENABLE_XML                      OFF CACHE BOOL "" FORCE)
set(ENABLE_JSON                     ON  CACHE BOOL "" FORCE)
set(ENABLE_UTIL                     OFF CACHE BOOL "" FORCE)
set(ENABLE_NET                      OFF CACHE BOOL "" FORCE)
set(ENABLE_NETSSL                   OFF CACHE BOOL "" FORCE)
set(ENABLE_NETSSL_WIN               OFF CACHE BOOL "" FORCE)
set(ENABLE_CRYPTO                   OFF CACHE BOOL "" FORCE)
set(ENABLE_JWT                      OFF CACHE BOOL "" FORCE)
set(ENABLE_DATA                     OFF CACHE BOOL "" FORCE)
set(ENABLE_DATA_SQLITE              OFF CACHE BOOL "" FORCE)
set(ENABLE_DATA_MYSQL               OFF CACHE BOOL "" FORCE)
set(ENABLE_DATA_ODBC                OFF CACHE BOOL "" FORCE)
set(ENABLE_DATA_POSTGRESQL          OFF CACHE BOOL "" FORCE)
set(ENABLE_MONGODB                  OFF CACHE BOOL "" FORCE)
set(ENABLE_REDIS                    OFF CACHE BOOL "" FORCE)
set(ENABLE_PROMETHEUS               OFF CACHE BOOL "" FORCE)
set(ENABLE_ZIP                      OFF CACHE BOOL "" FORCE)
set(ENABLE_SEVENZIP                 OFF CACHE BOOL "" FORCE)
set(ENABLE_PAGECOMPILER             OFF CACHE BOOL "" FORCE)
set(ENABLE_PAGECOMPILER_FILE2PAGE   OFF CACHE BOOL "" FORCE)
set(ENABLE_ACTIVERECORD             OFF CACHE BOOL "" FORCE)
set(ENABLE_ACTIVERECORD_COMPILER    OFF CACHE BOOL "" FORCE)
set(ENABLE_APACHECONNECTOR          OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(poco)
