add_executable(run_maze run_maze.cc)
target_link_libraries(run_maze PUBLIC
                      speed
                      maze
                      maze_algorithms
                      maze_solvers)

