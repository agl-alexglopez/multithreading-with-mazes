file(GLOB BUILDER_SOURCES "*.cc")
add_library(maze_builders STATIC ${BUILDER_SOURCES})

add_library(maze_builders_sanitized EXCLUDE_FROM_ALL ${BUILDER_SOURCES})
target_compile_options(maze_builders_sanitized PUBLIC ${SANITIZING_FLAGS})

add_library(maze_builders_optimized EXCLUDE_FROM_ALL ${BUILDER_SOURCES})
target_compile_options(maze_builders_optimized PUBLIC "-O2")
