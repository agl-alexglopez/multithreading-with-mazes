file(GLOB BUILDER_SOURCES "*.cc")
add_library(maze_solvers STATIC ${BUILDER_SOURCES})

add_library(maze_solvers_sanitized EXCLUDE_FROM_ALL ${BUILDER_SOURCES})
target_compile_options(maze_solvers_sanitized PUBLIC ${SANITIZING_FLAGS})

add_library(maze_solvers_optimized EXCLUDE_FROM_ALL ${BUILDER_SOURCES})
target_compile_options(maze_solvers_optimized PUBLIC "-O2")
