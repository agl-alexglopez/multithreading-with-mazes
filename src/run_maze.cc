#include <iostream>
#include <sstream>
#include <vector>
#include <string_view>
#include <unordered_set>
#include "thread_maze.hh"

const std::unordered_set<std::string> argument_flags = {
    "-r", "-c", "-b", "-s", "-h", "-hunt", "-gather"
};

void set_relevant_arg(Thread_maze::Packaged_args& maze_args,
                      std::string_view flag,
                      std::string_view arg);
void print_usage();

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
                    print_usage();
                    std::abort();
                } else if (*found_arg == "-hunt") {
                    maze_args.game = Thread_maze::Thread_game::hunt;
                } else if (*found_arg == "-gather") {
                    maze_args.game = Thread_maze::Thread_game::gather;
                } else if (*found_arg == "-h") {
                    print_usage();
                    return 0;
                } else {
                    process_current = true;
                    prev_flag = arg;
                }
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
            print_usage();
            std::abort();
        }
    } else if (flag == "-s") {
        if (arg == "dfs") {
            maze_args.solver = Thread_maze::Solver_algorithm::depth_first_search;
        } else if (arg == "bfs") {
            maze_args.solver = Thread_maze::Solver_algorithm::breadth_first_search;
        } else {
            std::cerr << "Invalid solver argument: " << arg << std::endl;
            print_usage();
            std::abort();
        }
    } else if (flag == "-h") {
        print_usage();
        std::abort();
    }
}

void print_usage() {
    std::cout << "┌──────────────────────────────────────────────────────┐\n"
              << "│                                                      │\n"
              << "├────────────Thread Maze Usage Instructions────────────┤\n"
              << "│                                                      │\n"
              << "│  Use flags, followed by arguments, in any order:     │\n"
              << "│                                                      │\n"
              << "│  -r - Rows flag. Set rows for the maze.              │\n"
              << "│      - Any number > 5. Zoom out for larger mazes!    │\n"
              << "│  -c - Columns flag. Set columns for the maze.        │\n"
              << "│      - Any number > 7. Zoom out for larger mazes!    │\n"
              << "│  -b - Builder flag. Set maze building algorithm.     │\n"
              << "│      - random-dfs - Randomized Depth First Search    │\n"
              << "│      - loop-erase - Loop-Erased Random Walk          │\n"
              << "│  -s - Solver flag. Set maze solving algorithm.       │\n"
              << "│      - dfs - Depth First Search                      │\n"
              << "│      - bfs - Breadth First Search                    │\n"
              << "│  -hunt - 4 threads compete to find one finish.       │\n"
              << "│      - No arguments.                                 │\n"
              << "│  -gather - 4 threads gather 4 finish squares.        │\n"
              << "│      - No arguments.                                 │\n"
              << "│  -h  - Help flag. Make this prompt appear.           │\n"
              << "│      - No arguments.                                 │\n"
              << "│                                                      │\n"
              << "│  If any flags are omitted, defaults are used.        │\n"
              << "│                                                      │\n"
              << "│  Examples:                                           │\n"
              << "│  ./run_maze                                          │\n"
              << "│  ./run_maze -r 51 -c 111 -b random-dfs -s bfs -hunt  │\n"
              << "│  ./run_maze -c 111 -s bfs -gather                    │\n"
              << "│                                                      │\n"
              << "└──────────────────────────────────────────────────────┘\n";
    std::cout << std::endl;
}
