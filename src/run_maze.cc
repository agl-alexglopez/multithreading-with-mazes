#include <iostream>
#include <sstream>
#include <vector>
#include <string_view>
#include <unordered_set>
#include "thread_maze.hh"

const std::unordered_set<std::string> argument_flags = {
    "-r", "-c", "-b", "-s", "-h", "-g", "-d", "-m",
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
                }
                if (*found_arg == "-h") {
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
        if (maze_args.odd_rows % 2 == 0) {
            maze_args.odd_rows++;
        }
        if (maze_args.odd_rows < 7 || maze_args.odd_cols < 7) {
            std::cerr << "Smallest maze possible is 7x7" << std::endl;
            std::abort();
        }
    } else if (flag == "-c") {
        maze_args.odd_cols = std::stoull(arg.data());
        if (maze_args.odd_cols % 2 == 0) {
            maze_args.odd_cols++;
        }
        if (maze_args.odd_cols < 7) {
            std::cerr << "Smallest maze possible is 3x7" << std::endl;
            std::abort();
        }
    } else if (flag == "-b") {
        if (arg == "random-df") {
            maze_args.builder = Thread_maze::Builder_algorithm::randomized_depth_first;
        } else if (arg == "loop-erase"){
            maze_args.builder = Thread_maze::Builder_algorithm::randomized_loop_erased;
        } else if (arg == "random-grid"){
            maze_args.builder = Thread_maze::Builder_algorithm::randomized_grid;
        } else if (arg == "arena") {
            maze_args.builder = Thread_maze::Builder_algorithm::arena;
        } else {
            std::cerr << "Invalid builder argument: " << arg << std::endl;
            print_usage();
            std::abort();
        }
    } else if (flag == "-m") {
        if (arg == "cross") {
            maze_args.modification = Thread_maze::Maze_modification::add_cross;
        } else if (arg == "x") {
            maze_args.modification = Thread_maze::Maze_modification::add_x;
        } else {
            std::cerr << "Invalid modification argument: " << arg << std::endl;
            print_usage();
            std::abort();
        }
    } else if (flag == "-s") {
        if (arg == "dfs") {
            maze_args.solver = Thread_maze::Solver_algorithm::depth_first_search;
        } else if (arg == "dfs-random") {
            maze_args.solver = Thread_maze::Solver_algorithm::randomized_depth_first_search;
        } else if (arg == "bfs") {
            maze_args.solver = Thread_maze::Solver_algorithm::breadth_first_search;
        } else {
            std::cerr << "Invalid solver argument: " << arg << std::endl;
            print_usage();
            std::abort();
        }
    } else if (flag == "-g") {
        if (arg == "hunt") {
            maze_args.game = Thread_maze::Maze_game::hunt;
        } else if (arg == "gather") {
            maze_args.game = Thread_maze::Maze_game::gather;
        } else {
            std::cerr << "Invalid solver argument: " << arg << std::endl;
            print_usage();
            std::abort();
        }
    } else if (flag == "-d") {
        if (arg == "standard") {
            maze_args.style = Thread_maze::Maze_style::standard;
        } else if (arg == "round") {
            maze_args.style = Thread_maze::Maze_style::rounded;
        } else {
            std::cerr << "Invalid solver argument: " << arg << std::endl;
            print_usage();
            std::abort();
        }
    }
}

void print_usage() {
    std::cout << "┌──────────────────────────────────────────────────────┐\n"
              << "│                                                      │\n"
              << "├────────────Thread Maze Usage Instructions────────────┤\n"
              << "│                                                      │\n"
              << "│  Use flags, followed by arguments, in any order:     │\n"
              << "│                                                      │\n"
              << "│  -r Rows flag. Set rows for the maze.                │\n"
              << "│      Any number > 5. Zoom out for larger mazes!      │\n"
              << "│  -c Columns flag. Set columns for the maze.          │\n"
              << "│      Any number > 7. Zoom out for larger mazes!      │\n"
              << "│  -b Builder flag. Set maze building algorithm.       │\n"
              << "│      random-df - Randomized Depth First Search.      │\n"
              << "│      loop-erase - Loop-Erased Random Walk.           │\n"
              << "│      random-grid - A random grid pattern.            │\n"
              << "│      arena - Open floor with no walls.               │\n"
              << "│  -m Modification flag. Add shortcuts to the maze.    │\n"
              << "│      cross - Add crossroads through the center.      │\n"
              << "│      x - Add an x of crossing paths through center.  │\n"
              << "│  -s Solver flag. Set maze solving algorithm.         │\n"
              << "│      dfs - Depth First Search                        │\n"
              << "│      dfs-random - Randomized Breadth First Search    │\n"
              << "│      bfs - Breadth First Search                      │\n"
              << "│  -g Game flag. Set the game for the threads to play. │\n"
              << "│      hunt - 4 threads race to find one finish.       │\n"
              << "│      gather - 4 threads gather 4 finish squares.     │\n"
              << "│  -d Draw flag. Set the line style for the maze.      │\n"
              << "│      standard - The default straight lines.          │\n"
              << "│      round - Rounded corners.                        │\n"
              << "│  -h Help flag. Make this prompt appear.              │\n"
              << "│      No arguments.                                   │\n"
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
