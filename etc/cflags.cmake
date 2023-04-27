set (CMAKE_CXX_STANDARD 20)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SANITIZING_FLAGS -fno-sanitize-recover=all -fsanitize=undefined -fsanitize=address)

# ask for more warnings from the compiler
set (CMAKE_BASE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# AppleClang specific flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2")
endif()

# GCC or Clang specific flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2 -Wno-unqualified-std-cast-call")
endif()
