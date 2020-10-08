cmake_minimum_required(VERSION 3.17)

project(string_properties)

set(HEADER_FILES
    include/meta/meta.h
    include/meta/meta_detail.h
    )

set(SOURCE_FILES
    src/meta.cpp
    )

set(TEST_FILES
    tests/tests.cpp
    )

add_library(string_properties ${HEADER_FILES} src/meta.cpp)
target_include_directories(string_properties PUBLIC include)
set_target_properties(
    string_properties
    PROPERTIES
    CXX_STANDARD 17
)

add_executable(string_properties_tests ${TEST_FILES})
set_target_properties(
    string_properties_tests
    PROPERTIES
    CXX_STANDARD 17
)
target_link_libraries(string_properties_tests PRIVATE string_properties)
