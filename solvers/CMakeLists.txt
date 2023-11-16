add_library(my_queue INTERFACE)
add_library(solver_utilities solver_utilities.hh solver_utilities.cc)
target_link_libraries(solver_utilities PRIVATE maze speed)

add_library(maze_solvers maze_solvers.hh bfs_threads.cc dfs_threads.cc floodfs_threads.cc randomized_dfs_threads.cc darkdfs_threads.cc darkbfs_threads.cc darkfloodfs_threads.cc darkrandomized_dfs_threads.cc)
target_link_libraries( maze_solvers PRIVATE speed my_queue print_utilities solver_utilities Threads::Threads)
