cmake_minimum_required(VERSION 3.22)

project(
    Experimental-Utilities
    VERSION 0.0.1
    DESCRIPTION "Terrible implementantion of a few STL containers and functions"
    LANGUAGES CXX)

#Project directories 
set(smm_header_rel_dir "${CMAKE_SOURCE_DIR}/include/")
set(smm_source_rel_dir  "${CMAKE_SOURCE_DIR}/src/")

#Relative path of public headers (From ${headers_rel_dir})
set(smm_pub_header_dirs 
    "include/expu/debug.hpp"
    "include/expu/defines.hpp"
    "include/expu/traits_ext.hpp"
    "include/expu/mem_utils.hpp"
    "include/expu/iter_utils.hpp"
    "include/expu/int_seq.hpp"
    "include/expu/containers/darray.hpp"
    "include/expu/containers/llist.hpp"
    "include/expu/containers/graph.hpp"
    "include/expu/containers/hashmap.hpp"
    "include/expu/iterators/sorting.hpp"
)

add_library(${PROJECT_NAME} STATIC ${smm_source_dirs} ${smm_pub_header_dirs})

if(MSVC)
    #Required for __VA_OPT__ support
    target_compile_options(${PROJECT_NAME} PUBLIC /Zc:preprocessor)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${smm_header_rel_dir})

#Match MSVC filters to file structure
source_group(
    TREE 
    ${CMAKE_SOURCE_DIR} 
    FILES 
    ${smm_source_dirs} 
    ${smm_pub_header_dirs})

#Ensures dependencies are installed to the 'deps' directory.
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/deps")

#Testing
enable_testing()
add_subdirectory("tests")

#Benchmarks are compiled as part of the project
add_subdirectory("benchmarks")