cmake_minimum_required(VERSION 3.20)
project(CppToRustDemos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CPPTORUST_ALLOW_FETCHCONTENT
  "Allow ModelStore to fetch dependencies when not installed system-wide"
  ON
)

set(MODELSTORE_BUILD_TESTS OFF CACHE BOOL "Disable ModelStore tests in demo project" FORCE)
if (CPPTORUST_ALLOW_FETCHCONTENT)
  set(MODELSTORE_ALLOW_FETCHCONTENT ON CACHE BOOL "Allow ModelStore FetchContent" FORCE)
endif()

add_subdirectory(external/CppLmmModelStore)

enable_testing()

add_executable(demo_01_resolve_paths demos/demo_01_resolve_paths.cpp)
target_link_libraries(demo_01_resolve_paths PRIVATE ModelStore::ModelStore)

add_executable(demo_02_ensure_model_dir demos/demo_02_ensure_model_dir.cpp)
target_link_libraries(demo_02_ensure_model_dir PRIVATE ModelStore::ModelStore)

add_executable(demo_03_model_exists demos/demo_03_model_exists.cpp)
target_link_libraries(demo_03_model_exists PRIVATE ModelStore::ModelStore)

add_executable(demo_04_stream_parser_basic demos/demo_04_stream_parser_basic.cpp)
target_link_libraries(demo_04_stream_parser_basic PRIVATE ModelStore::ModelStore)

add_executable(demo_05_stream_parser_chunked demos/demo_05_stream_parser_chunked.cpp)
target_link_libraries(demo_05_stream_parser_chunked PRIVATE ModelStore::ModelStore)

add_executable(cpp_to_rust_tests tests/cpp_to_rust_tests.cpp)
target_link_libraries(cpp_to_rust_tests PRIVATE ModelStore::ModelStore)
add_test(NAME cpp_to_rust_tests COMMAND cpp_to_rust_tests)
