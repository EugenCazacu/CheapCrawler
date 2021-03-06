cmake_minimum_required (VERSION 3.10)
project (CheapCrawler)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_compile_options("-Wall" "-Werror")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif ()

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

find_package(GTest MODULE REQUIRED)
find_package(GMock MODULE REQUIRED)
find_package(boost_program_options MODULE REQUIRED)
find_package(boost_asio            MODULE REQUIRED)
find_package(libcurl               MODULE REQUIRED)

# These packages are not available with conan yet
find_package(PkgConfig REQUIRED)
pkg_check_modules(URIPARSER REQUIRED IMPORTED_TARGET liburiparser)

add_subdirectory(src)

option(CHEAP_CRAWLER_RUN_TESTS "If enabled CheapCrawler tests will be run." OFF)
if(${CHEAP_CRAWLER_RUN_TESTS})
  # enable CTest.  This will set BUILD_TESTING to ON unless otherwise specified on the command line
  include( CTest )
  add_subdirectory(test)
endif()

