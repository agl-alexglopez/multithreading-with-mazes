cmake_minimum_required(VERSION 3.24.2)

project(mazes CXX)

if(${PROJECT_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Maze must be built outside its source directory, e.g. `cmake -B build`.")
endif()

find_package(Threads REQUIRED)

include(etc/build_type.cmake)
include(etc/scanners.cmake)

include_directories("${PROJECT_SOURCE_DIR}/run_maze")
include_directories("${PROJECT_SOURCE_DIR}/measure")
include_directories("${PROJECT_SOURCE_DIR}/demo")
include_directories("${PROJECT_SOURCE_DIR}/maze")
include_directories("${PROJECT_SOURCE_DIR}/printers")
include_directories("${PROJECT_SOURCE_DIR}/painters")
include_directories("${PROJECT_SOURCE_DIR}/speed")
include_directories("${PROJECT_SOURCE_DIR}/builders")
include_directories("${PROJECT_SOURCE_DIR}/solvers")

add_subdirectory("${PROJECT_SOURCE_DIR}/run_maze")
add_subdirectory("${PROJECT_SOURCE_DIR}/measure")
add_subdirectory("${PROJECT_SOURCE_DIR}/demo")
add_subdirectory("${PROJECT_SOURCE_DIR}/maze")
add_subdirectory("${PROJECT_SOURCE_DIR}/printers")
add_subdirectory("${PROJECT_SOURCE_DIR}/painters")
add_subdirectory("${PROJECT_SOURCE_DIR}/speed")
add_subdirectory("${PROJECT_SOURCE_DIR}/builders")
add_subdirectory("${PROJECT_SOURCE_DIR}/solvers")
