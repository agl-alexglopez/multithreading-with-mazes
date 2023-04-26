add_library(maze_utilities maze_utilities.hh maze_utilities.cc)

add_library(disjoint_set disjoint_set.hh disjoint_set.cc)

add_library(maze_algorithms_debug maze_algorithms.hh arena.cc grid.cc kruskal.cc maze.cc maze.hh prim.cc recursive_backtracker.cc recursive_subdivision.cc wilson_path_carver.cc wilson_wall_adder.cc)
target_link_libraries(maze_algorithms_debug PRIVATE disjoint_set maze_utilities)

add_library(maze_algorithms_sanitized maze_algorithms.hh arena.cc grid.cc kruskal.cc maze.cc maze.hh prim.cc recursive_backtracker.cc recursive_subdivision.cc wilson_path_carver.cc wilson_wall_adder.cc)
target_link_libraries(maze_algorithms_sanitized PRIVATE disjoint_set maze_utilities)
target_compile_options(maze_algorithms_sanitized PUBLIC ${SANITIZING_FLAGS})

add_library(maze_algorithms_optimized maze_algorithms.hh arena.cc grid.cc kruskal.cc maze.cc maze.hh prim.cc recursive_backtracker.cc recursive_subdivision.cc wilson_path_carver.cc wilson_wall_adder.cc)
target_link_libraries(maze_algorithms_optimized PRIVATE disjoint_set maze_utilities)
target_compile_options(maze_algorithms_optimized PUBLIC "-O2")
