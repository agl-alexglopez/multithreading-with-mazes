add_library(disjoint_set disjoint_set.hh disjoint_set.cc)

add_library(maze_utilities maze_utilities.hh maze_utilities.cc)
target_link_libraries(maze_utilities PRIVATE maze speed)

add_library(maze_algorithms maze_algorithms.hh arena.cc grid.cc kruskal.cc prim.cc recursive_backtracker.cc recursive_subdivision.cc wilson_path_carver.cc wilson_wall_adder.cc eller.cc mods.cc)
target_link_libraries(maze_algorithms PRIVATE maze disjoint_set maze_utilities print_utilities speed)
