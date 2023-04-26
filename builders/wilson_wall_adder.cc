#include "maze_algorithms.hh"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

namespace Builder {

namespace {

struct Loop {
  Maze::Point& walk;
  Maze::Point& root;
};

struct Random_walk {
  Maze::Point prev;
  Maze::Point walk;
  Maze::Point next;
};

bool is_valid_walk_step( Maze& maze, const Maze::Point& next, const Maze::Point& prev)
{
  return next.row >= 0 && next.row < maze.row_size()
    && next.col >= 0 && next.col < maze.col_size() && next != prev;
}

void join_walk_walls( Maze& maze, const Maze::Point& cur, const Maze::Point& next )
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
    std::cerr << "Wall join error. Step through wall didn't work" << std::endl;
    abort();
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze[next.row][next.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze.build_wall_line( cur.row, cur.col );
  maze.build_wall_line( wall.row, wall.col );
  maze.build_wall_line( next.row, next.col );
}

void animate_walk_walls( Maze& maze, const Maze::Point& cur, const Maze::Point& next )
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
    std::cerr << "Wall join error. Step through wall didn't work" << std::endl;
    std::abort();
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze[next.row][next.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze.build_wall_line_animated( cur.row, cur.col );
  maze.build_wall_line_animated( wall.row, wall.col );
  maze.build_wall_line_animated( next.row, next.col );
}

void connect_walk_to_maze ( Maze& maze, const Maze::Point& walk )
{
  Maze::Point cur = walk;
  while ( maze[cur.row][cur.col] & Maze::markers_mask_ ) {
    Maze::Backtrack_marker mark
      = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    join_walk_walls( maze, cur, next );
    // Clean up after ourselves and leave no marks behind for the maze solvers.
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
    cur = next;
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::markers_mask_ );
  maze.build_wall_line( cur.row, cur.col );
}

void animate_walk_to_maze( Maze& maze, const Maze::Point& walk )
{
  Maze::Point cur = walk;
  while ( maze[cur.row][cur.col] & Maze::markers_mask_ ) {
    Maze::Backtrack_marker mark
      = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    animate_walk_walls( maze, cur, next );
    // Clean up after ourselves and leave no marks behind for the maze solvers.
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
    maze.flush_cursor_maze_coordinate( cur.row, cur.col );
    std::this_thread::sleep_for( std::chrono::microseconds( maze.build_speed() ) );
    cur = next;
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::markers_mask_ );
  maze.build_wall_line_animated( cur.row, cur.col );
}

void erase_loop( Maze& maze, const Loop& loop )
{
  Maze::Point cur = loop.walk;
  while ( cur != loop.root ) {
    maze[cur.row][cur.col] &= static_cast<Maze::Square> ( ~Maze::start_bit_ );
    Maze::Backtrack_marker mark
      = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
    cur = next;
  }
}

void animate_erase_loop( Maze& maze, const Loop& loop )
{
  Maze::Point cur = loop.walk;
  while ( cur != loop.root ) {
    maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit_ );
    Maze::Backtrack_marker mark
      = ( maze[cur.row][cur.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker> ( ~Maze::markers_mask_ );
    maze.flush_cursor_maze_coordinate( cur.row, cur.col );
    std::this_thread::sleep_for( std::chrono::microseconds( maze.build_speed() ) );
    cur = next;
  }
}

bool continue_random_walks( Maze& maze, Random_walk& cur )
{
  if ( maze.has_builder_bit( cur.next ) ) {
    join_walk_walls( maze, cur.walk, cur.next );
    connect_walk_to_maze( maze, cur.walk );
    cur.walk = maze.choose_arbitrary_point( Maze::Parity_point::even );

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
    Maze::Backtrack_marker mark
      = ( maze[cur.walk.row][cur.walk.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    cur.prev = { cur.walk.row + direction.row, cur.walk.col + direction.col };
    return true;
  }

  maze.mark_origin( cur.walk, cur.next );
  cur.prev = cur.walk;
  cur.walk = cur.next;
  return true;
};

bool animate_random_walks( Maze& maze, Random_walk& cur )
{
  if ( maze.has_builder_bit( cur.next ) ) {
    animate_walk_walls( maze, cur.walk, cur.next );
    animate_walk_to_maze( maze, cur.walk );
    cur.walk = maze.choose_arbitrary_point( Maze::Parity_point::even );

    if ( !cur.walk.row ) {
      return false;
    }

    maze[cur.walk.row][cur.walk.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask_ );
    cur.prev = {};
    return true;
  }
  if ( maze[cur.next.row][cur.next.col] & Maze::start_bit_ ) {
    animate_erase_loop( maze, { cur.walk, cur.next } );
    cur.walk = cur.next;
    cur.prev = {};
    Maze::Backtrack_marker mark
      = ( maze[cur.walk.row][cur.walk.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    const Maze::Point& direction = Maze::backtracking_marks_.at( mark );
    cur.prev = { cur.walk.row + direction.row, cur.walk.col + direction.col };
    return true;
  }
  maze.mark_origin_animated( cur.walk, cur.next );
  cur.prev = cur.walk;
  cur.walk = cur.next;
  return true;
}

} // namespace

/* * * * * * * * * * * * * * * *   Wilson Wall Adder Algorithm   * * * * * * * * * * * * * * * * */

void generate_wilson_wall_adder_maze( Maze& maze )
{
  maze.build_wall_outline();
  // Walls must start and connect between even squares.
  std::mt19937 generator( std::random_device{}() );
  std::uniform_int_distribution<int> row_rand( 2, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_rand( 2, maze.col_size() - 2 );
  Random_walk cur
    = { {}, { 2 * ( row_rand( generator ) / 2 ), 2 * ( col_rand( generator ) / 2 ) }, {} };
  std::vector<int> random_direction_indices( Maze::generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  for ( ;; ) {
    // Every walk is distinguished from the maze with the start bit.
    maze[cur.walk.row][cur.walk.col] |= Maze::start_bit_;
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = Maze::generate_directions_.at( i );
      cur.next = { cur.walk.row + p.row, cur.walk.col + p.col };
      if ( !is_valid_walk_step( maze, cur.walk, cur.prev ) ) {
        continue;
      }
      if ( !continue_random_walks( maze, cur ) ) {
        return;
      }
      break;
    }
  }

}

void animate_wilson_wall_adder_maze( Maze& maze )
{
  maze.build_wall_outline();
  std::mt19937 generator( std::random_device{}() );
  std::uniform_int_distribution<int> row_rand( 2, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_rand( 2, maze.col_size() - 2 );
  Random_walk cur
    = { {}, { 2 * ( row_rand( generator ) / 2 ), 2 * ( col_rand( generator ) / 2 ) }, {} };

  std::vector<int> random_direction_indices( Maze::generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  for ( ;; ) {
    // Every walk is distinguished from the maze with the start bit.
    maze[cur.walk.row][cur.walk.col] |= Maze::start_bit_;
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = Maze::generate_directions_.at( i );
      cur.next = { cur.walk.row + p.row, cur.walk.col + p.col };
      if ( !is_valid_walk_step( maze, cur.next, cur.prev ) ) {
        continue;
      }
      if ( !animate_random_walks( maze, cur ) ) {
        return;
      }
      break;
    }
  }
}

} // namespace Builder
