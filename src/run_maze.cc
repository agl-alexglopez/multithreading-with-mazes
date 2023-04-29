#include "maze.hh"
#include "maze_algorithms.hh"
#include "maze_solvers.hh"
#include "maze_utilities.hh"

#include <array>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <random>
#include <span>
#include <string_view>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using Build_function = std::tuple<std::function<void( Builder::Maze& )>,
                                  std::function<void( Builder::Maze&, Builder::Builder_speed )>>;

using Solve_function
  = std::tuple<std::function<void( Builder::Maze& )>, std::function<void( Builder::Maze&, Solver::Solver_speed )>>;

constexpr int static_image = 0;
constexpr int animated_playback = 1;

struct Flag_arg
{
  std::string_view flag;
  std::string_view arg;
};

struct Maze_runner
{
  Builder::Maze::Maze_args args;

  int builder_view { static_image };
  Builder::Builder_speed builder_speed {};
  Build_function builder { Builder::generate_recursive_backtracker_maze,
                           Builder::animate_recursive_backtracker_maze };

  int modification_getter { static_image };
  std::optional<Build_function> modder {};

  int solver_view { static_image };
  Solver::Solver_speed solver_speed {};
  Solve_function solver { Solver::solve_with_dfs_thread_hunt, Solver::animate_with_dfs_thread_hunt };
  Maze_runner() : args {} {}
};

struct Lookup_tables
{
  const std::unordered_set<std::string> argument_flags;
  const std::unordered_map<std::string, Build_function> builder_table;
  const std::unordered_map<std::string, Build_function> modification_table;
  const std::unordered_map<std::string, Solve_function> solver_table;
  const std::unordered_map<std::string, Builder::Maze::Maze_style> style_table;
  const std::unordered_map<std::string, Solver::Solver_speed> solver_animation_table;
  const std::unordered_map<std::string, Builder::Builder_speed> builder_animation_table;
};

void set_relevant_arg( const Lookup_tables& tables, Maze_runner& runner, const Flag_arg& pairs );
void set_rows( Maze_runner& runner, const Flag_arg& pairs );
void set_cols( Maze_runner& runner, const Flag_arg& pairs );
void print_invalid_arg( const Flag_arg& pairs );
void print_usage();

int main( int argc, char** argv )
{
  const Lookup_tables tables = {
    { "-r", "-c", "-b", "-s", "-h", "-g", "-d", "-m", "-sa", "-ba" },
    {
      { "rdfs", { Builder::generate_recursive_backtracker_maze, Builder::animate_recursive_backtracker_maze } },
      { "wilson", { Builder::generate_wilson_path_carver_maze, Builder::animate_wilson_path_carver_maze } },
      { "wilson-walls", { Builder::generate_wilson_wall_adder_maze, Builder::animate_wilson_wall_adder_maze } },
      { "fractal", { Builder::generate_recursive_subdivision_maze, Builder::animate_recursive_subdivision_maze } },
      { "kruskal", { Builder::generate_kruskal_maze, Builder::animate_kruskal_maze } },
      { "prim", { Builder::generate_prim_maze, Builder::animate_prim_maze } },
      { "grid", { Builder::generate_grid_maze, Builder::animate_grid_maze } },
      { "arena", { Builder::generate_arena, Builder::animate_arena } },
    },
    {
      { "cross", { Builder::add_cross, Builder::add_cross_animated } },
      { "x", { Builder::add_x, Builder::add_x_animated } },
    },
    {
      { "dfs-hunt", { Solver::solve_with_dfs_thread_hunt, Solver::animate_with_dfs_thread_hunt } },
      { "dfs-gather", { Solver::solve_with_dfs_thread_gather, Solver::animate_with_dfs_thread_gather } },
      { "dfs-corners", { Solver::solve_with_dfs_thread_corners, Solver::animate_with_dfs_thread_corners } },
      { "floodfs-hunt", { Solver::solve_with_floodfs_thread_hunt, Solver::animate_with_floodfs_thread_hunt } },
      { "floodfs-gather",
        { Solver::solve_with_floodfs_thread_gather, Solver::animate_with_floodfs_thread_gather } },
      { "floodfs-corners",
        { Solver::solve_with_floodfs_thread_corners, Solver::animate_with_floodfs_thread_corners } },
      { "rdfs-hunt",
        { Solver::solve_with_randomized_dfs_thread_hunt, Solver::animate_with_randomized_dfs_thread_hunt } },
      { "rdfs-gather",
        { Solver::solve_with_randomized_dfs_thread_gather, Solver::animate_with_randomized_dfs_thread_gather } },
      { "rdfs-corners",
        { Solver::solve_with_randomized_dfs_thread_corners, Solver::animate_with_randomized_dfs_thread_corners } },
      { "bfs-hunt", { Solver::solve_with_bfs_thread_hunt, Solver::animate_with_bfs_thread_hunt } },
      { "bfs-gather", { Solver::solve_with_bfs_thread_gather, Solver::animate_with_bfs_thread_gather } },
      { "bfs-corners", { Solver::solve_with_bfs_thread_corners, Solver::animate_with_bfs_thread_corners } },
    },
    {
      { "sharp", Builder::Maze::Maze_style::sharp },
      { "round", Builder::Maze::Maze_style::round },
      { "doubles", Builder::Maze::Maze_style::doubles },
      { "bold", Builder::Maze::Maze_style::bold },
      { "contrast", Builder::Maze::Maze_style::contrast },
      { "spikes", Builder::Maze::Maze_style::spikes },
    },
    {
      { "0", Solver::Solver_speed::instant },
      { "1", Solver::Solver_speed::speed_1 },
      { "2", Solver::Solver_speed::speed_2 },
      { "3", Solver::Solver_speed::speed_3 },
      { "4", Solver::Solver_speed::speed_4 },
      { "5", Solver::Solver_speed::speed_5 },
      { "6", Solver::Solver_speed::speed_6 },
      { "7", Solver::Solver_speed::speed_7 },
    },
    {
      { "0", Builder::Builder_speed::instant },
      { "1", Builder::Builder_speed::speed_1 },
      { "2", Builder::Builder_speed::speed_2 },
      { "3", Builder::Builder_speed::speed_3 },
      { "4", Builder::Builder_speed::speed_4 },
      { "5", Builder::Builder_speed::speed_5 },
      { "6", Builder::Builder_speed::speed_6 },
      { "7", Builder::Builder_speed::speed_7 },
    },
  };

  Maze_runner runner;
  const auto args = std::span( argv, static_cast<size_t>( argc ) );
  bool process_current = false;
  std::string_view prev_flag = {};
  // In the case of no arguments this is skipped and we use our sensible defaults.
  for ( size_t i = 1; i < args.size(); i++ ) {
    auto* arg = args[i];
    if ( process_current ) {
      set_relevant_arg( tables, runner, { prev_flag, arg } );
      process_current = false;
    } else {
      const auto& found_arg = tables.argument_flags.find( arg );
      if ( found_arg == tables.argument_flags.end() ) {
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

  Builder::Maze maze( runner.args );

  // Functions are stored in tuples so use tuple get syntax and then call them immidiately.

  if ( runner.builder_view == animated_playback ) {
    std::get<animated_playback>( runner.builder )( maze, runner.builder_speed );
    if ( runner.modder ) {
      std::get<animated_playback>( runner.modder.value() )( maze, runner.builder_speed );
    }
  } else {
    std::get<static_image>( runner.builder )( maze );
    if ( runner.modder ) {
      std::get<static_image>( runner.modder.value() )( maze );
    }
  }

  if ( runner.solver_view == animated_playback ) {
    std::get<animated_playback>( runner.solver )( maze, runner.solver_speed );
  } else {
    std::get<static_image>( runner.solver )( maze );
  }
  return 0;
}

void set_relevant_arg( const Lookup_tables& tables, Maze_runner& runner, const Flag_arg& pairs )
{
  if ( pairs.flag == "-r" ) {
    set_rows( runner, pairs );
    return;
  }
  if ( pairs.flag == "-c" ) {
    set_cols( runner, pairs );
    return;
  }
  if ( pairs.flag == "-b" ) {
    const auto found = tables.builder_table.find( pairs.arg.data() );
    if ( found == tables.builder_table.end() ) {
      print_invalid_arg( pairs );
    }
    runner.builder = found->second;
    return;
  }
  if ( pairs.flag == "-m" ) {
    const auto found = tables.modification_table.find( pairs.arg.data() );
    if ( found == tables.modification_table.end() ) {
      print_invalid_arg( pairs );
    }
    runner.modder = found->second;
    return;
  }
  if ( pairs.flag == "-s" ) {
    const auto found = tables.solver_table.find( pairs.arg.data() );
    if ( found == tables.solver_table.end() ) {
      print_invalid_arg( pairs );
    }
    runner.solver = found->second;
    return;
  }
  if ( pairs.flag == "-d" ) {
    const auto found = tables.style_table.find( pairs.arg.data() );
    if ( found == tables.style_table.end() ) {
      print_invalid_arg( pairs );
    }
    runner.args.style = found->second;
    return;
  }
  if ( pairs.flag == "-sa" ) {
    const auto found = tables.solver_animation_table.find( pairs.arg.data() );
    if ( found == tables.solver_animation_table.end() ) {
      print_invalid_arg( pairs );
    }
    runner.solver_speed = found->second;
    runner.solver_view = animated_playback;
    return;
  }
  if ( pairs.flag == "-ba" ) {
    const auto found = tables.builder_animation_table.find( pairs.arg.data() );
    if ( found == tables.builder_animation_table.end() ) {
      print_invalid_arg( pairs );
    }
    runner.builder_speed = found->second;
    runner.builder_view = animated_playback;
    runner.modification_getter = animated_playback;
    return;
  }
  print_invalid_arg( pairs );
}

void set_rows( Maze_runner& runner, const Flag_arg& pairs )
{
  runner.args.odd_rows = std::stoi( pairs.arg.data() );
  if ( runner.args.odd_rows % 2 == 0 ) {
    runner.args.odd_rows++;
  }
  if ( runner.args.odd_rows < 7 ) {
    print_invalid_arg( pairs );
  }
}
void set_cols( Maze_runner& runner, const Flag_arg& pairs )
{
  runner.args.odd_cols = std::stoi( pairs.arg.data() );
  if ( runner.args.odd_cols % 2 == 0 ) {
    runner.args.odd_cols++;
  }
  if ( runner.args.odd_cols < 7 ) {
    print_invalid_arg( pairs );
  }
}

void print_invalid_arg( const Flag_arg& pairs )
{
  std::cerr << "Flag was: " << pairs.flag << std::endl;
  std::cerr << "Invalid argument: " << pairs.arg << std::endl;
  print_usage();
  abort();
}


void print_usage()
{
  std::cout <<
  "┌───┬─────────┬─────┬───┬───────────┬─────┬───────┬─────────────┬─────┐\n"
  "│   │         │     │   │           │     │       │             │     │\n"
  "│ ╷ ╵ ┌───┐ ╷ └─╴ ╷ │ ╷ │ ┌─╴ ┌─┬─╴ │ ╶─┐ ╵ ┌───╴ │ ╶───┬─┐ ╶─┬─┘ ╶─┐ │\n"
  "│ │   │   │ │     │ │ │ │ │   │ │   │   │   │     │     │ │   │     │ │\n"
  "│ └───┤ ┌─┘ ├─────┘ └─┤ ╵ │ ┌─┘ ╵ ┌─┴─┐ └───┤ ╶───┴───┐ │ └─┐ ╵ ┌─┬─┘ │\n"
  "│     │ │   │      Thread Maze Usage Instructions     │ │   │   │ │   │\n"
  "├───┐ ╵ │ -Use flags, followed by arguments, in any order:╷ └─┬─┘ │ ╷ │\n"
  "│   │   │ -r Rows flag. Set rows for the maze.    │   │ │ │   │   │ │ │\n"
  "│ ╶─┴─┐ └─┐ Any number > 7. Zoom out for larger mazes!╵ ╵ ├─┐ │ ╶─┤ └─┤\n"
  "│     │   -c Columns flag. Set columns for the maze.│     │ │ │   │   │\n"
  "│ ┌─┐ └─┐ │ Any number > 7. Zoom out for larger mazes!────┤ │ │ ╷ └─┐ │\n"
  "│ │ │   │ -b Builder flag. Set maze building algorithm.   │ │ │ │   │ │\n"
  "│ │ └─┐ ╵ │ rdfs - Randomized Depth First Search. ┌─────┐ │ │ │ │ ┌─┘ │\n"
  "│ │   │   │ rdfs - Randomized Depth First Search. │     │ │ │ │ │ │   │\n"
  "│ ╵ ╷ ├───┼─rdfs - Randomized Depth First Search. └───┐ ╵ │ ╵ └─┘ │ ╶─┤\n"
  "│   │ │   │ kruskal - Randomized Kruskal's algorithm. │   │       │   │\n"
  "├───┴─┤ ╷ ╵ prim - Randomized Prim's algorithm.─┴───┐ │ ┌─┴─────┬─┴─┐ │\n"
  "│     │ │   wilson - Loop-Erased Random Path Carver.│ │ │       │   │ │\n"
  "│ ┌─┐ ╵ ├─┬─wilson-walls - Loop-Erased Random Wall Adder. ┌───┐ ╵ ╷ │ │\n"
  "│ │ │   │ │ fractal - Randomized recursive subdivision. │ │   │   │ │ │\n"
  "│ ╵ ├───┘ ╵ grid - A random grid pattern. ├─┐ │ ┌─────┤ ╵ │ ┌─┴───┤ ╵ │\n"
  "│   │       arena - Open floor with no walls. │ │     │   │ │     │   │\n"
  "├─╴ ├─────-m Modification flag. Add shortcuts to the maze.┘ │ ┌─┐ └─╴ │\n"
  "│   │     │ cross - Add crossroads through the center.      │ │ │     │\n"
  "│ ┌─┘ ┌─┐ │ x - Add an x of crossing paths through center.──┘ │ └─────┤\n"
  "│ │   │ │ -s Solver flag. Choose the game and solver. │ │     │       │\n"
  "│ ╵ ┌─┘ │ └─dfs-hunt - Depth First Search ╴ ┌───┴─┬─┘ │ │ ┌───┴─────┐ │\n"
  "│   │   │   dfs-gather - Depth First Search │     │   │ │ │         │ │\n"
  "├───┘ ╶─┴─╴ dfs-corners - Depth First Search  ┌─╴ │ ╶─┼─┘ │ ╷ ┌───╴ ╵ │\n"
  "│           floodfs-hunt - Depth First Search │   │   │   │ │ │       │\n"
  "│ ┌───────┬─floodfs-gather - Depth First Search ┌─┴─╴ │ ╶─┴─┤ └───────┤\n"
  "│ │       │ floodfs-corners - Depth First Search│     │     │         │\n"
  "│ │ ╷ ┌─╴ │ rdfs-hunt - Randomized Depth First Search─┴─┬─╴ │ ┌─────╴ │\n"
  "│ │ │ │   │ rdfs-gather - Randomized Depth First Search │   │ │       │\n"
  "│ └─┤ └───┤ rdfs-corners - Randomized Depth First Search┤ ┌─┘ │ ╶───┐ │\n"
  "│   │     │ bfs-hunt - Breadth First Search     │   │   │ │   │     │ │\n"
  "├─┐ │ ┌─┐ └─bfs-gather - Breadth First Search─┐ ╵ ╷ ├─╴ │ └─┐ ├───╴ │ │\n"
  "│ │ │ │ │   bfs-corners - Breadth First Search│   │ │   │   │ │     │ │\n"
  "│ │ │ ╵ └─-d Draw flag. Set the line style for the maze.┴─┐ └─┘ ┌─┬─┘ │\n"
  "│ │ │       sharp - The default straight lines. │   │     │     │ │   │\n"
  "│ │ └─┬───╴ round - Rounded corners.──╴ │ ╷ ╵ ╵ │ ╶─┴─┐ ╶─┴─────┘ │ ╶─┤\n"
  "│ │   │     doubles - Sharp double lines. │     │     │           │   │\n"
  "│ └─┐ └───┬─bold - Thicker straight lines.└─┬───┴─┬─╴ │ ┌───┬───╴ └─┐ │\n"
  "│   │     │ contrast - Full block width and height walls.   │       │ │\n"
  "│ ╷ ├─┬─╴ │ spikes - Connected lines with spikes. ╵ ┌─┘ ╵ ┌─┘ ┌─┐ ┌─┘ │\n"
  "│ │ │ │   -sa Solver Animation flag. Watch the maze solution. │ │ │   │\n"
  "│ │ ╵ │ ╶─┤ Any number 1-7. Speed increases with number.┌─┘ ┌─┤ ╵ │ ╶─┤\n"
  "│ │   │   -ba Builder Animation flag. Watch the maze build. │ │   │   │\n"
  "│ ├─╴ ├─┐ └─Any number 1-7. Speed increases with number.┘ ┌─┘ │ ┌─┴─┐ │\n"
  "│ │   │ │ -h Help flag. Make this prompt appear.  │   │   │   │ │   │ │\n"
  "│ └─┐ ╵ └─┐ No arguments.─┘ ┌───┐ └─┐ ├─╴ │ ╵ └───┤ ┌─┘ ┌─┴─╴ │ ├─╴ │ │\n"
  "│   │     -If any flags are omitted, defaults are used. │     │ │   │ │\n"
  "├─╴ ├───┐ -Examples:┐ ╶─┬─┬─┘ ╷ ├─╴ │ │ ┌─┴───────┘ ├─╴ │ ╶─┐ │ ╵ ┌─┘ │\n"
  "│   │   │ │ ./run_maze  │ │   │ │   │ │ │           │   │   │ │   │   │\n"
  "│ ╶─┤ ╶─┘ │ ./run_maze -r 51 -c 111 -b random-dfs -s bfs -hunt┘ ┌─┘ ┌─┤\n"
  "│   │     │ ./run_maze -c 111 -s bfs -g gather│   │   │   │ │   │   │ │\n"
  "│ ╷ │ ╶───┤ ./run_maze -s bfs -g corners -d round -b fractal╵ ┌─┤ ╶─┤ │\n"
  "│ │ │     │ ./run_maze -s dfs -ba 4 -sa 5 -b kruskal -m x │   │ │   │ │\n"
  "├─┘ ├───┬─┘ │ ╶─┼─╴ │ │ │ ╷ ├─┐ ╵ ╷ ├─┴───╴ │ │ ┌───┤ ╵ │ └─┐ ╵ └─┐ ╵ │\n"
  "│   │   │   │   │   │ │ │ │ │ │   │ │       │ │ │   │   │   │     │   │\n"
  "│ ╶─┘ ╷ ╵ ╶─┴───┘ ┌─┘ ╵ ╵ │ ╵ └───┤ ╵ ╶─────┘ │ ╵ ╷ └───┴─┐ └─────┴─╴ │\n"
  "│     │           │       │       │           │   │       │           │\n"
  "└─────┴───────────┴───────┴───────┴───────────┴───┴───────┴───────────┘\n";
  std::cout << std::endl;
}
