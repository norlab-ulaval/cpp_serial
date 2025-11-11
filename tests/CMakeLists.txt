cmake_minimum_required(VERSION 3.14)

if(NOT UNIX)
  message(STATUS "Skipping tests: only supported on UNIX-like systems")
  return()
endif()

if(NOT BUILD_TESTING)
  return()
endif()

include(GoogleTest)
find_package(GTest QUIET)

if(NOT GTest_FOUND)
  message(STATUS "GTest not found; skipping tests. Install libgtest-dev or provide GTest to enable tests.")
  return()
endif()

# serial library is defined in parent directory

add_executable(${PROJECT_NAME}-test unix_serial_tests.cc)
target_link_libraries(${PROJECT_NAME}-test
  PRIVATE
    ${PROJECT_NAME}
    GTest::gtest
    GTest::gtest_main
)
if(NOT APPLE)
  target_link_libraries(${PROJECT_NAME}-test PRIVATE util)
endif()
gtest_discover_tests(${PROJECT_NAME}-test)

if(NOT APPLE)  # these tests are unreliable on macOS
  add_executable(${PROJECT_NAME}-test-timer unit/unix_timer_tests.cc)
  target_link_libraries(${PROJECT_NAME}-test-timer
    PRIVATE
      ${PROJECT_NAME}
      GTest::gtest
      GTest::gtest_main
  )
  gtest_discover_tests(${PROJECT_NAME}-test-timer)
endif()
