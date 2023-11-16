set (CMAKE_CXX_STANDARD 20)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# AppleClang specific flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3 -Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2 -fno-sanitize-recover=all -fsanitize=undefined -fsanitize=address)
        add_link_options(-fno-sanitize-recover=all -fsanitize=undefined -fsanitize=address)
    else()
        add_compile_options(-Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2)
    endif()
endif()

# GCC or Clang specific flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Maybe this works, idk. Not getting any warnings.
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3 -Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2 -fno-sanitize-recover=all -fsanitize=undefined -fsanitize=address)
        add_link_options(-fno-sanitize-recover=all -fsanitize=undefined -fsanitize=address)
    else()
        add_compile_options(-Wall -Wpedantic -Wextra -Weffc++ -Werror -Wshadow -Wpointer-arith -Wcast-qual -Wformat=2)
    endif()
endif()
