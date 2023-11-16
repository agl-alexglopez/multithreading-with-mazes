add_executable(measure measure.cc)
target_link_libraries(measure PUBLIC
                      speed
                      maze
                      maze_algorithms
                      painters)
