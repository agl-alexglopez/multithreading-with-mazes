#include "maze_algorithms.hh"
#include "maze_utilities.hh"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <thread>

namespace Builder {

void generate_recursive_backtracker_maze( Maze& maze )
{
  fill_maze_with_walls( maze );
  // Note that backtracking occurs by encoding directions into path bits. No stack needed.
  std::mt19937 generator_( std::random_device {}() );
  std::uniform_int_distribution<int> row_random_( 1, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_random_( 1, maze.col_size() - 2 );

  Maze::Point start = { 2 * ( row_random_( generator_ ) / 2 ) + 1, 2 * ( col_random_( generator_ ) / 2 ) + 1 };
  std::vector<int> random_direction_indices( Maze::generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  Maze::Point cur = start;
  bool branches_remain = true;
  while ( branches_remain ) {
    // The unvisited neighbor is always random because array is re-shuffled each time.
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator_ );
    branches_remain = false;
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& direction = Maze::generate_directions_.at( i );
      Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
      if ( can_build_new_square( maze, next ) ) {
        branches_remain = true;
        carve_path_markings( maze, cur, next );
        cur = next;
        break;
      }
    }
    if ( !branches_remain && cur != start ) {
      Maze::Backtrack_marker dir
        = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
      const Maze::Point& backtracking = Maze::backtracking_marks_.at( dir );
      Maze::Point next = { cur.row + backtracking.row, cur.col + backtracking.col };
      // We are using fields the threads will use later. Clear bits as we backtrack.
      maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
      cur = next;
      branches_remain = true;
    }
  }
}

void animate_recursive_backtracker_maze( Maze& maze, Builder_speed speed )
{
  Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  fill_maze_with_walls_animated( maze );
  clear_and_flush_grid( maze );
  std::mt19937 generator_( std::random_device {}() );
  std::uniform_int_distribution<int> row_random_( 1, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_random_( 1, maze.col_size() - 2 );
  Maze::Point start = { 2 * ( row_random_( generator_ ) / 2 ) + 1, 2 * ( col_random_( generator_ ) / 2 ) + 1 };
  std::vector<int> random_direction_indices( Maze::generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  Maze::Point cur = start;
  bool branches_remain = true;
  while ( branches_remain ) {
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator_ );
    branches_remain = false;
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& direction = Maze::generate_directions_.at( i );
      Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
      if ( can_build_new_square( maze, next ) ) {
        carve_path_markings_animated( maze, cur, next, animation );
        branches_remain = true;
        cur = next;
        break;
      }
    }
    if ( !branches_remain && cur != start ) {
      Maze::Backtrack_marker dir
        = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
      const Maze::Point& backtracking = Maze::backtracking_marks_.at( dir );
      Maze::Point next = { cur.row + backtracking.row, cur.col + backtracking.col };
      // We are using fields the threads will use later. Clear bits as we backtrack.
      maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
      flush_cursor_maze_coordinate( maze, cur );
      std::this_thread::sleep_for( std::chrono::microseconds( animation ) );
      cur = next;
      branches_remain = true;
    }
  }
}

} // namespace Builder
