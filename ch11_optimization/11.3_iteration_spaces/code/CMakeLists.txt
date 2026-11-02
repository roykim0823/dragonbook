# CMake alternative to ../run.sh for the numbered programs in src/. Run from the section folder:
#
#   cmake -S code -B build && cmake --build build && ctest --test-dir build --output-on-failure
#   ctest --test-dir build -R 05          # just the program numbered 05
#
# Every src/NN_*.cpp becomes an executable and a test of the same name. A test fails if the
# program prints "MISMATCH" or a non-zero mismatch / bound-loss count. run.sh does the same
# without CMake and prints every program's output.

cmake_minimum_required(VERSION 3.16)
project(dragonbook_11_3_iteration_spaces LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")          # same flags as the build line in each .cpp
add_compile_options(-Wall -Wextra)
include_directories(${CMAKE_SOURCE_DIR}/include)   # the shared headers (code/include)

enable_testing()

file(GLOB PROGRAMS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/[0-9][0-9]_*.cpp")
list(SORT PROGRAMS)
foreach(src ${PROGRAMS})
  get_filename_component(prog ${src} NAME_WE)
  add_executable(${prog} ${src})
  add_test(NAME ${prog} COMMAND ${prog} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  set_tests_properties(${prog} PROPERTIES
    FAIL_REGULAR_EXPRESSION "MISMATCH;[1-9][0-9]* mismatches;[1-9][0-9]* bound losses")
endforeach()
