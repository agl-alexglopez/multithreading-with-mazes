
add_executable(run_maze_debug run_maze.cc)
target_link_libraries(run_maze_debug PUBLIC
                      speed
                      maze
                      maze_algorithms_debug
                      maze_solvers_debug)

#add_executable(run_maze_sanitized run_maze.cc)
#target_compile_options(run_maze_sanitized PUBLIC ${SANITIZING_FLAGS})
#target_link_libraries(run_maze_sanitized PUBLIC
#                      speed
#                      maze
#                      maze_algorithms_sanitized
#                      maze_solvers_sanitized)

add_executable(run_maze_optimized run_maze.cc)
target_compile_options(run_maze_optimized PUBLIC "-O2")
target_link_libraries(run_maze_optimized PUBLIC
                      speed
                      maze
                      maze_algorithms_optimized
                      maze_solvers_optimized)

