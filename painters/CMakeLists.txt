add_library(rgb rgb.hh rgb.cc)
target_link_libraries(rgb maze)

add_library(painters painters.hh distance.cc runs.cc)
target_link_libraries(painters PRIVATE print_utilities rgb maze speed my_queue Threads::Threads)
