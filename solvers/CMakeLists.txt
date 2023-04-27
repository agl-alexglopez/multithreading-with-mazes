add_library(my_queue INTERFACE my_queue.hh)

add_library(maze_solvers_debug maze_solvers.hh bfs_threads.cc  dfs_threads.cc  maze_solvers.cc   randomized_dfs_threads.cc)
target_link_libraries( maze_solvers_debug PRIVATE my_queue Threads::Threads)

add_library(maze_solvers_sanitized maze_solvers.hh bfs_threads.cc  dfs_threads.cc  maze_solvers.cc   randomized_dfs_threads.cc)
target_link_libraries( maze_solvers_sanitized PRIVATE my_queue Threads::Threads)
target_compile_options(maze_solvers_sanitized PUBLIC ${SANITIZING_FLAGS})

add_library(maze_solvers_optimized maze_solvers.hh bfs_threads.cc  dfs_threads.cc  maze_solvers.cc   randomized_dfs_threads.cc)
target_link_libraries( maze_solvers_optimized PRIVATE my_queue Threads::Threads)
target_compile_options(maze_solvers_optimized PUBLIC "-O2")
