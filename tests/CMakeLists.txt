
#Project directories 
set(smm_tests_source_rel_dir "${CMAKE_CURRENT_SOURCE_DIR}/src/")


#Relative path to source files (From ${smm_tests_source_rel_dir})
set(smm_tests_source_dirs 
    "${PROJECT_NAME}/test_utils.hpp"
    "${PROJECT_NAME}/meta_sort.hpp"

    "${PROJECT_NAME}/wrappers/disabled_wrapper.hpp"
    "${PROJECT_NAME}/wrappers/disabled_wrapper.cpp"
    "${PROJECT_NAME}/wrappers/counted_wrapper.hpp"
    "${PROJECT_NAME}/wrappers/counted_allocator.hpp"
    "${PROJECT_NAME}/wrappers/counted_allocator.cpp"
    "${PROJECT_NAME}/wrappers/cond_allocator.hpp"

    "${PROJECT_NAME}/containers/darray.cpp"
    "${PROJECT_NAME}/containers/named_requirements.cpp"
    "${PROJECT_NAME}/containers/named_requirements.hpp"
    "${PROJECT_NAME}/containers/darray_iters.cpp")

#Convert relative paths to absolute 
list(TRANSFORM smm_tests_source_dirs PREPEND ${smm_tests_source_rel_dir})

add_executable("${PROJECT_NAME}_tests" ${smm_tests_source_dirs})
target_include_directories(
    "${PROJECT_NAME}_tests" 
    PRIVATE 
    "${smm_tests_source_rel_dir}/${PROJECT_NAME}")

#Download and install gtest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.11.0)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

target_link_libraries(
    "${PROJECT_NAME}_tests"
    "${PROJECT_NAME}"
	gtest_main)

include(GoogleTest)
gtest_discover_tests("${PROJECT_NAME}_tests")

#Match MSVC filters to file structure starting from 'tests' subdirectory
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${smm_tests_source_dirs})