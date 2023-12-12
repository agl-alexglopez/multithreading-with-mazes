cmake_minimum_required(VERSION 3.24.2)
project(mazes CXX)

if(${PROJECT_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Maze must be built outside its source directory, e.g. `cmake -B build`.")
endif()

find_package(Threads REQUIRED)

include(etc/build_type.cmake)
include(etc/scanners.cmake)

add_library(labyrinth)
target_sources(labyrinth
  PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS 
      ${PROJECT_SOURCE_DIR}/speed
      ${PROJECT_SOURCE_DIR}/maze
      ${PROJECT_SOURCE_DIR}/printers
      ${PROJECT_SOURCE_DIR}/builders
      ${PROJECT_SOURCE_DIR}/solvers
      ${PROJECT_SOURCE_DIR}/painters
    FILES
      ${PROJECT_SOURCE_DIR}/maze/maze.cc
      ${PROJECT_SOURCE_DIR}/speed/speed.cc
      ${PROJECT_SOURCE_DIR}/printers/printers.cc
      ${PROJECT_SOURCE_DIR}/builders/maze_utilities.cc
      ${PROJECT_SOURCE_DIR}/builders/disjoint_set.cc
      ${PROJECT_SOURCE_DIR}/builders/arena.cc
      ${PROJECT_SOURCE_DIR}/builders/grid.cc
      ${PROJECT_SOURCE_DIR}/builders/eller.cc
      ${PROJECT_SOURCE_DIR}/builders/prim.cc
      ${PROJECT_SOURCE_DIR}/builders/kruskal.cc
      ${PROJECT_SOURCE_DIR}/builders/recursive_subdivision.cc
      ${PROJECT_SOURCE_DIR}/builders/recursive_backtracker.cc
      ${PROJECT_SOURCE_DIR}/builders/wilson_path_carver.cc
      ${PROJECT_SOURCE_DIR}/builders/wilson_wall_adder.cc
      ${PROJECT_SOURCE_DIR}/builders/mods.cc
      ${PROJECT_SOURCE_DIR}/solvers/my_queue.cc
      ${PROJECT_SOURCE_DIR}/solvers/solver_utilities.cc
      ${PROJECT_SOURCE_DIR}/solvers/dfs_threads.cc
      ${PROJECT_SOURCE_DIR}/solvers/darkdfs_threads.cc
      ${PROJECT_SOURCE_DIR}/solvers/bfs_threads.cc
      ${PROJECT_SOURCE_DIR}/solvers/darkbfs_threads.cc
      ${PROJECT_SOURCE_DIR}/solvers/randomized_dfs_threads.cc
      ${PROJECT_SOURCE_DIR}/solvers/darkrandomized_dfs_threads.cc
      ${PROJECT_SOURCE_DIR}/solvers/floodfs_threads.cc
      ${PROJECT_SOURCE_DIR}/solvers/darkfloodfs_threads.cc
      ${PROJECT_SOURCE_DIR}/painters/rgb.cc
      ${PROJECT_SOURCE_DIR}/painters/runs.cc
      ${PROJECT_SOURCE_DIR}/painters/distance.cc
)
target_link_libraries(labyrinth PRIVATE Threads::Threads)
