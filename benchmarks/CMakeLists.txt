set(smm_benchmark_source_rel_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/")

set(smm_benchmarks_source_dirs
    "${PROJECT_NAME}/containers/darray.cpp")

#Convert relative paths to absolute 
list(TRANSFORM smm_benchmarks_source_dirs PREPEND ${smm_benchmark_source_rel_dir})

add_executable("${PROJECT_NAME}_benchmark" ${smm_benchmarks_source_dirs})
target_include_directories(
    "${PROJECT_NAME}_benchmark" 
    PRIVATE 
    "${smm_benchmark_source_rel_dir}/${PROJECT_NAME}")

find_package(benchmark REQUIRED)
target_link_libraries(
    "${PROJECT_NAME}_benchmark" 
    benchmark::benchmark
    "${PROJECT_NAME}")

#Match MSVC filters to file structure starting from 'tests' subdirectory
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${smm_benchmarks_source_dirs})