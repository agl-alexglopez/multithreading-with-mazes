module;
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <thread>
#include <vector>
export module labyrinth:wilson_wall_adder;
import :maze;
import :speed;
import :build_utilities;

///////////////////////////////////   Exported Interface  ///////////////////////////////////////////

export namespace Wilson_wall_adder {
void generate_maze( Maze::Maze& maze );
void animate_maze( Maze::Maze& maze, Speed::Speed speed );
} // namespace Wilson_wall_adder

//////////////////////////////////   Implementation   /////////////////////////////////////////////////

namespace {

struct Loop
{
  Maze::Point walk;
  Maze::Point root;
};

struct Random_walk
{
  Maze::Point prev;
  Maze::Point walk;
  Maze::Point next;
};

bool is_valid_walk_step( Maze::Maze& maze, const Maze::Point& next, const Maze::Point& prev )
{
  return next.row >= 0 && next.row < maze.row_size() && next.col >= 0 && next.col < maze.col_size() && next != prev;
}

void join_walk_walls( Maze::Maze& maze, const Maze::Point& cur, const Maze::Point& next )
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
    std::cerr << "Wall join error. Step through wall didn't work"
              << "\n";
    std::abort();
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit );
  maze[next.row][next.col] &= static_cast<Maze::Square>( ~Maze::start_bit );
  Butil::build_wall_line( maze, cur );
  Butil::build_wall_line( maze, wall );
  Butil::build_wall_line( maze, next );
}

void animate_walk_walls( Maze::Maze& maze,
                         const Maze::Point& cur,
                         const Maze::Point& next,
                         Speed::Speed_unit speed )
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
    std::cerr << "Wall join error. Step through wall didn't work"
              << "\n";
    std::abort();
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit );
  maze[next.row][next.col] &= static_cast<Maze::Square>( ~Maze::start_bit );
  Butil::build_wall_line_animated( maze, cur, speed );
  Butil::build_wall_line_animated( maze, wall, speed );
  Butil::build_wall_line_animated( maze, next, speed );
}

void connect_walk_to_maze( Maze::Maze& maze, const Maze::Point& walk )
{
  Maze::Point cur = walk;
  while ( maze[cur.row][cur.col] & Maze::markers_mask ) {
    const Maze::Backtrack_marker mark {
      static_cast<uint16_t>( ( maze[cur.row][cur.col] & Maze::markers_mask ).load() >> Maze::marker_shift ) };
    const Maze::Point& direction = Maze::backtracking_marks.at( mark.load() );
    const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    join_walk_walls( maze, cur, next );
    // Clean up after ourselves and leave no marks behind for the maze solvers.
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask );
    cur = next;
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit );
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::markers_mask );
  Butil::build_wall_line( maze, cur );
}

void animate_walk_to_maze( Maze::Maze& maze, const Maze::Point& walk, Speed::Speed_unit speed )
{
  Maze::Point cur = walk;
  while ( maze[cur.row][cur.col] & Maze::markers_mask ) {
    const Maze::Backtrack_marker mark {
      static_cast<uint16_t>( ( maze[cur.row][cur.col] & Maze::markers_mask ).load() >> Maze::marker_shift ) };
    const Maze::Point& direction = Maze::backtracking_marks.at( mark.load() );
    const Maze::Point& half_step = Maze::backtracking_half_marks.at( mark.load() );
    const Maze::Point half = { cur.row + half_step.row, cur.col + half_step.col };
    const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    animate_walk_walls( maze, cur, next, speed );
    // Clean up after ourselves and leave no marks behind for the maze solvers.
    maze[half.row][half.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask );
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask );
    Butil::flush_cursor_maze_coordinate( maze, half );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
    Butil::flush_cursor_maze_coordinate( maze, cur );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
    cur = next;
  }
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit );
  maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::markers_mask );
  Butil::build_wall_line_animated( maze, cur, speed );
}

void erase_loop( Maze::Maze& maze, const Loop& loop )
{
  Maze::Point cur = loop.walk;
  while ( cur != loop.root ) {
    maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit );
    const Maze::Backtrack_marker mark {
      static_cast<uint16_t>( ( maze[cur.row][cur.col] & Maze::markers_mask ).load() >> Maze::marker_shift ) };
    const Maze::Point& direction = Maze::backtracking_marks.at( mark.load() );
    const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask );
    cur = next;
  }
}

void animate_erase_loop( Maze::Maze& maze, const Loop& loop, Speed::Speed_unit speed )
{
  Maze::Point cur = loop.walk;
  while ( cur != loop.root ) {
    maze[cur.row][cur.col] &= static_cast<Maze::Square>( ~Maze::start_bit );
    const Maze::Backtrack_marker mark {
      static_cast<uint16_t>( ( maze[cur.row][cur.col] & Maze::markers_mask ).load() >> Maze::marker_shift ) };
    const Maze::Point& direction = Maze::backtracking_marks.at( mark.load() );
    const Maze::Point& half_step = Maze::backtracking_half_marks.at( mark.load() );
    const Maze::Point half = { cur.row + half_step.row, cur.col + half_step.col };
    const Maze::Point next = { cur.row + direction.row, cur.col + direction.col };
    maze[half.row][half.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask );
    maze[cur.row][cur.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask );
    Butil::flush_cursor_maze_coordinate( maze, half );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
    Butil::flush_cursor_maze_coordinate( maze, cur );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
    cur = next;
  }
}

bool continue_random_walks( Maze::Maze& maze, Random_walk& cur )
{
  if ( Butil::has_builder_bit( maze, cur.next ) ) {
    join_walk_walls( maze, cur.walk, cur.next );
    connect_walk_to_maze( maze, cur.walk );
    cur.walk = choose_arbitrary_point( maze, Butil::Parity_point::even );

    if ( !cur.walk.row ) {
      return false;
    }

    maze[cur.walk.row][cur.walk.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask );
    cur.prev = {};
    return true;
  }

  if ( maze[cur.next.row][cur.next.col] & Maze::start_bit ) {
    erase_loop( maze, { cur.walk, cur.next } );
    cur.walk = cur.next;
    cur.prev = {};
    const Maze::Backtrack_marker mark { static_cast<uint16_t>(
      ( maze[cur.walk.row][cur.walk.col] & Maze::markers_mask ).load() >> Maze::marker_shift ) };
    const Maze::Point& direction = Maze::backtracking_marks.at( mark.load() );
    cur.prev = { cur.walk.row + direction.row, cur.walk.col + direction.col };
    return true;
  }

  Butil::mark_origin( maze, cur.walk, cur.next );
  cur.prev = cur.walk;
  cur.walk = cur.next;
  return true;
};

bool animate_random_walks( Maze::Maze& maze, Random_walk& cur, Speed::Speed_unit speed )
{
  if ( Butil::has_builder_bit( maze, cur.next ) ) {
    animate_walk_walls( maze, cur.walk, cur.next, speed );
    animate_walk_to_maze( maze, cur.walk, speed );
    cur.walk = choose_arbitrary_point( maze, Butil::Parity_point::even );

    if ( !cur.walk.row ) {
      return false;
    }

    maze[cur.walk.row][cur.walk.col] &= static_cast<Maze::Backtrack_marker>( ~Maze::markers_mask );
    cur.prev = {};
    return true;
  }
  if ( maze[cur.next.row][cur.next.col] & Maze::start_bit ) {
    animate_erase_loop( maze, { cur.walk, cur.next }, speed );
    cur.walk = cur.next;
    cur.prev = {};
    const Maze::Backtrack_marker mark { static_cast<uint16_t>(
      ( maze[cur.walk.row][cur.walk.col] & Maze::markers_mask ).load() >> Maze::marker_shift ) };
    const Maze::Point& direction = Maze::backtracking_marks.at( mark.load() );
    cur.prev = { cur.walk.row + direction.row, cur.walk.col + direction.col };
    return true;
  }
  Butil::mark_origin_animated( maze, cur.walk, cur.next, speed );
  cur.prev = cur.walk;
  cur.walk = cur.next;
  return true;
}

} // namespace

/* * * * * * * * * * * * * * * *   Wilson Wall Adder Algorithm   * * * * * * * * * * * * * * * * */

namespace Wilson_wall_adder {

void generate_maze( Maze::Maze& maze )
{
  Butil::build_wall_outline( maze );
  // Walls must start and connect between even squares.
  std::mt19937 generator( std::random_device {}() );
  std::uniform_int_distribution<int> row_rand( 2, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_rand( 2, maze.col_size() - 2 );
  Random_walk cur = { {}, { 2 * ( row_rand( generator ) / 2 ), 2 * ( col_rand( generator ) / 2 ) }, {} };
  std::vector<int> random_direction_indices( Maze::build_dirs.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  for ( ;; ) {
    // Every walk is distinguished from the maze with the start bit.
    maze[cur.walk.row][cur.walk.col] |= Maze::start_bit;
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = Maze::build_dirs.at( i );
      cur.next = { cur.walk.row + p.row, cur.walk.col + p.col };
      if ( !is_valid_walk_step( maze, cur.next, cur.prev ) ) {
        continue;
      }
      if ( !continue_random_walks( maze, cur ) ) {
        Butil::clear_and_flush_grid( maze );
        return;
      }
      break;
    }
  }
}

void animate_maze( Maze::Maze& maze, Speed::Speed speed )
{
  const Speed::Speed_unit animation = Butil::builder_speeds.at( static_cast<int>( speed ) );
  Butil::build_wall_outline( maze );
  Butil::clear_and_flush_grid( maze );
  std::mt19937 generator( std::random_device {}() );
  std::uniform_int_distribution<int> row_rand( 2, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_rand( 2, maze.col_size() - 2 );
  Random_walk cur = { {}, { 2 * ( row_rand( generator ) / 2 ), 2 * ( col_rand( generator ) / 2 ) }, {} };

  std::vector<int> random_direction_indices( Maze::build_dirs.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  for ( ;; ) {
    // Every walk is distinguished from the maze with the start bit.
    maze[cur.walk.row][cur.walk.col] |= Maze::start_bit;
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = Maze::build_dirs.at( i );
      cur.next = { cur.walk.row + p.row, cur.walk.col + p.col };
      if ( !is_valid_walk_step( maze, cur.next, cur.prev ) ) {
        continue;
      }
      if ( !animate_random_walks( maze, cur, animation ) ) {
        return;
      }
      break;
    }
  }
}

} // namespace Wilson_wall_adder
