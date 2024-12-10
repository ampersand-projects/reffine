set(TEST_FILES
    src/basic_tests.cpp
    src/test_base.cpp
)

add_executable(reffine_test ${TEST_FILES})
target_include_directories(reffine_test PUBLIC include)
target_link_libraries(reffine_test gtest_main reffine)

include(GoogleTest)
gtest_discover_tests(reffine_test)
