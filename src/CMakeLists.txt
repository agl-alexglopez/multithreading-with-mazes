
add_library(my_queue INTERFACE my_queue.hh)
add_library(thread_maze thread_maze.hh thread_maze.cc)
target_link_libraries(thread_maze PUBLIC my_queue)
add_library(thread_solvers thread_solvers.hh thread_solvers.cc)
target_link_libraries(thread_solvers PUBLIC thread_maze Threads::Threads)

add_executable(run_maze_debug run_maze.cc)
target_link_libraries(run_maze_debug PUBLIC
                      thread_maze
                      thread_solvers)

#add_executable(run_maze_sanitized run_maze.cc)
#target_compile_options(run_maze_sanitized PUBLIC ${SANITIZING_FLAGS})
#target_link_libraries(run_maze_sanitized PUBLIC
#                      thread_maze
#                      thread_solvers)

add_executable(run_maze_optimized run_maze.cc)
target_compile_options(run_maze_optimized PUBLIC "-O2")
target_link_libraries(run_maze_optimized PUBLIC
                      thread_maze
                      thread_solvers)
