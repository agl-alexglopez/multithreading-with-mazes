import labyrinth;

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <ostream>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using Build_demo = std::function<void( Maze::Maze&, Speed::Speed )>;
using Solve_demo = std::function<void( Maze::Maze&, Speed::Speed )>;

struct Flag_arg
{
  std::string_view flag;
  std::string_view arg;
};

struct Demo_runner
{
  Maze::Maze_args args {};

  std::vector<Speed::Speed> builder_speed { Speed::Speed::speed_1,
                                            Speed::Speed::speed_2,
                                            Speed::Speed::speed_3,
                                            Speed::Speed::speed_4,
                                            Speed::Speed::speed_5,
                                            Speed::Speed::speed_6,
                                            Speed::Speed::speed_7 };

  std::vector<Maze::Maze_style> wall_style {
    Maze::Maze_style::sharp,
    Maze::Maze_style::round,
    Maze::Maze_style::doubles,
    Maze::Maze_style::bold,
    Maze::Maze_style::contrast,
    Maze::Maze_style::spikes,
  };

  std::vector<Build_demo> builders { Recursive_backtracker::animate_maze,
                                     Recursive_subdivision::animate_maze,
                                     Wilson_path_carver::animate_maze,
                                     Wilson_wall_adder::animate_maze,
                                     Prim::animate_maze,
                                     Kruskal::animate_maze,
                                     Eller::animate_maze,
                                     Grid::animate_maze,
                                     Arena::animate_maze };

  std::vector<Build_demo> modifications { Mods::add_cross_animated, Mods::add_x_animated };

  std::vector<Speed::Speed> solver_speed { Speed::Speed::speed_1,
                                           Speed::Speed::speed_2,
                                           Speed::Speed::speed_3,
                                           Speed::Speed::speed_4,
                                           Speed::Speed::speed_5,
                                           Speed::Speed::speed_6,
                                           Speed::Speed::speed_7 };

  std::vector<Solve_demo> solvers {
    Dfs::animate_hunt,
    Dfs::animate_gather,
    Dfs::animate_corners,
    Floodfs::animate_hunt,
    Floodfs::animate_gather,
    Floodfs::animate_corners,
    Rdfs::animate_hunt,
    Rdfs::animate_gather,
    Rdfs::animate_corners,
    Bfs::animate_hunt,
    Bfs::animate_gather,
    Bfs::animate_corners,
    Dark_dfs::animate_hunt,
    Dark_dfs::animate_gather,
    Dark_dfs::animate_corners,
    Dark_bfs::animate_hunt,
    Dark_bfs::animate_gather,
    Dark_bfs::animate_corners,
    Dark_floodfs::animate_hunt,
    Dark_floodfs::animate_gather,
    Dark_floodfs::animate_corners,
    Dark_rdfs::animate_hunt,
    Dark_rdfs::animate_gather,
    Dark_rdfs::animate_corners,
    Distance::animate_distance_from_center,
    Runs::animate_runs,
  };
};

} // namespace

void set_rows( Demo_runner& runner, const Flag_arg& pairs );
void set_cols( Demo_runner& runner, const Flag_arg& pairs );

int main( int argc, char** argv )
{
  Demo_runner demo;
  const auto args = std::span( argv, static_cast<uint64_t>( argc ) );
  bool process_current = false;
  Flag_arg flags = {};
  for ( uint64_t i = 1; i < args.size(); i++ ) {
    const auto* arg = args[i];
    if ( process_current ) {
      flags.arg = arg;
      if ( flags.flag == "-r" ) {
        set_rows( demo, flags );
      } else if ( flags.flag == "-c" ) {
        set_cols( demo, flags );
      }
      process_current = false;
    } else {
      const std::string arg_str( arg );
      if ( arg_str == "-r" || arg_str == "-c" ) {
        flags.flag = arg;
      } else {
        std::cerr << "The only arguments are optional row [-r] or column [-c] dimensions."
                  << "\n";
        std::abort();
      }
      process_current = true;
    }
  }

  std::mt19937 gen( std::random_device {}() );
  std::uniform_int_distribution<uint64_t> wall_chooser( 0, demo.wall_style.size() - 1 );
  std::uniform_int_distribution<uint64_t> builder_chooser( 0, demo.builders.size() - 1 );
  std::uniform_int_distribution<uint64_t> speed_chooser( 0, demo.builder_speed.size() - 1 );
  std::uniform_int_distribution<uint64_t> optional_modification( 0, 5 );
  std::uniform_int_distribution<uint64_t> modification_chooser( 0, demo.modifications.size() - 1 );
  std::uniform_int_distribution<uint64_t> solver_chooser( 0, demo.solvers.size() - 1 );
  for ( ;; ) {
    demo.args.style = demo.wall_style[wall_chooser( gen )];
    Maze::Maze maze( demo.args );
    demo.builders[builder_chooser( gen )]( maze, demo.builder_speed[speed_chooser( gen )] );

    if ( optional_modification( gen ) == 0 ) {
      demo.modifications[modification_chooser( gen )]( maze, demo.builder_speed[speed_chooser( gen )] );
    }

    Printer::set_cursor_position( { 0, 0 } );
    demo.solvers[solver_chooser( gen )]( maze, demo.solver_speed[speed_chooser( gen )] );

    // We don't need loading time, it's just jarring to immediately transition after the solution finishes.
    std::cout << "Loading next maze..." << std::flush;
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    Printer::set_cursor_position( { 0, 0 } );
  }
}

void set_rows( Demo_runner& runner, const Flag_arg& pairs )
{
  runner.args.odd_rows = std::stoi( pairs.arg.data() );
  if ( runner.args.odd_rows % 2 == 0 ) {
    runner.args.odd_rows++;
  }
  if ( runner.args.odd_rows < 7 ) {
    std::cerr << "Minimum row may be 7."
              << "\n";
    std::abort();
  }
}

void set_cols( Demo_runner& runner, const Flag_arg& pairs )
{
  runner.args.odd_cols = std::stoi( pairs.arg.data() );
  if ( runner.args.odd_cols % 2 == 0 ) {
    runner.args.odd_cols++;
  }
  if ( runner.args.odd_cols < 7 ) {
    std::cerr << "Minimum col may be 7."
              << "\n";
    std::abort();
  }
}
