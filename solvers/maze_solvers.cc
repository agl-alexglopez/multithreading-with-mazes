#include "maze_solvers.hh"
#include <iostream>
#include <random>

namespace Solver {

namespace {

bool is_valid_start_or_finish( const Maze& maze, const Maze::Point& choice )
{
  return choice.row > 0 && choice.row < maze.row_size() - 1 && choice.col > 0 && choice.col < maze.col_size() - 1
    && maze[choice.row][choice.col] & Maze::path_bit_ &&  !( maze[choice.row][choice.col] & finish_bit_ )
      && ! (maze[choice.row][choice.col] & start_bit_ );
}

} // namespace

std::vector<Maze::Point> set_corner_starts( const Maze& maze )
{
  Maze::Point point1 = { 1, 1 };
  if ( !( maze[point1.row][point1.col] & Maze::path_bit_ ) ) {
    point1 = find_nearest_square( maze, point1 );
  }
  Maze::Point point2 = { 1, maze.col_size() - 2 };
  if ( !( maze[point2.row][point2.col] & Maze::path_bit_ ) ) {
    point2 = find_nearest_square( maze, point2 );
  }
  Maze::Point point3 = { maze.row_size() - 2, 1 };
  if ( !( maze[point3.row][point3.col] & Maze::path_bit_ ) ) {
    point3 = find_nearest_square( maze, point3 );
  }
  Maze::Point point4 = { maze.row_size() - 2, maze.col_size() - 2 };
  if ( !( maze[point4.row][point4.col] & Maze::path_bit_ ) ) {
    point4 = find_nearest_square( maze, point4 );
  }
  return { point1, point2, point3, point4 };
}

Maze::Point pick_random_point( const Maze& maze )
{
  std::mt19937 generator( std::random_device{}() );
  std::uniform_int_distribution<int> row_random( 1, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_random( 1, maze.col_size() - 2 );
  Maze::Point choice = { row_random( generator ), col_random( generator ) };
  if ( !is_valid_start_or_finish( maze, choice ) ) {
    choice = find_nearest_square( maze, choice );
  }
  return choice;
}

Maze::Point find_nearest_square( const Maze& maze, const Maze::Point& choice )
{
  // Fanning out from a starting point should work on any medium to large maze.
  for ( const Maze::Point& p : all_directions_ ) {
    Maze::Point next = { choice.row + p.row, choice.col + p.col };
    if ( is_valid_start_or_finish( maze, next ) ) {
      return next;
    }
  }
  // Getting desperate here. We should only need this for very small mazes.
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      if ( is_valid_start_or_finish( maze, { row, col }) ) {
        return { row, col };
      }
    }
  }
  std::cerr << "Could not place a point. Bad point = "
            << "{" << choice.row << "," << choice.col << "}" << std::endl;
  print_maze( maze );
  std::abort();
}

void clear_and_flush_paths( const Maze& maze )
{
  clear_screen();
  print_maze( maze );
}

void clear_screen()
{
  std::cout << ansi_clear_screen_;
}

void print_maze( const Maze& maze )
{
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      print_point( maze, { row, col } );
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

void flush_cursor_path_coordinate( const Maze& maze, const Maze::Point& point )
{
  set_cursor_point( point );
  print_point( maze, point );
  std::cout << std::flush;
}

void print_point( const Maze& maze, const Maze::Point& point )
{
  const Maze::Square& square = maze[point.row][point.col];
  if ( square & finish_bit_ ) {
    std::cout << ansi_finish_;
  } else if ( square & start_bit_ ) {
    std::cout << ansi_start_;
  } else if ( square & thread_mask_ ) {
    Thread_paint thread_color = ( square & thread_mask_ ) >> thread_tag_offset_; // NOLINT
    std::cout << thread_colors_.at( thread_color );
  } else {
    maze.print_maze_square( point.row, point.col );
  }
}

void set_cursor_point( const Maze::Point& point )
{
  std::string cursor_pos = "\033[" + std::to_string( point.row + 1 ) + ";" + std::to_string( point.col + 1 ) + "f";
  std::cout << cursor_pos;
}


void print_hunt_solution_message( std::optional<int> winning_index )
{
  if ( !winning_index ) {
    std::cout << thread_colors_.at( all_threads_failed_index_ ) << " no winner...\n";
    return;
  }
  std::cout << ( thread_colors_.at( thread_masks_.at( winning_index.value() ) >> thread_tag_offset_ ) )
    << " thread won!\n";
}

void print_gather_solution_message()
{
  for ( const Thread_paint& mask : thread_masks_ ) {
    std::cout << thread_colors_.at( mask >> thread_tag_offset_ );
  }
  std::cout << " All threads found their finish squares!\n";
}

void print_overlap_key() {
  std::cout << "┌────────────────────────────────────────────────────────────────┐\n"
            << "│     Overlap Key: 3_THREAD | 2_THREAD | 1_THREAD | 0_THREAD     │\n"
            << "├────────────┬────────────┬────────────┬────────────┬────────────┤\n"
            << "│ " << thread_colors_.at( 1 ) << " = 0      │ " << thread_colors_.at( 2 ) << " = 1      │ "
            << thread_colors_.at( 3 ) << " = 1|0    │ " << thread_colors_.at( 4 ) << " = 2      │ " << thread_colors_.at( 5 )
            << " = 2|0    │\n"
            << "├────────────┼────────────┼────────────┼────────────┼────────────┤\n"
            << "│ " << thread_colors_.at( 6 ) << " = 2|1    │ " << thread_colors_.at( 7 ) << " = 2|1|0  │ "
            << thread_colors_.at( 8 ) << " = 3      │ " << thread_colors_.at( 9 ) << " = 3|0    │ " << thread_colors_.at( 10 )
            << " = 3|1    │\n"
            << "├────────────┼────────────┼────────────┼────────────┼────────────┤\n"
            << "│ " << thread_colors_.at( 11 ) << " = 3|1|0  │ " << thread_colors_.at( 12 ) << " = 3|2    │ "
            << thread_colors_.at( 13 ) << " = 3|2|0  │ " << thread_colors_.at( 14 ) << " = 3|2|1  │ " << thread_colors_.at( 15 )
            << " = 3|2|1|0│\n"
            << "└────────────┴────────────┴────────────┴────────────┴────────────┘\n";
}

} // namespace Solver