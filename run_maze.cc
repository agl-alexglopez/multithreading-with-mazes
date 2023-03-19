#include <iostream>
#include <vector>
#include <string_view>
#include <unordered_set>
#include "thread_maze.hh"

const std::unordered_set<std::string> maze_params = {
    "-r", "-c", "-b", "-s",
};



int main(int argc, char **argv) {
    if (argc == 1 || argc > 5) {
        Thread_maze maze(51, 171);
        maze.solve_with_dfs_threads();
    } else {
        const std::vector<std::string_view> args(argv + 1, argv + argc);
        for (const auto& arg : args) {
            const auto& found_arg = maze_params.find(arg.data());
            if (found_arg == maze_params.end()) {
                std::cerr << "Invalid argument parameter: " << arg << std::endl;
                std::abort();
            }
        }
    }

    return 0;
}

