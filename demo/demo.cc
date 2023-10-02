#include "maze_algorithms.hh"
#include "maze_solvers.hh"
#include "print_utilities.hh"
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <ostream>
#include <random>
#include <span>
#include <thread>

namespace {

using Build_demo = std::function<void( Builder::Maze&, Builder::Builder_speed )>;
using Solve_demo = std::function<void( Builder::Maze&, Solver::Solver_speed )>;

struct Flag_arg
{
  std::string_view flag;
  std::string_view arg;
};

struct Demo_runner
{
  Builder::Maze::Maze_args args {};

  std::vector<Builder::Builder_speed> builder_speed { Builder::Builder_speed::speed_1,
                                                      Builder::Builder_speed::speed_2,
                                                      Builder::Builder_speed::speed_3,
                                                      Builder::Builder_speed::speed_4,
                                                      Builder::Builder_speed::speed_5,
                                                      Builder::Builder_speed::speed_6,
                                                      Builder::Builder_speed::speed_7 };

  std::vector<Builder::Maze::Maze_style> wall_style {
    Builder::Maze::Maze_style::sharp,
    Builder::Maze::Maze_style::round,
    Builder::Maze::Maze_style::doubles,
    Builder::Maze::Maze_style::bold,
    Builder::Maze::Maze_style::contrast,
    Builder::Maze::Maze_style::spikes,
  };

  std::vector<Build_demo> builders { Builder::animate_recursive_backtracker_maze,
                                     Builder::animate_recursive_subdivision_maze,
                                     Builder::animate_wilson_path_carver_maze,
                                     Builder::animate_wilson_wall_adder_maze,
                                     Builder::animate_prim_maze,
                                     Builder::animate_kruskal_maze,
                                     Builder::animate_eller_maze,
                                     Builder::animate_grid_maze,
                                     Builder::animate_arena };

  std::vector<Build_demo> modifications { Builder::add_cross_animated, Builder::add_x_animated };

  std::vector<Solver::Solver_speed> solver_speed { Solver::Solver_speed::speed_1,
                                                   Solver::Solver_speed::speed_2,
                                                   Solver::Solver_speed::speed_3,
                                                   Solver::Solver_speed::speed_4,
                                                   Solver::Solver_speed::speed_5,
                                                   Solver::Solver_speed::speed_6,
                                                   Solver::Solver_speed::speed_7 };

  std::vector<Solve_demo> solvers { Solver::animate_with_dfs_thread_hunt,
                                    Solver::animate_with_dfs_thread_gather,
                                    Solver::animate_with_dfs_thread_corners,
                                    Solver::animate_with_floodfs_thread_hunt,
                                    Solver::animate_with_floodfs_thread_gather,
                                    Solver::animate_with_floodfs_thread_corners,
                                    Solver::animate_with_randomized_dfs_thread_hunt,
                                    Solver::animate_with_randomized_dfs_thread_gather,
                                    Solver::animate_with_randomized_dfs_thread_corners,
                                    Solver::animate_with_bfs_thread_hunt,
                                    Solver::animate_with_bfs_thread_gather,
                                    Solver::animate_with_bfs_thread_corners };
};

} // namespace

void set_rows( Demo_runner& runner, const Flag_arg& pairs );
void set_cols( Demo_runner& runner, const Flag_arg& pairs );

int main( int argc, char** argv )
{
  Demo_runner demo;
  const auto args = std::span( argv, static_cast<size_t>( argc ) );
  bool process_current = false;
  Flag_arg flags = {};
  for ( size_t i = 1; i < args.size(); i++ ) {
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
        std::cerr << "The only arguments are optional row [-r] or column [-c] dimensions." << std::endl;
        abort();
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
    Builder::Maze maze( demo.args );
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
    std::cerr << "Minimum row may be 7." << std::endl;
    abort();
  }
}

void set_cols( Demo_runner& runner, const Flag_arg& pairs )
{
  runner.args.odd_cols = std::stoi( pairs.arg.data() );
  if ( runner.args.odd_cols % 2 == 0 ) {
    runner.args.odd_cols++;
  }
  if ( runner.args.odd_cols < 7 ) {
    std::cerr << "Minimum col may be 7." << std::endl;
    abort();
  }
}
