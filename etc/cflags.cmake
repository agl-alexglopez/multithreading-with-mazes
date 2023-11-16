set (CMAKE_CXX_STANDARD 20)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)


# ask for more warnings from the compiler
set(CMAKE_BASE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# AppleClang specific flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set (CMAKE_CXX_FLAGS "${CMAKE_BASE_CXX_FLAGS} -g3 -Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2")
endif()

# GCC or Clang specific flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Maybe this works, idk. Not getting any warnings.
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set (CMAKE_CXX_FLAGS "${CMAKE_BASE_CXX_FLAGS} -g3 -Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2 -Wno-unqualified-std-cast-call -fno-sanitize-recover=all -fsanitize=undefined -fsanitize=address")
        add_link_options(-fno-sanitize-recover=all -fsanitize=undefined -fsanitize=address)
    else()
        set (CMAKE_CXX_FLAGS "${CMAKE_BASE_CXX_FLAGS} -Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2 -Wno-unqualified-std-cast-call")
    endif()
endif()
