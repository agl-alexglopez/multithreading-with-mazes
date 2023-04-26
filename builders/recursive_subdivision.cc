#include "maze_algorithms.hh"

#include <chrono>
#include <random>
#include <stack>
#include <thread>

namespace Builder {

namespace {

using Height = int;
using Width = int;

int random_even_division( std::mt19937& generator, int axis_limit )
{
  std::uniform_int_distribution<int> divider( 1, ( axis_limit - 2 ) / 2 );
  return 2 * divider( generator );
}

int random_odd_passage( std::mt19937& generator, int axis_limit )
{
  std::uniform_int_distribution<int> divider( 1, ( axis_limit - 2 ) / 2 );
  return 2 * divider( generator ) + 1;
}

} // namespace

void generate_recursive_subdivision_maze( Maze& maze )
{
  build_wall_outline( maze );
  std::mt19937 generator( std::random_device {}() );
  std::stack<std::tuple<Maze::Point, Height, Width>> chamber_stack(
    { { { 0, 0 }, maze.row_size(), maze.col_size() } } );
  while ( !chamber_stack.empty() ) {
    std::tuple<Maze::Point, Height, Width>& chamber = chamber_stack.top();
    const Maze::Point& chamber_offset = std::get<0>( chamber );
    int chamber_height = std::get<1>( chamber );
    int chamber_width = std::get<2>( chamber );
    if ( chamber_height >= chamber_width && chamber_width > 3 ) {
      int divide = random_even_division( generator, chamber_height );
      int passage = random_odd_passage( generator, chamber_width );
      for ( int col = 0; col < chamber_width; col++ ) {
        if ( col != passage ) {
          maze[chamber_offset.row + divide][chamber_offset.col + col]
            &= static_cast<Maze::Square>( ~Maze::path_bit_ );
          build_wall_line( maze, { chamber_offset.row + divide, chamber_offset.col + col } );
        }
      }
      // Remember to shrink height of this branch before we continue down next branch.
      std::get<1>( chamber ) = divide + 1;
      Maze::Point offset = { chamber_offset.row + divide, chamber_offset.col };
      chamber_stack.push( std::make_tuple( offset, chamber_height - divide, chamber_width ) );
    } else if ( chamber_width > chamber_height && chamber_height > 3 ) {
      int divide = random_even_division( generator, chamber_width );
      int passage = random_odd_passage( generator, chamber_height );
      for ( int row = 0; row < chamber_height; row++ ) {
        if ( row != passage ) {
          maze[chamber_offset.row + row][chamber_offset.col + divide]
            &= static_cast<Maze::Square>( ~Maze::path_bit_ );
          build_wall_line( maze, { chamber_offset.row + row, chamber_offset.col + divide } );
        }
      }
      // In this case, we are shrinking the width.
      std::get<2>( chamber ) = divide + 1;
      Maze::Point offset = { chamber_offset.row, chamber_offset.col + divide };
      chamber_stack.push( std::make_tuple( offset, chamber_height, chamber_width - divide ) );
    } else {
      chamber_stack.pop();
    }
  }
}

void animate_recursive_subdivision_maze( Maze& maze, Builder_speed speed )
{
  Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  build_wall_outline( maze );
  std::mt19937 generator( std::random_device {}() );
  std::stack<std::tuple<Maze::Point, Height, Width>> chamber_stack(
    { { { 0, 0 }, maze.row_size(), maze.col_size() } } );
  while ( !chamber_stack.empty() ) {
    std::tuple<Maze::Point, Height, Width>& chamber = chamber_stack.top();
    const Maze::Point& chamber_offset = std::get<0>( chamber );
    int chamber_height = std::get<1>( chamber );
    int chamber_width = std::get<2>( chamber );
    if ( chamber_height >= chamber_width && chamber_width > 3 ) {
      int divide = random_even_division( generator, chamber_height );
      int passage = random_odd_passage( generator, chamber_width );
      for ( int col = 0; col < chamber_width; col++ ) {
        if ( col != passage ) {
          maze[chamber_offset.row + divide][chamber_offset.col + col]
            &= static_cast<Maze::Square>( ~Maze::path_bit_ );
          build_wall_line_animated( maze, { chamber_offset.row + divide, chamber_offset.col + col }, animation );
        }
      }
      std::get<1>( chamber ) = divide + 1;
      Maze::Point offset = { chamber_offset.row + divide, chamber_offset.col };
      chamber_stack.push( std::make_tuple( offset, chamber_height - divide, chamber_width ) );
    } else if ( chamber_width > chamber_height && chamber_height > 3 ) {
      int divide = random_even_division( generator, chamber_width );
      int passage = random_odd_passage( generator, chamber_height );
      for ( int row = 0; row < chamber_height; row++ ) {
        if ( row != passage ) {
          maze[chamber_offset.row + row][chamber_offset.col + divide]
            &= static_cast<Maze::Square>( ~Maze::path_bit_ );
          build_wall_line_animated( maze, { chamber_offset.row + row, chamber_offset.col + divide }, animation );
        }
      }
      std::get<2>( chamber ) = divide + 1;
      Maze::Point offset = { chamber_offset.row, chamber_offset.col + divide };
      chamber_stack.push( std::make_tuple( offset, chamber_height, chamber_width - divide ) );
    } else {
      chamber_stack.pop();
    }
  }
}

} // namespace Builder
