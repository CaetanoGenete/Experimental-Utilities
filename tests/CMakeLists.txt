include(FetchContent)
include(GoogleTest)
include(add_gtest)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.12.1
    SOURCE_DIR     "${EXPU_DEPENDENCIES_SOURCE_DIR}/googletest/"
    BINARY_DIR     "${EXPU_DEPENDENCIES_BINARY_DIR}/googletest/build/"
    SUBBUILD_DIR   "${EXPU_DEPENDENCIES_BINARY_DIR}/googletest/sub-build/")

FetchContent_MakeAvailable(googletest)

set_target_properties(gtest      PROPERTIES FOLDER extern)
set_target_properties(gtest_main PROPERTIES FOLDER extern)
set_target_properties(gmock      PROPERTIES FOLDER extern)
set_target_properties(gmock_main PROPERTIES FOLDER extern)

add_gtest(darray "darray.cpp" expu)
target_compile_definitions(
    darray 
    PRIVATE 
    EXPU_ALLOW_TRIVIAL_TEST_TYPE 
    EXPU_CHECKED_ALLOCATOR_LEVEL=1)

add_gtest(fixed_array "fixed_array.cpp" expu)
target_compile_definitions(
    fixed_array
    PRIVATE 
    EXPU_ALLOW_TRIVIAL_TEST_TYPE)

add_gtest(typelist_set_operations "typelist_set_operations.cpp" expu)