
add_executable(run_maze_debug run_maze.cc)
target_link_libraries(run_maze_debug PUBLIC
                      maze
                      maze_algorithms_debug
                      maze_solvers_debug)

#add_executable(run_maze_sanitized run_maze.cc)
#target_compile_options(run_maze_sanitized PUBLIC ${SANITIZING_FLAGS})
#target_link_libraries(run_maze_sanitized PUBLIC
#                      maze
#                      maze_algorithms_sanitized
#                      maze_solvers_sanitized)

add_executable(run_maze_optimized run_maze.cc)
target_compile_options(run_maze_optimized PUBLIC "-O2")
target_link_libraries(run_maze_optimized PUBLIC
                      maze
                      maze_algorithms_optimized
                      maze_solvers_optimized)

add_executable(demo_debug demo.cc)
target_link_libraries(demo_debug PUBLIC
                      maze
                      maze_algorithms_debug
                      maze_solvers_debug)

add_executable(demo_optimized demo.cc)
target_compile_options(demo_optimized PUBLIC "-O2")
target_link_libraries(demo_optimized PUBLIC
                      maze
                      maze_algorithms_optimized
                      maze_solvers_optimized)

