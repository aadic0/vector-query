cmake_minimum_required(VERSION 3.25)
project(VectorQuery VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(VECTOR_QUERY_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

include(FetchContent)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.4.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(googletest googlebenchmark)

function(vector_query_settings target)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_features(${target} PUBLIC cxx_std_23)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow)
    endif()
    if(VECTOR_QUERY_SANITIZERS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT MSVC)
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        else()
            message(FATAL_ERROR "Sanitizer preset requires GCC or Clang with ASan/UBSan support")
        endif()
    endif()
endfunction()

add_library(vector_query STATIC src/data_generator.cpp src/scalar_kernel.cpp)
target_include_directories(vector_query PUBLIC ${PROJECT_SOURCE_DIR}/include)
vector_query_settings(vector_query)

add_executable(vector_query_demo examples/run_query.cpp)
target_link_libraries(vector_query_demo PRIVATE vector_query)
vector_query_settings(vector_query_demo)

include(CTest)
add_executable(vector_query_tests tests/data_generator_tests.cpp tests/scalar_kernel_tests.cpp)
target_link_libraries(vector_query_tests PRIVATE vector_query GTest::gtest_main)
vector_query_settings(vector_query_tests)
if(BUILD_TESTING)
    include(GoogleTest)
    gtest_discover_tests(vector_query_tests DISCOVERY_MODE PRE_TEST)
endif()

add_executable(vector_query_benchmarks benchmarks/scalar_kernel_benchmark.cpp)
target_link_libraries(vector_query_benchmarks PRIVATE vector_query benchmark::benchmark_main)
vector_query_settings(vector_query_benchmarks)
