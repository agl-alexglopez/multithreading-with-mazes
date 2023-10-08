add_library(disjoint_set disjoint_set.hh disjoint_set.cc)

add_library(maze_utilities maze_utilities.hh maze_utilities.cc)
target_link_libraries(maze_utilities PRIVATE maze speed)

add_library(maze_algorithms_debug maze_algorithms.hh arena.cc grid.cc kruskal.cc prim.cc recursive_backtracker.cc recursive_subdivision.cc wilson_path_carver.cc wilson_wall_adder.cc eller.cc mods.cc)
target_link_libraries(maze_algorithms_debug PRIVATE maze disjoint_set maze_utilities print_utilities speed)

add_library(maze_algorithms_sanitized maze_algorithms.hh arena.cc grid.cc kruskal.cc prim.cc recursive_backtracker.cc recursive_subdivision.cc wilson_path_carver.cc wilson_wall_adder.cc eller.cc mods.cc)
target_link_libraries(maze_algorithms_sanitized PRIVATE maze disjoint_set maze_utilities print_utilities speed)
target_compile_options(maze_algorithms_sanitized PUBLIC ${SANITIZING_FLAGS})

add_library(maze_algorithms maze_algorithms.hh arena.cc grid.cc kruskal.cc prim.cc recursive_backtracker.cc recursive_subdivision.cc wilson_path_carver.cc wilson_wall_adder.cc eller.cc mods.cc)
target_link_libraries(maze_algorithms PRIVATE maze disjoint_set maze_utilities print_utilities speed)
target_compile_options(maze_algorithms PUBLIC "-O2")
