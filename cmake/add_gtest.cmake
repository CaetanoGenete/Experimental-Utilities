function(add_gtest TEST_NAME SOURCES LIBRARIES)
    add_executable(${TEST_NAME} ${SOURCES})
    
    #Here linking to gtest_main. TODO: Perhaps add option otherwise if need
    #ever arises. 
    target_link_libraries(${TEST_NAME} ${LIBRARIES} gtest gmock gtest_main)

    gtest_discover_tests(
        ${TEST_NAME}
        WORKING_DIRECTORY ${EXPU_TESTS_DIR})

    set_target_properties(${TEST_NAME} PROPERTIES FOLDER tests)

endfunction()