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
    builder_speed_units_( builder_speeds_.at( static_cast<int>( args.builder_speed ) ) ),
    wall_style_index_( static_cast<int>( args.style ) ),
    modification_( args.modification )
{}

void Maze::add_positive_slope( int row, int col ) {
  auto row_size = static_cast<float>( maze_row_size_ ) - 2.0F;
  auto col_size = static_cast<float>( maze_col_size_ ) - 2.0F;
  auto cur_row = static_cast<float>( row );
  // y = mx + b. We will get the negative slope. This line goes top left to bottom right.
  float slope = ( 2.0F - row_size ) / ( 2.0F - col_size );
  float b = 2.0F - ( 2.0F * slope );
  int on_line = static_cast<int>( ( cur_row - b ) / slope );
  if ( col == on_line && col < maze_col_size_ - 2 && col > 1 ) {
    // An X is hard to notice and might miss breaking wall lines so make it wider.
    build_path( row, col );
    if ( col + 1 < maze_col_size_ - 2 ) {
      build_path( row, col + 1 );
    }
    if ( col - 1 > 1 ) {
      build_path( row, col - 1 );
    }
    if ( col + 2 < maze_col_size_ - 2 ) {
      build_path( row, col + 2 );
    }
    if ( col - 2 > 1 ) {
      build_path( row, col - 2 );
    }
  }
}

void Maze::add_positive_slope_animated( int row, int col ) {
  auto row_size = static_cast<float>( maze_row_size_ ) - 2.0F;
  auto col_size = static_cast<float>( maze_col_size_ ) - 2.0F;
  auto cur_row = static_cast<float>( row );
  // y = mx + b. We will get the negative slope. This line goes top left to bottom right.
  float slope = ( 2.0F - row_size ) / ( 2.0F - col_size );
  float b = 2.0F - ( 2.0F * slope );
  int on_line = static_cast<int>( ( cur_row - b ) / slope );
  if ( col == on_line && col < maze_col_size_ - 2 && col > 1 ) {
    // An X is hard to notice and might miss breaking wall lines so make it wider.
    build_path_animated( row, col );
    if ( col + 1 < maze_col_size_ - 2 ) {
      build_path_animated( row, col + 1 );
    }
    if ( col - 1 > 1 ) {
      build_path_animated( row, col - 1 );
    }
    if ( col + 2 < maze_col_size_ - 2 ) {
      build_path_animated( row, col + 2 );
    }
    if ( col - 2 > 1 ) {
      build_path_animated( row, col - 2 );
    }
  }

}

void Maze::add_negative_slope( int row, int col ) {
  auto row_size = static_cast<float>( maze_row_size_ ) - 2.0F;
  auto col_size = static_cast<float>( maze_col_size_ ) - 2.0F;
  auto cur_row = static_cast<float>( row );
  float slope = ( 2.0F - row_size ) / ( col_size - 2.0F );
  float b = row_size - ( 2.0F * slope );
  int on_line = static_cast<int>( ( cur_row - b ) / slope );
  if ( col == on_line && col > 1 && col < maze_col_size_ - 2 && row < maze_row_size_ - 2 ) {
    build_path( row, col );
    if ( col + 1 < maze_col_size_ - 2 ) {
      build_path( row, col + 1 );
    }
    if ( col - 1 > 1 ) {
      build_path( row, col - 1 );
    }
    if ( col + 2 < maze_col_size_ - 2 ) {
      build_path( row, col + 2 );
    }
    if ( col - 2 > 1 ) {
      build_path( row, col - 2 );
    }
  }
}

void Maze::add_negative_slope_animated( int row, int col ) {
  auto row_size = static_cast<float>( maze_row_size_ ) - 2.0F;
  auto col_size = static_cast<float>( maze_col_size_ ) - 2.0F;
  auto cur_row = static_cast<float>( row );
  float slope = ( 2.0F - row_size ) / ( col_size - 2.0F );
  float b = row_size - ( 2.0F * slope );
  int on_line = static_cast<int>( ( cur_row - b ) / slope );
  if ( col == on_line && col > 1 && col < maze_col_size_ - 2 && row < maze_row_size_ - 2 ) {
    build_path_animated( row, col );
    if ( col + 1 < maze_col_size_ - 2 ) {
      build_path_animated( row, col + 1 );
    }
    if ( col - 1 > 1 ) {
      build_path_animated( row, col - 1 );
    }
    if ( col + 2 < maze_col_size_ - 2 ) {
      build_path_animated( row, col + 2 );
    }
    if ( col - 2 > 1 ) {
      build_path_animated( row, col - 2 );
    }
  }
}

void Maze::add_modification( int row, int col )
{
  if ( modification_ == Maze_modification::add_cross ) {
    if ( ( row == maze_row_size_ / 2 && col > 1 && col < maze_col_size_ - 2 )
         || ( col == maze_col_size_ / 2 && row > 1 && row < maze_row_size_ - 2 ) ) {
      build_path( row, col );
      if ( col + 1 < maze_col_size_ - 2 ) {
        build_path( row, col + 1 );
      }
    }
  } else if ( modification_ == Maze_modification::add_x ) {
    add_positive_slope( row, col );
    add_negative_slope( row, col );
  }
}

void Maze::add_modification_animated( int row, int col )
{
  if ( modification_ == Maze_modification::add_cross ) {
    if ( ( row == maze_row_size_ / 2 && col > 1 && col < maze_col_size_ - 2 )
         || ( col == maze_col_size_ / 2 && row > 1 && row < maze_row_size_ - 2 ) ) {
      build_path_animated( row, col );
      if ( col + 1 < maze_col_size_ - 2 ) {
        build_path_animated( row, col + 1 );
      }
    }
  } else if ( modification_ == Maze_modification::add_x ) {
    add_positive_slope_animated( row, col );
    add_negative_slope_animated( row, col );
  }
}

void Maze::build_wall_outline()
{
  for ( int row = 0; row < maze_row_size_; row++ ) {
    for ( int col = 0; col < maze_col_size_; col++ ) {
      if ( col == 0 || col == maze_col_size_ - 1 || row == 0 || row == maze_row_size_ - 1 ) {
        maze_[row][col] |= builder_bit_;
        build_wall_carefully( row, col );
      } else {
        build_path( row, col );
      }
    }
  }
}

Maze::Point Maze::choose_arbitrary_point( Maze::Parity_point parity ) const {
  int init = parity == Parity_point::even ? 2 : 1;
  for ( int row = init; row < maze_row_size_ - 1; row += 2 ) {
    for ( int col = init; col < maze_col_size_ - 1; col += 2 ) {
      Point cur = { row, col };
      if ( !( maze_[cur.row][cur.col] & builder_bit_ ) ) {
        return cur;
      }
    }
  }
  return { 0, 0 };
}

bool Maze::can_build_new_square( const Point& next ) const {
  return next.row > 0 && next.row < maze_row_size_ - 1
    && next.col > 0 && next.col < maze_col_size_ - 1
    && !( maze_[next.row][next.col] & builder_bit_ );
}

bool Maze::has_builder_bit( const Point& next ) const {
  return maze_[next.row][next.col] & builder_bit_;
}

bool Maze::is_square_within_perimeter_walls( const Point& next ) const
{
  return next.row < maze_row_size_ - 1 && next.row > 0
    && next.col < maze_col_size_ - 1 && next.col > 0;
}

void Maze::build_wall_line( int row, int col )
{
  Wall_line wall = 0b0;
  if ( row - 1 >= 0 && !( maze_[row - 1][col] & path_bit_ ) ) {
    wall |= north_wall_;
    maze_[row - 1][col] |= south_wall_;
  }
  if ( row + 1 < maze_row_size_ && !( maze_[row + 1][col] & path_bit_ ) ) {
    wall |= south_wall_;
    maze_[row + 1][col] |= north_wall_;
  }
  if ( col - 1 >= 0 && !( maze_[row][col - 1] & path_bit_ ) ) {
    wall |= west_wall_;
    maze_[row][col - 1] |= east_wall_;
  }
  if ( col + 1 < maze_col_size_ && !( maze_[row][col + 1] & path_bit_ ) ) {
    wall |= east_wall_;
    maze_[row][col + 1] |= west_wall_;
  }
  maze_[row][col] |= wall;
  maze_[row][col] |= builder_bit_;
  maze_[row][col] &= static_cast<Square>(~path_bit_);
}

void Maze::build_wall_line_animated( int row, int col )
{
  Wall_line wall = 0b0;
  if ( row - 1 >= 0 && !( maze_[row - 1][col] & path_bit_ ) ) {
    wall |= north_wall_;
    maze_[row - 1][col] |= south_wall_;
    flush_cursor_maze_coordinate( row - 1, col );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  if ( row + 1 < maze_row_size_ && !( maze_[row + 1][col] & path_bit_ ) ) {
    wall |= south_wall_;
    maze_[row + 1][col] |= north_wall_;
    flush_cursor_maze_coordinate( row + 1, col );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  if ( col - 1 >= 0 && !( maze_[row][col - 1] & path_bit_ ) ) {
    wall |= west_wall_;
    maze_[row][col - 1] |= east_wall_;
    flush_cursor_maze_coordinate( row, col - 1 );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  if ( col + 1 < maze_col_size_ && !( maze_[row][col + 1] & path_bit_ ) ) {
    wall |= east_wall_;
    maze_[row][col + 1] |= west_wall_;
    flush_cursor_maze_coordinate( row, col + 1 );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  maze_[row][col] |= wall;
  maze_[row][col] |= builder_bit_;
  maze_[row][col] &= static_cast<Square>(~path_bit_);
  flush_cursor_maze_coordinate( row, col );
  std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
}

void Maze::clear_for_wall_adders()
{
  for ( int row = 0; row < maze_row_size_; row++ ) {
    for ( int col = 0; col < maze_col_size_; col++ ) {
      if ( col == 0 || col == maze_col_size_ - 1 || row == 0 || row == maze_row_size_ - 1 ) {
        maze_[row][col] |= builder_bit_;
      } else {
        build_path( row, col );
      }
    }
  }
}

void Maze::mark_origin( const Point& walk, const Point& next )
{
  if ( next.row > walk.row ) {
    maze_[next.row][next.col] |= from_north_;
  } else if ( next.row < walk.row ) {
    maze_[next.row][next.col] |= from_south_;
  } else if ( next.col < walk.col ) {
    maze_[next.row][next.col] |= from_east_;
  } else if ( next.col > walk.col ) {
    maze_[next.row][next.col] |= from_west_;
  }
}

void Maze::mark_origin_animated( const Point& walk, const Point& next )
{
  if ( next.row > walk.row ) {
    maze_[next.row][next.col] |= from_north_;
  } else if ( next.row < walk.row ) {
    maze_[next.row][next.col] |= from_south_;
  } else if ( next.col < walk.col ) {
    maze_[next.row][next.col] |= from_east_;
  } else if ( next.col > walk.col ) {
    maze_[next.row][next.col] |= from_west_;
  }
  flush_cursor_maze_coordinate( next.row, next.col );
  std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
}

/* * * * * * * * * Path Carvers * * * * * * * */

void Maze::fill_maze_with_walls()
{
  for ( int row = 0; row < maze_row_size_; row++ ) {
    for ( int col = 0; col < maze_col_size_; col++ ) {
      build_wall( row, col );
      if ( modification_ != Maze_modification::none ) {
        add_modification( row, col );
      }
    }
  }
}

void Maze::fill_maze_with_walls_animated()
{
  clear_screen();
  for ( int row = 0; row < maze_row_size_; row++ ) {
    for ( int col = 0; col < maze_col_size_; col++ ) {
      build_wall( row, col );
      if ( modification_ != Maze_modification::none ) {
        add_modification_animated( row, col );
      }
    }
  }
}

void Maze::carve_path_walls( int row, int col )
{
  maze_[row][col] |= path_bit_;
  if ( row - 1 >= 0 ) {
    maze_[row - 1][col] &= static_cast<Square>( ~south_wall_ );
  }
  if ( row + 1 < maze_row_size_ ) {
    maze_[row + 1][col] &= static_cast<Square>(~north_wall_);
  }
  if ( col - 1 >= 0 ) {
    maze_[row][col - 1] &= static_cast<Square>(~east_wall_);
  }
  if ( col + 1 < maze_col_size_ ) {
    maze_[row][col + 1] &= static_cast<Square>(~west_wall_);
  }
  maze_[row][col] |= builder_bit_;
}

// The animated version tries to save cursor movements if they are not necessary.
void Maze::carve_path_walls_animated( int row, int col )
{
  maze_[row][col] |= path_bit_;
  flush_cursor_maze_coordinate( row, col );
  std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  if ( row - 1 >= 0 && !( maze_[row - 1][col] & path_bit_ ) ) {
    maze_[row - 1][col] &= static_cast<Square>(~south_wall_);
    flush_cursor_maze_coordinate( row - 1, col );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  if ( row + 1 < maze_row_size_ && !( maze_[row + 1][col] & path_bit_ ) ) {
    maze_[row + 1][col] &= static_cast<Square>(~north_wall_);
    flush_cursor_maze_coordinate( row + 1, col );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  if ( col - 1 >= 0 && !( maze_[row][col - 1] & path_bit_ ) ) {
    maze_[row][col - 1] &= static_cast<Square>(~east_wall_);
    flush_cursor_maze_coordinate( row, col - 1 );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  if ( col + 1 < maze_col_size_ && !( maze_[row][col + 1] & path_bit_ ) ) {
    maze_[row][col + 1] &= static_cast<Square>(~west_wall_);
    flush_cursor_maze_coordinate( row, col + 1 );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  maze_[row][col] |= builder_bit_;
}

void Maze::carve_path_markings( const Point& cur, const Point& next )
{
  Point wall = cur;
  carve_path_walls( cur.row, cur.col );
  if ( next.row < cur.row ) {
    wall.row--;
    maze_[next.row][next.col] |= from_south_;
  } else if ( next.row > cur.row ) {
    wall.row++;
    maze_[next.row][next.col] |= from_north_;
  } else if ( next.col < cur.col ) {
    wall.col--;
    maze_[next.row][next.col] |= from_east_;
  } else if ( next.col > cur.col ) {
    wall.col++;
    maze_[next.row][next.col] |= from_west_;
  } else {
    std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
  }
  carve_path_walls( next.row, next.col );
  carve_path_walls( wall.row, wall.col );
}

void Maze::carve_path_markings_animated( const Point& cur, const Point& next )
{
  Point wall = cur;
  carve_path_walls_animated( cur.row, cur.col );
  if ( next.row < cur.row ) {
    wall.row--;
    maze_[next.row][next.col] |= from_south_;
  } else if ( next.row > cur.row ) {
    wall.row++;
    maze_[next.row][next.col] |= from_north_;
  } else if ( next.col < cur.col ) {
    wall.col--;
    maze_[next.row][next.col] |= from_east_;
  } else if ( next.col > cur.col ) {
    wall.col++;
    maze_[next.row][next.col] |= from_west_;
  } else {
    std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
  }
  carve_path_walls_animated( next.row, next.col );
  carve_path_walls_animated( wall.row, wall.col );
}

void Maze::join_squares( const Point& cur, const Point& next )
{
  Point wall = cur;
  build_path( cur.row, cur.col );
  maze_[cur.row][cur.col] |= builder_bit_;
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
  build_path( wall.row, wall.col );
  maze_[wall.row][wall.col] |= builder_bit_;
  build_path( next.row, next.col );
  maze_[next.row][next.col] |= builder_bit_;
}

void Maze::join_squares_animated( const Point& cur, const Point& next )
{
  Point wall = cur;
  carve_path_walls_animated( cur.row, cur.col );
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
  carve_path_walls_animated( wall.row, wall.col );
  carve_path_walls_animated( next.row, next.col );
}

void Maze::build_wall( int row, int col )
{
  Wall_line wall = 0b0;
  if ( row - 1 >= 0 ) {
    wall |= north_wall_;
  }
  if ( row + 1 < maze_row_size_ ) {
    wall |= south_wall_;
  }
  if ( col - 1 >= 0 ) {
    wall |= west_wall_;
  }
  if ( col + 1 < maze_col_size_ ) {
    wall |= east_wall_;
  }
  maze_[row][col] |= wall;
  maze_[row][col] &= static_cast<Square>(~path_bit_);
}

void Maze::build_wall_carefully( int row, int col )
{
  Wall_line wall = 0b0;
  if ( row - 1 >= 0 && !( maze_[row - 1][col] & path_bit_ ) ) {
    wall |= north_wall_;
    maze_[row - 1][col] |= south_wall_;
  }
  if ( row + 1 < maze_row_size_ && !( maze_[row + 1][col] & path_bit_ ) ) {
    wall |= south_wall_;
    maze_[row + 1][col] |= north_wall_;
  }
  if ( col - 1 >= 0 && !( maze_[row][col - 1] & path_bit_ ) ) {
    wall |= west_wall_;
    maze_[row][col - 1] |= east_wall_;
  }
  if ( col + 1 < maze_col_size_ && !( maze_[row][col + 1] & path_bit_ ) ) {
    wall |= east_wall_;
    maze_[row][col + 1] |= west_wall_;
  }
  maze_[row][col] |= wall;
  maze_[row][col] &= static_cast<Square>(~path_bit_);
}

void Maze::build_path( int row, int col )
{
  if ( row - 1 >= 0 ) {
    maze_[row - 1][col] &= static_cast<Square>(~south_wall_);
  }
  if ( row + 1 < maze_row_size_ ) {
    maze_[row + 1][col] &= static_cast<Square>(~north_wall_);
  }
  if ( col - 1 >= 0 ) {
    maze_[row][col - 1] &= static_cast<Square>(~east_wall_);
  }
  if ( col + 1 < maze_col_size_ ) {
    maze_[row][col + 1] &= static_cast<Square>(~west_wall_);
  }
  maze_[row][col] |= path_bit_;
}

void Maze::build_path_animated( int row, int col )
{
  maze_[row][col] |= path_bit_;
  flush_cursor_maze_coordinate( row, col );
  std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  if ( row - 1 >= 0 && !( maze_[row - 1][col] & path_bit_ ) ) {
    maze_[row - 1][col] &= static_cast<Square>(~south_wall_);
    flush_cursor_maze_coordinate( row - 1, col );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  if ( row + 1 < maze_row_size_ && !( maze_[row + 1][col] & path_bit_ ) ) {
    maze_[row + 1][col] &= static_cast<Square>(~north_wall_);
    flush_cursor_maze_coordinate( row + 1, col );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  if ( col - 1 >= 0 && !( maze_[row][col - 1] & path_bit_ ) ) {
    maze_[row][col - 1] &= static_cast<Square>(~east_wall_);
    flush_cursor_maze_coordinate( row, col - 1 );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
  if ( col + 1 < maze_col_size_ && !( maze_[row][col + 1] & path_bit_ ) ) {
    maze_[row][col + 1] &= static_cast<Square>(~west_wall_);
    flush_cursor_maze_coordinate( row, col + 1 );
    std::this_thread::sleep_for( std::chrono::microseconds( builder_speed_units_ ) );
  }
}

/* * * * * * * * * * * * * * *      Cout Printing Functions      * * * * * * * * * * * * * * * * */

void Maze::clear_and_flush_grid() const
{
  clear_screen();
  for ( int row = 0; row < maze_row_size_; row++ ) {
    for ( int col = 0; col < maze_col_size_; col++ ) {
      print_square( row, col );
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

void Maze::clear_screen()
{
  std::cout << ansi_clear_screen_;
}

void Maze::flush_cursor_maze_coordinate( int row, int col ) const
{
  set_cursor_position( row, col );
  print_square( row, col );
  std::cout << std::flush;
}

void Maze::set_cursor_position( int row, int col )
{
  std::string cursor_pos = "\033[" + std::to_string( row + 1 ) + ";" + std::to_string( col + 1 ) + "f";
  std::cout << cursor_pos;
}

void Maze::print_maze_square( int row, int col ) const
{
  const Square& square = maze_[row][col];
  if ( !( square & Maze::path_bit_ ) ) {
    std::cout <<
      Maze::wall_styles_.at( static_cast<size_t>( wall_style_index_ ) ).at( square & wall_mask_ );
  } else if ( square & Maze::path_bit_ ) {
    std::cout << " ";
  } else {
    std::cerr << "Printed maze and a square was not categorized." << std::endl;
    abort();
  }
}

void Maze::print_square( int row, int col ) const
{
  const Square& square = maze_[row][col];
  if ( square & markers_mask_ ) {
    Backtrack_marker mark = ( maze_[row][col] & markers_mask_ ) >> marker_shift_; // NOLINT
    std::cout << backtracking_symbols_.at( mark );
  } else if ( !( square & path_bit_ ) ) {
    std::cout << wall_styles_.at( static_cast<size_t>( wall_style_index_ ) ).at( square & wall_mask_ );
  } else if ( square & path_bit_ ) {
    std::cout << " ";
  } else {
    std::cerr << "Printed maze and a square was not categorized." << std::endl;
    abort();
  }
}

void Maze::print_maze() const
{
  for ( int row = 0; row < maze_row_size_; row++ ) {
    for ( int col = 0; col < maze_col_size_; col++ ) {
      print_square( row, col );
    }
    std::cout << "\n";
  }
}

std::vector<Maze::Square>& Maze::operator[]( uint64_t index ) {
  return maze_.at( index );
}

const std::vector<Maze::Square>& Maze::operator[]( uint64_t index ) const {
  return maze_.at( index );
}

Maze::Animation_speed Maze::build_speed() const {
  return builder_speed_units_;
}

int Maze::row_size() const {
  return maze_row_size_;
}

int Maze::col_size() const {
  return maze_col_size_;
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
