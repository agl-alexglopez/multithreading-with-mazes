import labyrinth;

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace {

using Build_function
  = std::tuple<std::function<void( Maze::Maze& )>, std::function<void( Maze::Maze&, Speed::Speed )>>;

using Solve_function
  = std::tuple<std::function<void( Maze::Maze& )>, std::function<void( Maze::Maze&, Speed::Speed )>>;

constexpr int static_image = 0;
constexpr int animated_playback = 1;

struct Flag_arg
{
  std::string_view flag;
  std::string_view arg;
};

struct Maze_runner
{
  Maze::Maze_args args;

  int builder_view { static_image };
  Speed::Speed builder_speed {};
  Build_function builder { Recursive_backtracker::generate_recursive_backtracker,
                           Recursive_backtracker::animate_recursive_backtracker };

  int modification_getter { static_image };
  std::optional<Build_function> modder {};

  int solver_view { static_image };
  Speed::Speed solver_speed {};
  Solve_function solver { Dfs::dfs_thread_hunt, Dfs::animate_dfs_thread_hunt };
  Maze_runner() : args {} {}
};

struct Lookup_tables
{
  std::unordered_set<std::string> argument_flags;
  std::unordered_map<std::string, Build_function> builder_table;
  std::unordered_map<std::string, Build_function> modification_table;
  std::unordered_map<std::string, Solve_function> solver_table;
  std::unordered_map<std::string, Maze::Maze_style> style_table;
  std::unordered_map<std::string, Speed::Speed> solver_animation_table;
  std::unordered_map<std::string, Speed::Speed> builder_animation_table;
};

} // namespace

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
      { "rdfs",
        { Recursive_backtracker::generate_recursive_backtracker,
          Recursive_backtracker::animate_recursive_backtracker } },
      { "wilson",
        { Wilson_path_carver::generate_wilson_path_carver, Wilson_path_carver::animate_wilson_path_carver } },
      { "wilson-walls",
        { Wilson_wall_adder::generate_wilson_wall_adder, Wilson_wall_adder::animate_wilson_wall_adder } },
      { "fractal",
        { Recursive_subdivision::generate_recursive_subdivision,
          Recursive_subdivision::animate_recursive_subdivision } },
      { "kruskal", { Kruskal::generate_kruskal, Kruskal::animate_kruskal } },
      { "eller", { Eller::generate_eller, Eller::animate_eller } },
      { "prim", { Prim::generate_prim, Prim::animate_prim } },
      { "grid", { Grid::generate_grid, Grid::animate_grid } },
      { "arena", { Arena::generate_arena, Arena::animate_arena } },
    },
    {
      { "cross", { Mods::add_cross, Mods::add_cross_animated } },
      { "x", { Mods::add_x, Mods::add_x_animated } },
    },
    {
      { "dfs-hunt", { Dfs::dfs_thread_hunt, Dfs::animate_dfs_thread_hunt } },
      { "dfs-gather", { Dfs::dfs_thread_gather, Dfs::animate_dfs_thread_gather } },
      { "dfs-corners", { Dfs::dfs_thread_corners, Dfs::animate_dfs_thread_corners } },
      { "floodfs-hunt", { Floodfs::floodfs_thread_hunt, Floodfs::animate_floodfs_thread_hunt } },
      { "floodfs-gather", { Floodfs::floodfs_thread_gather, Floodfs::animate_floodfs_thread_gather } },
      { "floodfs-corners", { Floodfs::floodfs_thread_corners, Floodfs::animate_floodfs_thread_corners } },
      { "rdfs-hunt", { Rdfs::randomized_dfs_thread_hunt, Rdfs::animate_randomized_dfs_thread_hunt } },
      { "rdfs-gather", { Rdfs::randomized_dfs_thread_gather, Rdfs::animate_randomized_dfs_thread_gather } },
      { "rdfs-corners", { Rdfs::randomized_dfs_thread_corners, Rdfs::animate_randomized_dfs_thread_corners } },
      { "bfs-hunt", { Bfs::bfs_thread_hunt, Bfs::animate_bfs_thread_hunt } },
      { "bfs-gather", { Bfs::bfs_thread_gather, Bfs::animate_bfs_thread_gather } },
      { "bfs-corners", { Bfs::bfs_thread_corners, Bfs::animate_bfs_thread_corners } },
      { "darkdfs-hunt", { Dfs::dfs_thread_hunt, Dark_dfs::animate_darkdfs_thread_hunt } },
      { "darkdfs-gather", { Dfs::dfs_thread_gather, Dark_dfs::animate_darkdfs_thread_gather } },
      { "darkdfs-corners", { Dfs::dfs_thread_gather, Dark_dfs::animate_darkdfs_thread_corners } },
      { "darkbfs-hunt", { Bfs::bfs_thread_hunt, Dark_bfs::animate_darkbfs_thread_hunt } },
      { "darkbfs-gather", { Bfs::bfs_thread_gather, Dark_bfs::animate_darkbfs_thread_gather } },
      { "darkbfs-corners", { Bfs::bfs_thread_gather, Dark_bfs::animate_darkbfs_thread_corners } },
      { "darkfloodfs-hunt", { Dfs::dfs_thread_hunt, Dark_floodfs::animate_darkfloodfs_thread_hunt } },
      { "darkfloodfs-gather", { Dfs::dfs_thread_gather, Dark_floodfs::animate_darkfloodfs_thread_gather } },
      { "darkfloodfs-corners", { Dfs::dfs_thread_gather, Dark_floodfs::animate_darkfloodfs_thread_corners } },
      { "darkrdfs-hunt", { Dfs::dfs_thread_hunt, Dark_rdfs::animate_darkrandomized_dfs_thread_hunt } },
      { "darkrdfs-gather", { Dfs::dfs_thread_gather, Dark_rdfs::animate_darkrandomized_dfs_thread_gather } },
      { "darkrdfs-corners", { Dfs::dfs_thread_gather, Dark_rdfs::animate_darkrandomized_dfs_thread_corners } },
    },
    {
      { "sharp", Maze::Maze_style::sharp },
      { "round", Maze::Maze_style::round },
      { "doubles", Maze::Maze_style::doubles },
      { "bold", Maze::Maze_style::bold },
      { "contrast", Maze::Maze_style::contrast },
      { "spikes", Maze::Maze_style::spikes },
    },
    {
      { "0", Speed::Speed::instant },
      { "1", Speed::Speed::speed_1 },
      { "2", Speed::Speed::speed_2 },
      { "3", Speed::Speed::speed_3 },
      { "4", Speed::Speed::speed_4 },
      { "5", Speed::Speed::speed_5 },
      { "6", Speed::Speed::speed_6 },
      { "7", Speed::Speed::speed_7 },
    },
    {
      { "0", Speed::Speed::instant },
      { "1", Speed::Speed::speed_1 },
      { "2", Speed::Speed::speed_2 },
      { "3", Speed::Speed::speed_3 },
      { "4", Speed::Speed::speed_4 },
      { "5", Speed::Speed::speed_5 },
      { "6", Speed::Speed::speed_6 },
      { "7", Speed::Speed::speed_7 },
    },
  };

  Maze_runner runner;
  const auto args = std::span( argv, static_cast<uint64_t>( argc ) );
  bool process_current = false;
  std::string_view prev_flag = {};
  // In the case of no arguments this is skipped and we use our sensible defaults.
  for ( uint64_t i = 1; i < args.size(); i++ ) {
    auto* arg = args[i];
    if ( process_current ) {
      set_relevant_arg( tables, runner, { prev_flag, arg } );
      process_current = false;
    } else {
      const auto& found_arg = tables.argument_flags.find( arg );
      if ( found_arg == tables.argument_flags.end() ) {
        std::cerr << "Invalid argument flag: " << arg << "\n";
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

  Maze::Maze maze( runner.args );

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

  // This helps ensure we have a smooth transition from build to solve with no flashing from redrawing frame.
  Printer::set_cursor_position( { 0, 0 } );

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
  std::cerr << "Flag was: " << pairs.flag << "\n";
  std::cerr << "Invalid argument: " << pairs.arg << "\n";
  print_usage();
  std::abort();
}

void print_usage()
{
  std::cout << "┌───┬─────────┬─────┬───┬───────────┬─────┬───────┬─────────────┬─────┐\n"
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
               "│ │ └─┐ ╵ │ rdfs - Randomized Depth First Search.         │ │ └─┘ ┌─┘ │\n"
               "│     │   │ kruskal - Randomized Kruskal's algorithm. │   │       │   │\n"
               "├─────┤ ╷ ╵ prim - Randomized Prim's algorithm.─┴───┐ │ ┌─┴─────┬─┴─┐ │\n"
               "│     │ │   eller - Randomized Eller's algorithm.   │ │ │       │   │ │\n"
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
               "│ │ │ │ │   dark[solver]-[game] - A mystery...    │ │   │   │ │     │ │\n"
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
  std::cout << "\n";
}
