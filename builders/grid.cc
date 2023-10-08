#include "maze.hh"
#include "maze_algorithms.hh"
#include "maze_utilities.hh"
#include "speed.hh"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <numeric>
#include <random>
#include <stack>
#include <thread>
#include <vector>

namespace Builder {

namespace {

constexpr int run_limit = 4;

struct Run_start
{
  Maze::Point cur;
  Maze::Point direction;
};

void complete_run( Maze& maze, std::stack<Maze::Point>& dfs, Run_start run )
{
  // This allows us to run over previous paths which is what makes this algorithm unique.
  Maze::Point next = { run.cur.row + run.direction.row, run.cur.col + run.direction.col };
  // Create the "grid" by running in one direction until wall or limit.
  int cur_run = 0;
  while ( is_square_within_perimeter_walls( maze, next ) && cur_run < run_limit ) {
    join_squares( maze, run.cur, next );
    run.cur = next;
    dfs.push( next );
    next.row += run.direction.row;
    next.col += run.direction.col;
    cur_run++;
  }
}

void animate_run( Maze& maze, std::stack<Maze::Point>& dfs, Run_start run, Speed::Speed_unit speed )
{
  Maze::Point next = { run.cur.row + run.direction.row, run.cur.col + run.direction.col };
  int cur_run = 0;
  while ( is_square_within_perimeter_walls( maze, next ) && cur_run < run_limit ) {
    join_squares_animated( maze, run.cur, next, speed );
    run.cur = next;
    dfs.push( next );
    next.row += run.direction.row;
    next.col += run.direction.col;
    cur_run++;
  }
}

} // namespace

void generate_grid( Maze& maze )
{
  fill_maze_with_walls( maze );
  std::mt19937 generator( std::random_device {}() );
  std::uniform_int_distribution row_random( 1, maze.row_size() - 2 );
  std::uniform_int_distribution col_random( 1, maze.col_size() - 2 );
  std::stack<Maze::Point> dfs(
    { { 2 * ( row_random( generator ) / 2 ) + 1, 2 * ( col_random( generator ) / 2 ) + 1 } } );
  std::vector<int> random_direction_indices( Maze::build_dirs_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  while ( !dfs.empty() ) {
    const Maze::Point cur = dfs.top();
    std::shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );
    bool branches_remain = false;
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& direction = Maze::build_dirs_.at( i );
      const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
      if ( can_build_new_square( maze, next ) ) {
        complete_run( maze, dfs, { cur, direction } );
        branches_remain = true;
        break;
      }
    }
    if ( !branches_remain ) {
      dfs.pop();
    }
  }
  clear_and_flush_grid( maze );
}

void animate_grid( Maze& maze, Speed::Speed speed )
{
  const Speed::Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  fill_maze_with_walls_animated( maze );
  clear_and_flush_grid( maze );
  std::mt19937 generator( std::random_device {}() );
  std::uniform_int_distribution row_random( 1, maze.row_size() - 2 );
  std::uniform_int_distribution col_random( 1, maze.col_size() - 2 );
  std::stack<Maze::Point> dfs(
    { { 2 * ( row_random( generator ) / 2 ) + 1, 2 * ( col_random( generator ) / 2 ) + 1 } } );
  std::vector<int> random_direction_indices( Maze::build_dirs_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  while ( !dfs.empty() ) {
    const Maze::Point cur = dfs.top();
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );
    bool branches_remain = false;
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& direction = Maze::build_dirs_.at( i );
      const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
      if ( can_build_new_square( maze, next ) ) {
        animate_run( maze, dfs, { cur, direction }, animation );
        branches_remain = true;
        break;
      }
    }
    if ( !branches_remain ) {
      flush_cursor_maze_coordinate( maze, cur );
      std::this_thread::sleep_for( std::chrono::microseconds( animation ) );
      dfs.pop();
    }
  }
}

} // namespace Builder
