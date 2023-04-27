#include "maze_algorithms.hh"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

namespace Builder {

namespace {

struct Loop
{
  Maze::Point& walk;
  Maze::Point& root;
};

struct Random_walk
{
  Maze::Point prev;
  Maze::Point walk;
  Maze::Point next;
};

bool is_valid_random_step( Maze& maze, const Maze::Point& next, const Maze::Point& previous )
{
  return next.row > 0 && next.row < maze.row_size() - 1 && next.col > 0 && next.col < maze.col_size() - 1
         && next != previous;
}

void build_with_marks( Maze& maze, const Maze::Point& cur, const Maze::Point& next )
{
  Maze::Point wall = cur;
  if ( next.row < cur.row ) {
    wall.row--;
  } else if ( next.row > cur.row ) {
    wall.row++;
  } else if ( next.col < cur.col ) {
    wall.col--;
  } else if ( next.col > cur.col ) {
    wall.col++;
  } else {
    std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
    std::abort();
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze[next.row][next.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  carve_path_walls( maze, cur );
  carve_path_walls( maze, next );
  carve_path_walls( maze, wall );
}

void animate_with_marks( Maze& maze, const Maze::Point& cur, const Maze::Point& next, Speed_unit speed )
{
  Maze::Point wall = cur;
  if ( next.row < cur.row ) {
    wall.row--;
  } else if ( next.row > cur.row ) {
    wall.row++;
  } else if ( next.col < cur.col ) {
    wall.col--;
  } else if ( next.col > cur.col ) {
    wall.col++;
  } else {
    std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze[next.row][next.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  carve_path_walls_animated( maze, cur, speed );
  carve_path_walls_animated( maze, wall, speed );
  carve_path_walls_animated( maze, next, speed );
}

void connect_walk_to_maze( Maze& maze, const Maze::Point& walk )
{
  Maze::Point cur = walk;
  while ( maze[cur.row][cur.col] & Maze::markers_mask_ ) {
    const Maze::Backtrack_marker mark = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    build_with_marks( maze, cur, next );
    // Clean up after ourselves and leave no marks behind for the maze solvers.
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
    cur = next;
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::markers_mask_ );
  carve_path_walls( maze, cur );
}

void animate_walk_to_maze( Maze& maze, const Maze::Point& walk, Speed_unit speed )
{
  Maze::Point cur = walk;
  while ( maze[cur.row][cur.col] & Maze::markers_mask_ ) {
    const Maze::Backtrack_marker mark = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    animate_with_marks( maze, cur, next, speed );
    // Clean up after ourselves and leave no marks behind for the maze solvers.
    maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::markers_mask_ );
    flush_cursor_maze_coordinate( maze, cur );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
    cur = next;
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::markers_mask_ );
  carve_path_walls_animated( maze, cur, speed );
  flush_cursor_maze_coordinate( maze, cur );
  std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
}

void erase_loop( Maze& maze, const Loop& loop )
{
  Maze::Point cur = loop.walk;
  while ( cur != loop.root ) {
    maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
    const Maze::Backtrack_marker mark = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
    cur = next;
  }
}

void animate_erase_loop( Maze& maze, const Loop& loop, Speed_unit speed )
{
  Maze::Point cur = loop.walk;
  while ( cur != loop.root ) {
    maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
    const Maze::Backtrack_marker mark = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::markers_mask_ );
    flush_cursor_maze_coordinate( maze, cur );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
    cur = next;
  }
}

bool continue_random_walks( Maze& maze, Random_walk& cur )
{
  if ( has_builder_bit( maze, cur.next ) ) {
    build_with_marks( maze, cur.walk, cur.next );
    connect_walk_to_maze( maze, cur.walk );
    cur.walk = choose_arbitrary_point( maze, Parity_point::odd );

    if ( !cur.walk.row ) {
      return false;
    }

    maze[cur.walk.row][cur.walk.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
    cur.prev = {};
    return true;
  }
  if ( maze[cur.next.row][cur.next.col] & Maze::start_bit_ ) {
    erase_loop( maze, { cur.walk, cur.next } );
    cur.walk = cur.next;
    cur.prev = {};
    const Maze::Backtrack_marker mark
      = ( maze[cur.walk.row][cur.walk.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    cur.prev = { cur.walk.row + direction.row, cur.walk.col + direction.col };
    return true;
  }
  mark_origin( maze, cur.walk, cur.next );
  cur.prev = cur.walk;
  cur.walk = cur.next;
  return true;
}

bool animate_random_walks( Maze& maze, Random_walk& cur, Speed_unit speed )
{
  if ( has_builder_bit( maze, cur.next ) ) {
    animate_with_marks( maze, cur.walk, cur.next, speed );
    animate_walk_to_maze( maze, cur.walk, speed );
    cur.walk = choose_arbitrary_point( maze, Parity_point::odd );

    if ( !cur.walk.row ) {
      return false;
    }

    maze[cur.walk.row][cur.walk.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
    cur.prev = {};
    return true;
  }
  if ( maze[cur.next.row][cur.next.col] & Maze::start_bit_ ) {
    animate_erase_loop( maze, { cur.walk, cur.next }, speed );
    cur.walk = cur.next;
    cur.prev = {};
    const Maze::Backtrack_marker mark
      = ( maze[cur.walk.row][cur.walk.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    cur.prev = { cur.walk.row + direction.row, cur.walk.col + direction.col };
    return true;
  }
  mark_origin_animated( maze, cur.walk, cur.next, speed );
  cur.prev = cur.walk;
  cur.walk = cur.next;
  return true;
}

} // namespace

/* * * * * * * * * * * * * * * * *   Wilson's Path Carving Algorithm  * * * * * * * * * * * * * * */

void generate_wilson_path_carver_maze( Maze& maze )
{
  fill_maze_with_walls( maze );
  /* Important to remember that this maze builds by jumping two squares at a time. Therefore for
   * Wilson's algorithm to work two points must both be even or odd to find each other. For any
   * number N, 2 * N + 1 is always odd, 2 * N is always even.
   */
  std::mt19937 generator( std::random_device {}() );
  std::uniform_int_distribution<int> row_rand( 2, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_rand( 2, maze.col_size() - 2 );
  const Maze::Point start = { 2 * ( row_rand( generator ) / 2 ) + 1, 2 * ( col_rand( generator ) / 2 ) + 1 };

  build_path( maze, start );
  maze[start.row][start.col] |= Maze::builder_bit_;
  Random_walk cur = { {}, { 1, 1 }, {} };
  maze[cur.walk.row][cur.walk.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
  std::vector<int> random_direction_indices( Maze::generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );

  for ( ;; ) {
    maze[cur.walk.row][cur.walk.col] |= Maze::start_bit_;
    std::shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );

    for ( const int& i : random_direction_indices ) {

      const Maze::Point& p = Maze::generate_directions_.at( i );
      cur.next = { cur.walk.row + p.row, cur.walk.col + p.col };
      if ( !is_valid_random_step( maze, cur.next, cur.prev ) ) {
        continue;
      }
      if ( !continue_random_walks( maze, cur ) ) {
        return;
      }
      break;
    }
  }
}

void animate_wilson_path_carver_maze( Maze& maze, Builder_speed speed )
{
  const Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  fill_maze_with_walls_animated( maze );
  clear_and_flush_grid( maze );
  std::mt19937 generator( std::random_device {}() );
  std::uniform_int_distribution<int> row_rand( 2, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_rand( 2, maze.col_size() - 2 );
  const Maze::Point start = { 2 * ( row_rand( generator ) / 2 ) + 1, 2 * ( col_rand( generator ) / 2 ) + 1 };

  build_path( maze, start );
  flush_cursor_maze_coordinate( maze, start );
  maze[start.row][start.col] |= Maze::builder_bit_;
  Random_walk cur = { {}, { 1, 1 }, {} };
  maze[cur.walk.row][cur.walk.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
  std::vector<int> random_direction_indices( Maze::generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );

  for ( ;; ) {
    maze[cur.walk.row][cur.walk.col] |= Maze::start_bit_;
    std::shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );

    for ( const int& i : random_direction_indices ) {

      const Maze::Point& p = Maze::generate_directions_.at( i );
      cur.next = { cur.walk.row + p.row, cur.walk.col + p.col };
      if ( !is_valid_random_step( maze, cur.next, cur.prev ) ) {
        continue;
      }
      if ( !animate_random_walks( maze, cur, animation ) ) {
        return;
      }
      break;
    }
  }
}

} // namespace Builder
