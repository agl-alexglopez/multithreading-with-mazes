
#add_executable(run_maze_sanitized run_maze.cc)
#target_compile_options(run_maze_sanitized PUBLIC ${SANITIZING_FLAGS})
#target_link_libraries(run_maze_sanitized PUBLIC
#                      maze
#                      maze_algorithms_sanitized
#                      maze_solvers_sanitized
#                      painters_sanitized)

add_executable(demo_debug demo.cc)
target_link_libraries(demo_debug PUBLIC
                      maze
                      maze_algorithms_debug
                      maze_solvers_debug
                      painters_debug)

add_executable(demo demo.cc)
target_compile_options(demo PUBLIC "-O2")
target_link_libraries(demo PUBLIC
                      maze
                      maze_algorithms
                      maze_solvers
                      painters)

