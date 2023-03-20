#include <iostream>
#include <sstream>
#include <vector>
#include <string_view>
#include <unordered_set>
#include "thread_maze.hh"

const std::unordered_set<std::string> argument_flags = {
    "-r", "-c", "-b", "-s",
};

void set_relevant_arg(Thread_maze::Packaged_args& maze_args,
                      std::string_view flag,
                      std::string_view arg);

int main(int argc, char **argv) {
    Thread_maze::Packaged_args maze_args = {};
    if (argc == 1) {
        Thread_maze maze(maze_args);
        maze.solve_maze();
    } else {
        const std::vector<std::string_view> args(argv + 1, argv + argc);
        bool process_current = false;
        std::string_view prev_flag = {};
        for (const std::string_view& arg : args) {
            if (process_current) {
                set_relevant_arg(maze_args, prev_flag, arg);
                process_current = false;
            } else {
                const auto& found_arg = argument_flags.find(arg.data());
                if (found_arg == argument_flags.end()) {
                    std::cerr << "Invalid argument parameter: " << arg << std::endl;
                    std::abort();
                }
                process_current = true;
                prev_flag = arg;
            }
        }
        Thread_maze maze(maze_args);
        maze.solve_maze();
    }
    return 0;
}

void set_relevant_arg(Thread_maze::Packaged_args& maze_args,
                      std::string_view flag,
                      std::string_view arg) {
    if (flag == "-r") {
        maze_args.odd_rows = std::stoull(arg.data());
    } else if (flag == "-c") {
        maze_args.odd_cols = std::stoull(arg.data());
    } else if (flag == "-b") {
        if (arg == "random-df") {
            maze_args.builder = Thread_maze::Builder_algorithm::randomized_depth_first;
        } else if (arg == "loop-erase"){
            maze_args.builder = Thread_maze::Builder_algorithm::randomized_loop_erased;
        } else {
            std::cerr << "Invalid builder argument: " << arg << std::endl;
            std::abort();
        }
    } else if (flag == "-s") {
        if (arg == "dfs") {
            maze_args.solver = Thread_maze::Solver_algorithm::depth_first_search;
        } else if (arg == "bfs") {
            maze_args.solver = Thread_maze::Solver_algorithm::breadth_first_search;
        } else {
            std::cerr << "Invalid solver argument: " << arg << std::endl;
            std::abort();
        }
    }
}
