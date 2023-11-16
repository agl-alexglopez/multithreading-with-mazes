add_executable(demo demo.cc)
target_link_libraries(demo PUBLIC
                      maze
                      maze_algorithms
                      maze_solvers
                      painters)

