#include "maze.hh"
#include "maze_algorithms.hh"
#include "maze_solvers.hh"
#include "maze_utilities.hh"

#include <array>
#include <functional>
#include <iostream>
#include <span>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>


constexpr int get_static = 0;
constexpr int get_animated = 1;

struct Flag_arg
{
  std::string_view flag;
  std::string_view arg;
};

struct Maze_runner
{
  Builder::Maze::Maze_args args;

  int builder_getter { get_static };
  Builder::Builder_speed builder_speed {};
  std::tuple<std::function<void(Builder::Maze&)>,std::function<void(Builder::Maze&, Builder::Builder_speed)>> builder
    { Builder::generate_recursive_backtracker_maze, Builder::animate_recursive_backtracker_maze };

  int solver_getter { get_static };
  Solver::Solver_speed solver_speed {};
  std::tuple<std::function<void(Builder::Maze&)>,std::function<void(Builder::Maze&, Solver::Solver_speed)>> solver
    { Solver::solve_with_dfs_thread_hunt, Solver::animate_with_dfs_thread_hunt };
};


void set_relevant_arg( Maze_runner& runner, const Flag_arg& pairs );
void print_usage();

int main( int argc, char** argv )
{
  const std::unordered_set<std::string> argument_flags
    = { "-r", "-c", "-b", "-s", "-h", "-g", "-d", "-m", "-sa", "-ba" };

  const std::unordered_map
    <std::string,
    std::tuple<std::function<void(Builder::Maze&)>,std::function<void(Builder::Maze&, Builder::Builder_speed)>>>
    builder_table = {
    { "rdfs", { Builder::generate_recursive_backtracker_maze, Builder::animate_recursive_backtracker_maze } },
    { "wilson", { Builder::generate_wilson_path_carver_maze, Builder::animate_wilson_path_carver_maze } },
    { "wilson-walls", { Builder::generate_wilson_wall_adder_maze, Builder::animate_wilson_wall_adder_maze } },
    { "fractal", { Builder::generate_recursive_subdivision_maze, Builder::animate_recursive_subdivision_maze } },
    { "kruskal", { Builder::generate_kruskal_maze, Builder::animate_kruskal_maze } },
    { "prim", { Builder::generate_prim_maze, Builder::animate_prim_maze } },
    { "grid", { Builder::generate_grid_maze, Builder::animate_grid_maze } },
    { "arena", { Builder::generate_arena, Builder::animate_arena } },
  };

  const std::unordered_map<std::string, Builder::Maze::Maze_modification> modification_table = {
    { "none", Builder::Maze::Maze_modification::none },
    { "cross", Builder::Maze::Maze_modification::add_cross },
    { "x", Builder::Maze::Maze_modification::add_x },
  };

  const std::unordered_map<std::string, std::tuple<std::function<void(Builder::Maze&)>,std::function<void(Builder::Maze&, Solver::Solver_speed)>>> solver_table = {
    { "dfs-hunt", { Solver::solve_with_dfs_thread_hunt, Solver::animate_with_dfs_thread_hunt } },
    { "dfs-gather", { Solver::solve_with_dfs_thread_gather, Solver::animate_with_dfs_thread_gather } },
    { "dfs-corners", { Solver::solve_with_dfs_thread_corners, Solver::animate_with_dfs_thread_corners } },
    { "rdfs-hunt", { Solver::solve_with_randomized_dfs_thread_hunt, Solver::animate_with_randomized_dfs_thread_hunt } },
    { "rdfs-gather", { Solver::solve_with_randomized_dfs_thread_gather, Solver::animate_with_randomized_dfs_thread_gather } },
    { "rdfs-corners", { Solver::solve_with_randomized_dfs_thread_corners, Solver::animate_with_randomized_dfs_thread_corners } },
    { "bfs-hunt", { Solver::solve_with_bfs_thread_hunt, Solver::animate_with_bfs_thread_hunt } },
    { "bfs-gather", { Solver::solve_with_bfs_thread_gather, Solver::animate_with_bfs_thread_gather } },
    { "bfs-corners", { Solver::solve_with_bfs_thread_corners, Solver::animate_with_bfs_thread_corners } },
  };

  const std::unordered_map<std::string, Builder::Maze::Maze_style> style_table = {
    { "sharp", Builder::Maze::Maze_style::sharp },
    { "round", Builder::Maze::Maze_style::round },
    { "doubles", Builder::Maze::Maze_style::doubles },
    { "bold", Builder::Maze::Maze_style::bold },
    { "contrast", Builder::Maze::Maze_style::contrast },
    { "spikes", Builder::Maze::Maze_style::spikes },
  };

  const std::unordered_map<std::string, Solver::Solver_speed> solver_animation_table = {
    { "0", Solver::Solver_speed::instant },
    { "1", Solver::Solver_speed::speed_1 },
    { "2", Solver::Solver_speed::speed_2 },
    { "3", Solver::Solver_speed::speed_3 },
    { "4", Solver::Solver_speed::speed_4 },
    { "5", Solver::Solver_speed::speed_5 },
    { "6", Solver::Solver_speed::speed_6 },
    { "7", Solver::Solver_speed::speed_7 },
  };

  const std::unordered_map<std::string, Builder::Builder_speed> builder_animation_table = {
    { "0", Builder::Builder_speed::instant },
    { "1", Builder::Builder_speed::speed_1 },
    { "2", Builder::Builder_speed::speed_2 },
    { "3", Builder::Builder_speed::speed_3 },
    { "4", Builder::Builder_speed::speed_4 },
    { "5", Builder::Builder_speed::speed_5 },
    { "6", Builder::Builder_speed::speed_6 },
    { "7", Builder::Builder_speed::speed_7 },
  };

  Maze_runner runner;
  if ( argc > 1 ) {
    const auto args = std::span( argv, argc );
    bool process_current = false;
    std::string_view prev_flag = {};
    for ( auto const* arg : args ) {
      if ( process_current ) {
      Flag_arg pairs { prev_flag, {arg} };
      if ( pairs.flag == "-r" ) {
        runner.args.odd_rows = std::stoi( pairs.arg.data() );
        if ( runner.args.odd_rows % 2 == 0 ) {
          runner.args.odd_rows++;
        }
        if ( runner.args.odd_rows < 7 ) {
          std::cerr << "Smallest maze possible is 7x7" << std::endl;
          std::abort();
        }
      } else if ( pairs.flag == "-c" ) {
        runner.args.odd_cols = std::stoi( pairs.arg.data() );
        if ( runner.args.odd_cols % 2 == 0 ) {
          runner.args.odd_cols++;
        }
        if ( runner.args.odd_cols < 7 ) {
          std::cerr << "Smallest maze possible is 3x7" << std::endl;
          std::abort();
        }
      } else if ( pairs.flag == "-b" ) {
        const auto found = builder_table.find( pairs.arg.data() );
        if ( found == builder_table.end() ) {
          std::cerr << "Invalid builder argument: " << pairs.arg << std::endl;
          print_usage();
          std::abort();
        }
        runner.builder = found->second;
      } else if ( pairs.flag == "-m" ) {
        // todo
      } else if ( pairs.flag == "-s" ) {
        const auto found = solver_table.find( pairs.arg.data() );
        if ( found == solver_table.end() ) {
          std::cerr << "Invalid solver argument: " << pairs.arg << std::endl;
          print_usage();
          std::abort();
        }
        runner.solver = found->second;
      } else if ( pairs.flag == "-d" ) {
        const auto found = style_table.find( pairs.arg.data() );
        if ( found == style_table.end() ) {
          std::cerr << "Invalid drawing argument: " << pairs.arg << std::endl;
          print_usage();
          std::abort();
        }
        runner.args.style = found->second;
      } else if ( pairs.flag == "-sa" ) {
        const auto found = solver_animation_table.find( pairs.arg.data() );
        if ( found == solver_animation_table.end() ) {
          std::cerr << "Invalid solver animation argument: " << pairs.arg << std::endl;
          print_usage();
          std::abort();
        }
        runner.solver_speed = found->second;
        runner.solver_getter = get_animated;
      } else if ( pairs.flag == "-ba" ) {
        const auto found = builder_animation_table.find( pairs.arg.data() );
        if ( found == builder_animation_table.end() ) {
          std::cerr << "Invalid builder animation argument: " << pairs.arg << std::endl;
          print_usage();
          std::abort();
        }
        runner.builder_speed = found->second;
        runner.builder_getter = get_animated;
      } else {
        std::cerr << "Invalid pairs.flag past the first defense? " << pairs.flag << std::endl;
        print_usage();
        std::abort();
      }
        process_current = false;
      } else {
        const auto& found_arg = argument_flags.find( arg );
        if ( found_arg == argument_flags.end() ) {
          std::cerr << "Invalid argument flag: " << arg << std::endl;
          print_usage();
          std::abort();
        }
        if ( *found_arg == "-h" ) {
          print_usage();
          return 0;
        }
        process_current = true;
        prev_flag = arg;
      }
    }
  }
  Builder::Maze maze( runner.args );
  return 0;
}

void print_usage()
{
  std::cout << "┌──────────────────────────────────────────────────────┐\n"
            << "│                                                      │\n"
            << "├────────────Thread Maze Usage Instructions────────────┤\n"
            << "│                                                      │\n"
            << "│  Use flags, followed by arguments, in any order:     │\n"
            << "│                                                      │\n"
            << "│  -r Rows flag. Set rows for the maze.                │\n"
            << "│      Any number > 7. Zoom out for larger mazes!      │\n"
            << "│  -c Columns flag. Set columns for the maze.          │\n"
            << "│      Any number > 7. Zoom out for larger mazes!      │\n"
            << "│  -b Builder flag. Set maze building algorithm.       │\n"
            << "│      rdfs - Randomized Depth First Search.           │\n"
            << "│      kruskal - Randomized Kruskal's algorithm.       │\n"
            << "│      prim - Randomized Prim's algorithm.             │\n"
            << "│      wilson - Loop-Erased Random Path Carver.        │\n"
            << "│      wilson-walls - Loop-Erased Random Wall Adder.   │\n"
            << "│      fractal - Randomized recursive subdivision.     │\n"
            << "│      grid - A random grid pattern.                   │\n"
            << "│      arena - Open floor with no walls.               │\n"
            << "│  -m Modification flag. Add shortcuts to the maze.    │\n"
            << "│      cross - Add crossroads through the center.      │\n"
            << "│      x - Add an x of crossing paths through center.  │\n"
            << "│  -s Solver flag. Set maze solving algorithm.         │\n"
            << "│      dfs - Depth First Search                        │\n"
            << "│      rdfs - Randomized Depth First Search            │\n"
            << "│      bfs - Breadth First Search                      │\n"
            << "│  -g Game flag. Set the game for the threads to play. │\n"
            << "│      hunt - 4 threads race to find one finish.       │\n"
            << "│      gather - 4 threads gather 4 finish squares.     │\n"
            << "│      corners - 4 threads race to the center.         │\n"
            << "│  -d Draw flag. Set the line style for the maze.      │\n"
            << "│      sharp - The default straight lines.             │\n"
            << "│      round - Rounded corners.                        │\n"
            << "│      doubles - Sharp double lines.                   │\n"
            << "│      bold - Thicker straight lines.                  │\n"
            << "│      contrast - Full block width and height walls.   │\n"
            << "│      spikes - Connected lines with spikes.           │\n"
            << "│  -sa Solver Animation flag. Watch the maze solution. │\n"
            << "│      Any number 1-7. Speed increases with number.    │\n"
            << "│  -ba Builder Animation flag. Watch the maze build.   │\n"
            << "│      Any number 1-7. Speed increases with number.    │\n"
            << "│  -h Help flag. Make this prompt appear.              │\n"
            << "│      No arguments.                                   │\n"
            << "│                                                      │\n"
            << "│  If any flags are omitted, defaults are used.        │\n"
            << "│                                                      │\n"
            << "│  Examples:                                           │\n"
            << "│  ./run_maze                                          │\n"
            << "│  ./run_maze -r 51 -c 111 -b random-dfs -s bfs -hunt  │\n"
            << "│  ./run_maze -c 111 -s bfs -g gather                  │\n"
            << "│  ./run_maze -s bfs -g corners -d round -b fractal    │\n"
            << "│  ./run_maze -s dfs -ba 4 -sa 5 -b kruskal -m x       │\n"
            << "│                                                      │\n"
            << "└──────────────────────────────────────────────────────┘\n";
  std::cout << std::endl;
}
