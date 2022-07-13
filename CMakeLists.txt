cmake_minimum_required(VERSION 3.22)

project(
    Experimental-Utilities
    VERSION 0.0.1
    DESCRIPTION "Terrible implementantion of a few STL containers and functions"
    LANGUAGES CXX)

#Ensures targets are nicely organised into folder in choice IDE.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXPU_INCLUDE_DIR             "${CMAKE_SOURCE_DIR}/include/")
set(CMAKE_MODULE_PATH            "${CMAKE_SOURCE_DIR}/cmake/")
set(EXPU_DEPENDENCIES_BINARY_DIR "${CMAKE_BINARY_DIR}/dependencies/")
set(EXPU_DEPENDENCIES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/deps/")

#Relative path to headers from ${CMAKE_SOURCE_DIR}
set(EXPU_HEADERS
    "include/expu/debug.hpp"
    "include/expu/mem_utils.hpp"
    "include/expu/test_utils.hpp"
    "include/expu/meta/int_seq.hpp"
    "include/expu/meta/meta_utils.hpp"
    "include/expu/meta/function_traits.hpp"
    "include/expu/containers/darray.hpp"
    "include/expu/containers/linear_map.hpp"
    "include/expu/iterators/iter_utils.hpp"
    "include/expu/iterators/sorting.hpp"
    "include/expu/iterators/searching.hpp"
    "include/expu/iterators/seq_iter.hpp")

add_library(expu INTERFACE)
target_include_directories(expu INTERFACE ${EXPU_INCLUDE_DIR})
target_compile_features(expu INTERFACE cxx_std_20)

if(MSVC) #Required for __VA_OPTS__ support
    target_compile_options(expu INTERFACE /Zc:preprocessor)
endif()

###Additional options###

option(EXPU_CREATE_TARGET "Creates a mock target, used to view headers in IDE's")
if(EXPU_CREATE_TARGET)
    file(WRITE "${CMAKE_BINARY_DIR}/dummy.cpp" "// dummy.cpp exists solely to make cmake happy.")

    #Create dummy target to appear in IDE
    add_library(expu_headers MODULE EXCLUDE_FROM_ALL)
    target_sources(expu_headers PRIVATE ${EXPU_HEADERS} "${CMAKE_BINARY_DIR}/dummy.cpp")
    target_link_libraries(expu_headers expu)

    #Match IDE source tree with directory tree. 
    source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${EXPU_HEADERS})
endif()

option(EXPU_BUILD_TESTS "Builds and runs tests.")
if(EXPU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(EXPU_BUILD_BENCHMARKS "Builds benchmarks.")
if(EXPU_BUILD_BENCHMARKS)
    
endif()






