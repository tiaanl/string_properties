cmake_minimum_required(VERSION 2.8)

project("string_properties")

include_directories("include")

file(GLOB_RECURSE "ALL_FILES"
  "include/meta/*.cpp"
  "include/meta/*.h"
  "tests/*.cpp"
  "tests/*.h"
  )

add_executable("string_properties_tests" ${ALL_FILES})
