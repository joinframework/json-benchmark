# jsoncpp and POCO cache CMAKE_RUNTIME_OUTPUT_DIRECTORY to <build>/bin: keep the binaries where run_bench.sh expects them.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(conformance main.cpp)
target_include_directories(conformance PRIVATE ${CMAKE_SOURCE_DIR}/benchmarks)
target_compile_definitions(conformance PRIVATE DATA_DIR="${CMAKE_SOURCE_DIR}/data")
target_link_libraries(conformance PRIVATE ${JSON_BENCH_LIBS})
target_compile_options(conformance PRIVATE -Wall -Wextra)
