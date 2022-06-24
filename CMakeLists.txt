cmake_minimum_required(VERSION 3.22)

#C++20 project with no extensions
set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(
    smm
    VERSION 0.0.1
    DESCRIPTION "Terrible implementantion of a few STL containers and functions"
    LANGUAGES CXX)

#Project directories 
set(smm_header_rel_dir "${CMAKE_SOURCE_DIR}/include/")
set(smm_source_rel_dir  "${CMAKE_SOURCE_DIR}/src/")

#Relative path of public headers (From ${headers_rel_dir})
set(smm_pub_header_dirs 
    "${PROJECT_NAME}/debug.hpp"
    "${PROJECT_NAME}/defines.hpp"
    "${PROJECT_NAME}/traits_ext.hpp"
    "${PROJECT_NAME}/mem_utils.hpp"
    "${PROJECT_NAME}/iter_utils.hpp"
    "${PROJECT_NAME}/int_seq.hpp"

    "${PROJECT_NAME}/containers/darray.hpp"
    "${PROJECT_NAME}/containers/llist.hpp"
    "${PROJECT_NAME}/containers/graph.hpp"
    "${PROJECT_NAME}/containers/hashmap.hpp"

    "${PROJECT_NAME}/iterators/sorting.hpp"
)

#Relative path of source files (From ${headers_rel_dir})
set(smm_source_dirs 
    "${PROJECT_NAME}/place_holder.cpp")

#Convert relative paths to absolute 
list(TRANSFORM smm_pub_header_dirs PREPEND "${smm_header_rel_dir}")
list(TRANSFORM smm_source_dirs     PREPEND "${smm_source_rel_dir}")

add_library(${PROJECT_NAME} STATIC ${smm_source_dirs} ${smm_pub_header_dirs})

if(MSVC)
    #Required for __VA_OPT__ support
    target_compile_options(${PROJECT_NAME} PUBLIC /Zc:preprocessor)
endif()

set_target_properties(
    ${PROJECT_NAME} 
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY   "${CMAKE_SOURCE_DIR}/build/bin/${CMAKE_GENERATOR}"
    OUTPUT_NAME_DEBUG          "${PROJECT_NAME}d"
    OUTPUT_NAME_RELWITHDEBINFO "${PROJECT_NAME}rd")

target_include_directories(${PROJECT_NAME} PUBLIC ${smm_header_rel_dir})

#Match MSVC filters to file structure
source_group(
    TREE 
    ${CMAKE_SOURCE_DIR} 
    FILES 
    ${smm_source_dirs} 
    ${smm_pub_header_dirs})

#TESTING
enable_testing()
add_subdirectory("tests")

add_subdirectory("benchmarks")