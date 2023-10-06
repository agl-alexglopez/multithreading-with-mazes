add_library(rgb rgb.hh rgb.cc)
target_link_libraries(rgb maze)

add_library(painters_debug painters.hh distance.cc runs.cc)
target_link_libraries(painters_debug PRIVATE print_utilities rgb maze speed my_queue Threads::Threads)

add_library(painters_sanitized painters.hh distance.cc runs.cc)
target_compile_options(painters_sanitized PUBLIC ${SANITIZING_FLAGS})
target_link_libraries(painters_sanitized PRIVATE print_utilities rgb maze speed my_queue Threads::Threads)

add_library(painters_optimized painters.hh distance.cc runs.cc)
target_compile_options(painters_optimized PUBLIC "-O2")
target_link_libraries(painters_optimized PRIVATE print_utilities rgb maze speed my_queue Threads::Threads)
