#include "maze.hh"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

namespace Builder {

Maze::Maze( const Maze_args& args )
  : maze_row_size_( static_cast<int>( args.odd_rows) ),
    maze_col_size_( static_cast<int>( args.odd_cols ) ),
    maze_( maze_row_size_, std::vector<Square>( maze_col_size_, 0 ) ),
    wall_style_index_( static_cast<int>( args.style ) )
{}

std::vector<Maze::Square>& Maze::operator[]( uint64_t index ) {
  return maze_.at( index );
}

const std::vector<Maze::Square>& Maze::operator[]( uint64_t index ) const {
  return maze_.at( index );
}

int Maze::row_size() const {
  return maze_row_size_;
}

int Maze::col_size() const {
  return maze_col_size_;
}

const std::array<std::string_view,16>& Maze::wall_style() const {
  return wall_styles_.at( wall_style_index_ );
}

bool operator==( const Maze::Point& lhs, const Maze::Point& rhs )
{
  return lhs.row == rhs.row && lhs.col == rhs.col;
}

bool operator!=( const Maze::Point& lhs, const Maze::Point& rhs )
{
  return !( lhs == rhs );
}

} // namespace Builder
