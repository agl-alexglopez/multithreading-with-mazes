
add_executable(measure_debug measure.cc)
target_link_libraries(measure_debug PUBLIC
                      speed
                      maze
                      maze_algorithms_debug
                      painters_debug)

#add_executable(measure_sanitized measure.cc)
#target_compile_options(measure_sanitized PUBLIC ${SANITIZING_FLAGS})
#target_link_libraries(measure_sanitized PUBLIC
#                      speed
#                      maze
#                      maze_algorithms_sanitized
#                      painters_sanitized)

add_executable(measure_optimized measure.cc)
target_compile_options(measure_optimized PUBLIC "-O2")
target_link_libraries(measure_optimized PUBLIC
                      speed
                      maze
                      maze_algorithms_optimized
                      painters_optimized)
