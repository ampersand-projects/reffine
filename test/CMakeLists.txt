set(TEST_FILES
    src/test_utils.cpp
    src/test_reduce.cpp
    src/test_transform.cpp
    src/basic_tests.cpp
)

find_package(Arrow REQUIRED)

add_executable(reffine_test ${TEST_FILES})
target_include_directories(reffine_test PUBLIC include)
target_link_libraries(reffine_test gtest_main Arrow::arrow_shared reffine)

include(GoogleTest)
gtest_discover_tests(reffine_test)
