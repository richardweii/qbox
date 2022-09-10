cmake_minimum_required(VERSION 3.10)
project(qbox)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

enable_testing()
add_subdirectory(third_party/googletest)
include_directories(include third_party/googletest/googletest/include)
set(GTEST_LIB gtest gtest_main)

add_subdirectory(src)

function(add TEST_FILE)
  get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
  add_executable(${TEST_NAME} ${TEST_FILE})
  target_link_libraries(${TEST_NAME} PRIVATE 
    ${GTEST_LIB}
    coroutine
  )
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
  message(STATUS "ADD TEST : ${TEST_NAME}")
endfunction()

add_subdirectory(test)