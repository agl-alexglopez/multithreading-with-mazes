#include "maze_utilities.hh"
#include <chrono>
#include <iostream>
#include <thread>

namespace Builder {

void add_positive_slope( Maze& maze, const Maze::Point& p )
{
  auto row_size = static_cast<float>( maze.row_size() ) - 2.0F;
  auto col_size = static_cast<float>( maze.col_size() ) - 2.0F;
  auto cur_row = static_cast<float>( p.row );
  // y = mx + b. We will get the negative slope. This line goes top left to bottom right.
  float slope = ( 2.0F - row_size ) / ( 2.0F - col_size );
  float b = 2.0F - ( 2.0F * slope );
  int on_line = static_cast<int>( ( cur_row - b ) / slope );
  if ( p.col == on_line && p.col < maze.col_size() - 2 && p.col > 1 ) {
    // An X is hard to notice and might miss breaking wall lines so make it wider.
    build_path( maze, p );
    if ( p.col + 1 < maze.col_size() - 2 ) {
      build_path( maze, { p.row, p.col + 1 } );
    }
    if ( p.col - 1 > 1 ) {
      build_path( maze, { p.row, p.col - 1 } );
    }
    if ( p.col + 2 < maze.col_size() - 2 ) {
      build_path( maze, { p.row, p.col + 2 } );
    }
    if ( p.col - 2 > 1 ) {
      build_path( maze, { p.row, p.col - 2 } );
    }
  }
}

void add_positive_slope_animated( Maze& maze, const Maze::Point& p, Speed_unit speed )
{
  auto row_size = static_cast<float>( maze.row_size() ) - 2.0F;
  auto col_size = static_cast<float>( maze.col_size() ) - 2.0F;
  auto cur_row = static_cast<float>( p.row );
  // y = mx + b. We will get the negative slope. This line goes top left to bottom right.
  float slope = ( 2.0F - row_size ) / ( 2.0F - col_size );
  float b = 2.0F - ( 2.0F * slope );
  int on_line = static_cast<int>( ( cur_row - b ) / slope );
  if ( p.col == on_line && p.col < maze.col_size() - 2 && p.col > 1 ) {
    // An X is hard to notice and might miss breaking wall lines so make it wider.
    build_path_animated( maze, p, speed );
    if ( p.col + 1 < maze.col_size() - 2 ) {
      build_path_animated( maze, { p.row, p.col + 1 }, speed );
    }
    if ( p.col - 1 > 1 ) {
      build_path_animated( maze, { p.row, p.col - 1 }, speed );
    }
    if ( p.col + 2 < maze.col_size() - 2 ) {
      build_path_animated( maze, { p.row, p.col + 2 }, speed );
    }
    if ( p.col - 2 > 1 ) {
      build_path_animated( maze, { p.row, p.col - 2 }, speed );
    }
  }
}

void add_negative_slope( Maze& maze, const Maze::Point& p )
{
  auto row_size = static_cast<float>( maze.row_size() ) - 2.0F;
  auto col_size = static_cast<float>( maze.col_size() ) - 2.0F;
  auto cur_row = static_cast<float>( p.row );
  float slope = ( 2.0F - row_size ) / ( col_size - 2.0F );
  float b = row_size - ( 2.0F * slope );
  int on_line = static_cast<int>( ( cur_row - b ) / slope );
  if ( p.col == on_line && p.col > 1 && p.col < maze.col_size() - 2 && p.row < maze.row_size() - 2 ) {
    build_path( maze, p );
    if ( p.col + 1 < maze.col_size() - 2 ) {
      build_path( maze, { p.row, p.col + 1 } );
    }
    if ( p.col - 1 > 1 ) {
      build_path( maze, { p.row, p.col - 1 } );
    }
    if ( p.col + 2 < maze.col_size() - 2 ) {
      build_path( maze, { p.row, p.col + 2 } );
    }
    if ( p.col - 2 > 1 ) {
      build_path( maze, { p.row, p.col - 2 } );
    }
  }
}

void add_negative_slope_animated( Maze& maze, const Maze::Point& p, Speed_unit speed )
{
  auto row_size = static_cast<float>( maze.row_size() ) - 2.0F;
  auto col_size = static_cast<float>( maze.col_size() ) - 2.0F;
  auto cur_row = static_cast<float>( p.row );
  float slope = ( 2.0F - row_size ) / ( col_size - 2.0F );
  float b = row_size - ( 2.0F * slope );
  int on_line = static_cast<int>( ( cur_row - b ) / slope );
  if ( p.col == on_line && p.col > 1 && p.col < maze.col_size() - 2 && p.row < maze.row_size() - 2 ) {
    build_path_animated( maze, p, speed );
    if ( p.col + 1 < maze.col_size() - 2 ) {
      build_path_animated( maze, { p.row, p.col + 1 }, speed );
    }
    if ( p.col - 1 > 1 ) {
      build_path_animated( maze, { p.row, p.col - 1 }, speed );
    }
    if ( p.col + 2 < maze.col_size() - 2 ) {
      build_path_animated( maze, { p.row, p.col + 2 }, speed );
    }
    if ( p.col - 2 > 1 ) {
      build_path_animated( maze, { p.row, p.col - 2 }, speed );
    }
  }
}

void add_cross( Maze& maze, const Maze::Point& p )
{
  if ( ( p.row == maze.row_size() / 2 && p.col > 1 && p.col < maze.col_size() - 2 )
       || ( p.col == maze.col_size() / 2 && p.row > 1 && p.row < maze.row_size() - 2 ) ) {
    build_path( maze, p );
    if ( p.col + 1 < maze.col_size() - 2 ) {
      build_path( maze, { p.row, p.col + 1 } );
    }
  }
}

void add_cross_animated( Maze& maze, const Maze::Point& p, Speed_unit speed )
{
  if ( ( p.row == maze.row_size() / 2 && p.col > 1 && p.col < maze.col_size() - 2 )
       || ( p.col == maze.col_size() / 2 && p.row > 1 && p.row < maze.row_size() - 2 ) ) {
    build_path_animated( maze, p, speed );
    if ( p.col + 1 < maze.col_size() - 2 ) {
      build_path_animated( maze, { p.row, p.col + 1 }, speed );
    }
  }
}

void add_x( Maze& maze, const Maze::Point& p )
{
  add_positive_slope( maze, p );
  add_negative_slope( maze, p );
}

void add_x_animated( Maze& maze, const Maze::Point& p, Speed_unit speed )
{
  add_positive_slope_animated( maze, p, speed );
  add_negative_slope_animated( maze, p, speed );
}

void build_wall_outline( Maze& maze )
{
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      if ( col == 0 || col == maze.col_size() - 1 || row == 0 || row == maze.row_size() - 1 ) {
        maze[row][col] |= Maze::builder_bit_;
        build_wall_carefully( maze, { row, col } );
      } else {
        build_path( maze, { row, col } );
      }
    }
  }
}

Maze::Point choose_arbitrary_point( const Maze& maze, Parity_point parity )
{
  int init = parity == Parity_point::even ? 2 : 1;
  for ( int row = init; row < maze.row_size() - 1; row += 2 ) {
    for ( int col = init; col < maze.col_size() - 1; col += 2 ) {
      Maze::Point cur = { row, col };
      if ( !( maze[cur.row][cur.col] & Maze::builder_bit_ ) ) {
        return cur;
      }
    }
  }
  return { 0, 0 };
}

bool can_build_new_square( const Maze& maze, const Maze::Point& next )
{
  return next.row > 0 && next.row < maze.row_size() - 1 && next.col > 0 && next.col < maze.col_size() - 1
         && !( maze[next.row][next.col] & Maze::builder_bit_ );
}

bool has_builder_bit( const Maze& maze, const Maze::Point& next )
{
  return maze[next.row][next.col] & Maze::builder_bit_;
}

bool is_square_within_perimeter_walls( const Maze& maze, const Maze::Point& next )
{
  return next.row < maze.row_size() - 1 && next.row > 0 && next.col < maze.col_size() - 1 && next.col > 0;
}

void build_wall_line( Maze& maze, const Maze::Point& p )
{
  Maze::Wall_line wall = 0b0;
  if ( p.row - 1 >= 0 && !( maze[p.row - 1][p.col] & Maze::path_bit_ ) ) {
    wall |= Maze::north_wall_;
    maze[p.row - 1][p.col] |= Maze::south_wall_;
  }
  if ( p.row + 1 < maze.row_size() && !( maze[p.row + 1][p.col] & Maze::path_bit_ ) ) {
    wall |= Maze::south_wall_;
    maze[p.row + 1][p.col] |= Maze::north_wall_;
  }
  if ( p.col - 1 >= 0 && !( maze[p.row][p.col - 1] & Maze::path_bit_ ) ) {
    wall |= Maze::west_wall_;
    maze[p.row][p.col - 1] |= Maze::east_wall_;
  }
  if ( p.col + 1 < maze.col_size() && !( maze[p.row][p.col + 1] & Maze::path_bit_ ) ) {
    wall |= Maze::east_wall_;
    maze[p.row][p.col + 1] |= Maze::west_wall_;
  }
  maze[p.row][p.col] |= wall;
  maze[p.row][p.col] |= Maze::builder_bit_;
  maze[p.row][p.col] &= static_cast<Maze::Square>( ~Maze::path_bit_ );
}

void build_wall_line_animated( Maze& maze, const Maze::Point& p, Speed_unit speed )
{
  Maze::Wall_line wall = 0b0;
  if ( p.row - 1 >= 0 && !( maze[p.row - 1][p.col] & Maze::path_bit_ ) ) {
    wall |= Maze::north_wall_;
    maze[p.row - 1][p.col] |= Maze::south_wall_;
    flush_cursor_maze_coordinate( maze, { p.row - 1, p.col } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  if ( p.row + 1 < maze.row_size() && !( maze[p.row + 1][p.col] & Maze::path_bit_ ) ) {
    wall |= Maze::south_wall_;
    maze[p.row + 1][p.col] |= Maze::north_wall_;
    flush_cursor_maze_coordinate( maze, { p.row + 1, p.col } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  if ( p.col - 1 >= 0 && !( maze[p.row][p.col - 1] & Maze::path_bit_ ) ) {
    wall |= Maze::west_wall_;
    maze[p.row][p.col - 1] |= Maze::east_wall_;
    flush_cursor_maze_coordinate( maze, { p.row, p.col - 1 } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  if ( p.col + 1 < maze.col_size() && !( maze[p.row][p.col + 1] & Maze::path_bit_ ) ) {
    wall |= Maze::east_wall_;
    maze[p.row][p.col + 1] |= Maze::west_wall_;
    flush_cursor_maze_coordinate( maze, { p.row, p.col + 1 } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  maze[p.row][p.col] |= wall;
  maze[p.row][p.col] |= Maze::builder_bit_;
  maze[p.row][p.col] &= static_cast<Maze::Square>( ~Maze::path_bit_ );
  flush_cursor_maze_coordinate( maze, p );
  std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
}

void clear_for_wall_adders( Maze& maze )
{
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      if ( col == 0 || col == maze.col_size() - 1 || row == 0 || row == maze.row_size() - 1 ) {
        maze[row][col] |= Maze::builder_bit_;
      } else {
        build_path( maze, { row, col } );
      }
    }
  }
}

void mark_origin( Maze& maze, const Maze::Point& walk, const Maze::Point& next )
{
  if ( next.row > walk.row ) {
    maze[next.row][next.col] |= Maze::from_north_;
  } else if ( next.row < walk.row ) {
    maze[next.row][next.col] |= Maze::from_south_;
  } else if ( next.col < walk.col ) {
    maze[next.row][next.col] |= Maze::from_east_;
  } else if ( next.col > walk.col ) {
    maze[next.row][next.col] |= Maze::from_west_;
  }
}

void mark_origin_animated( Maze& maze, const Maze::Point& walk, const Maze::Point& next, Speed_unit speed )
{
  if ( next.row > walk.row ) {
    maze[next.row][next.col] |= Maze::from_north_;
  } else if ( next.row < walk.row ) {
    maze[next.row][next.col] |= Maze::from_south_;
  } else if ( next.col < walk.col ) {
    maze[next.row][next.col] |= Maze::from_east_;
  } else if ( next.col > walk.col ) {
    maze[next.row][next.col] |= Maze::from_west_;
  }
  flush_cursor_maze_coordinate( maze, next );
  std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
}

/* * * * * * * * * Path Carvers * * * * * * * */

void fill_maze_with_walls( Maze& maze )
{
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      build_wall( maze, { row, col } );
    }
  }
}

void fill_maze_with_walls_animated( Maze& maze )
{
  clear_screen();
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      build_wall( maze, { row, col } );
    }
  }
}

void carve_path_walls( Maze& maze, const Maze::Point& p )
{
  maze[p.row][p.col] |= Maze::path_bit_;
  if ( p.row - 1 >= 0 ) {
    maze[p.row - 1][p.col] &= static_cast<Maze::Square>( ~Maze::south_wall_ );
  }
  if ( p.row + 1 < maze.row_size() ) {
    maze[p.row + 1][p.col] &= static_cast<Maze::Square>( ~Maze::north_wall_ );
  }
  if ( p.col - 1 >= 0 ) {
    maze[p.row][p.col - 1] &= static_cast<Maze::Square>( ~Maze::east_wall_ );
  }
  if ( p.col + 1 < maze.col_size() ) {
    maze[p.row][p.col + 1] &= static_cast<Maze::Square>( ~Maze::west_wall_ );
  }
  maze[p.row][p.col] |= Maze::builder_bit_;
}

// The animated version tries to save cursor movements if they are not necessary.
void carve_path_walls_animated( Maze& maze, const Maze::Point& p, Speed_unit speed )
{
  maze[p.row][p.col] |= Maze::path_bit_;
  flush_cursor_maze_coordinate( maze, p );
  std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  if ( p.row - 1 >= 0 && !( maze[p.row - 1][p.col] & Maze::path_bit_ ) ) {
    maze[p.row - 1][p.col] &= static_cast<Maze::Square>( ~Maze::south_wall_ );
    flush_cursor_maze_coordinate( maze, { p.row - 1, p.col } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  if ( p.row + 1 < maze.row_size() && !( maze[p.row + 1][p.col] & Maze::path_bit_ ) ) {
    maze[p.row + 1][p.col] &= static_cast<Maze::Square>( ~Maze::north_wall_ );
    flush_cursor_maze_coordinate( maze, { p.row + 1, p.col } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  if ( p.col - 1 >= 0 && !( maze[p.row][p.col - 1] & Maze::path_bit_ ) ) {
    maze[p.row][p.col - 1] &= static_cast<Maze::Square>( ~Maze::east_wall_ );
    flush_cursor_maze_coordinate( maze, { p.row, p.col - 1 } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  if ( p.col + 1 < maze.col_size() && !( maze[p.row][p.col + 1] & Maze::path_bit_ ) ) {
    maze[p.row][p.col + 1] &= static_cast<Maze::Square>( ~Maze::west_wall_ );
    flush_cursor_maze_coordinate( maze, { p.row, p.col + 1 } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  maze[p.row][p.col] |= Maze::builder_bit_;
}

void carve_path_markings( Maze& maze, const Maze::Point& cur, const Maze::Point& next )
{
  Maze::Point wall = cur;
  carve_path_walls( maze, cur );
  if ( next.row < cur.row ) {
    wall.row--;
    maze[next.row][next.col] |= Maze::from_south_;
  } else if ( next.row > cur.row ) {
    wall.row++;
    maze[next.row][next.col] |= Maze::from_north_;
  } else if ( next.col < cur.col ) {
    wall.col--;
    maze[next.row][next.col] |= Maze::from_east_;
  } else if ( next.col > cur.col ) {
    wall.col++;
    maze[next.row][next.col] |= Maze::from_west_;
  } else {
    std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
  }
  carve_path_walls( maze, next );
  carve_path_walls( maze, wall );
}

void carve_path_markings_animated( Maze& maze, const Maze::Point& cur, const Maze::Point& next, Speed_unit speed )
{
  Maze::Point wall = cur;
  carve_path_walls_animated( maze, cur, speed );
  if ( next.row < cur.row ) {
    wall.row--;
    maze[next.row][next.col] |= Maze::from_south_;
  } else if ( next.row > cur.row ) {
    wall.row++;
    maze[next.row][next.col] |= Maze::from_north_;
  } else if ( next.col < cur.col ) {
    wall.col--;
    maze[next.row][next.col] |= Maze::from_east_;
  } else if ( next.col > cur.col ) {
    wall.col++;
    maze[next.row][next.col] |= Maze::from_west_;
  } else {
    std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
  }
  carve_path_walls_animated( maze, next, speed );
  carve_path_walls_animated( maze, next, speed );
}

void join_squares( Maze& maze, const Maze::Point& cur, const Maze::Point& next )
{
  Maze::Point wall = cur;
  build_path( maze, cur );
  maze[cur.row][cur.col] |= Maze::builder_bit_;
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
  build_path( maze, wall );
  maze[wall.row][wall.col] |= Maze::builder_bit_;
  build_path( maze, next );
  maze[next.row][next.col] |= Maze::builder_bit_;
}

void join_squares_animated( Maze& maze, const Maze::Point& cur, const Maze::Point& next, Speed_unit speed )
{
  Maze::Point wall = cur;
  carve_path_walls_animated( maze, cur, speed );
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
  carve_path_walls_animated( maze, wall, speed );
  carve_path_walls_animated( maze, next, speed );
}

void build_wall( Maze& maze, const Maze::Point& p )
{
  Maze::Wall_line wall = 0b0;
  if ( p.row - 1 >= 0 ) {
    wall |= Maze::north_wall_;
  }
  if ( p.row + 1 < maze.row_size() ) {
    wall |= Maze::south_wall_;
  }
  if ( p.col - 1 >= 0 ) {
    wall |= Maze::west_wall_;
  }
  if ( p.col + 1 < maze.col_size() ) {
    wall |= Maze::east_wall_;
  }
  maze[p.row][p.col] |= wall;
  maze[p.row][p.col] &= static_cast<Maze::Square>( ~Maze::path_bit_ );
}

void build_wall_carefully( Maze& maze, const Maze::Point& p )
{
  Maze::Wall_line wall = 0b0;
  if ( p.row - 1 >= 0 && !( maze[p.row - 1][p.col] & Maze::path_bit_ ) ) {
    wall |= Maze::north_wall_;
    maze[p.row - 1][p.col] |= Maze::south_wall_;
  }
  if ( p.row + 1 < maze.row_size() && !( maze[p.row + 1][p.col] & Maze::path_bit_ ) ) {
    wall |= Maze::south_wall_;
    maze[p.row + 1][p.col] |= Maze::north_wall_;
  }
  if ( p.col - 1 >= 0 && !( maze[p.row][p.col - 1] & Maze::path_bit_ ) ) {
    wall |= Maze::west_wall_;
    maze[p.row][p.col - 1] |= Maze::east_wall_;
  }
  if ( p.col + 1 < maze.col_size() && !( maze[p.row][p.col + 1] & Maze::path_bit_ ) ) {
    wall |= Maze::east_wall_;
    maze[p.row][p.col + 1] |= Maze::west_wall_;
  }
  maze[p.row][p.col] |= wall;
  maze[p.row][p.col] &= static_cast<Maze::Square>( ~Maze::path_bit_ );
}

void build_path( Maze& maze, const Maze::Point& p )
{
  if ( p.row - 1 >= 0 ) {
    maze[p.row - 1][p.col] &= static_cast<Maze::Square>( ~Maze::south_wall_ );
  }
  if ( p.row + 1 < maze.row_size() ) {
    maze[p.row + 1][p.col] &= static_cast<Maze::Square>( ~Maze::north_wall_ );
  }
  if ( p.col - 1 >= 0 ) {
    maze[p.row][p.col - 1] &= static_cast<Maze::Square>( ~Maze::east_wall_ );
  }
  if ( p.col + 1 < maze.col_size() ) {
    maze[p.row][p.col + 1] &= static_cast<Maze::Square>( ~Maze::west_wall_ );
  }
  maze[p.row][p.col] |= Maze::path_bit_;
}

void build_path_animated( Maze& maze, const Maze::Point& p, Speed_unit speed )
{
  maze[p.row][p.col] |= Maze::path_bit_;
  flush_cursor_maze_coordinate( maze, p );
  std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  if ( p.row - 1 >= 0 && !( maze[p.row - 1][p.col] & Maze::path_bit_ ) ) {
    maze[p.row - 1][p.col] &= static_cast<Maze::Square>( ~Maze::south_wall_ );
    flush_cursor_maze_coordinate( maze, { p.row - 1, p.col } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  if ( p.row + 1 < maze.row_size() && !( maze[p.row + 1][p.col] & Maze::path_bit_ ) ) {
    maze[p.row + 1][p.col] &= static_cast<Maze::Square>( ~Maze::north_wall_ );
    flush_cursor_maze_coordinate( maze, { p.row + 1, p.col } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  if ( p.col - 1 >= 0 && !( maze[p.row][p.col - 1] & Maze::path_bit_ ) ) {
    maze[p.row][p.col - 1] &= static_cast<Maze::Square>( ~Maze::east_wall_ );
    flush_cursor_maze_coordinate( maze, { p.row, p.col - 1 } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
  if ( p.col + 1 < maze.col_size() && !( maze[p.row][p.col + 1] & Maze::path_bit_ ) ) {
    maze[p.row][p.col + 1] &= static_cast<Maze::Square>( ~Maze::west_wall_ );
    flush_cursor_maze_coordinate( maze, { p.row, p.col + 1 } );
    std::this_thread::sleep_for( std::chrono::microseconds( speed ) );
  }
}

/* * * * * * * * * * * * * * *      Cout Printing Functions      * * * * * * * * * * * * * * * * */

void clear_and_flush_grid( const Maze& maze )
{
  clear_screen();
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      print_square( maze, { row, col } );
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

void clear_screen()
{
  std::cout << Maze::ansi_clear_screen_;
}

void flush_cursor_maze_coordinate( const Maze& maze, const Maze::Point& p )
{
  set_cursor_position( p );
  print_square( maze, p );
  std::cout << std::flush;
}

void set_cursor_position( const Maze::Point& p )
{
  std::string cursor_pos = "\033[" + std::to_string( p.row + 1 ) + ";" + std::to_string( p.col + 1 ) + "f";
  std::cout << cursor_pos;
}

void print_maze_square( const Maze& maze, const Maze::Point& p )
{
  const Maze::Square& square = maze[p.row][p.col];
  if ( !( square & Maze::path_bit_ ) ) {
    std::cout << maze.wall_style().at( square & Maze::wall_mask_ );
  } else if ( square & Maze::path_bit_ ) {
    std::cout << " ";
  } else {
    std::cerr << "Printed maze and a square was not categorized." << std::endl;
    abort();
  }
}

void print_square( const Maze& maze, const Maze::Point& p )
{
  const Maze::Square& square = maze[p.row][p.col];
  if ( square & Maze::markers_mask_ ) {
    Maze::Backtrack_marker mark = ( maze[p.row][p.col] & Maze::markers_mask_ ) >> Maze::marker_shift_; // NOLINT
    std::cout << Maze::backtracking_symbols_.at( mark );
  } else if ( !( square & Maze::path_bit_ ) ) {
    std::cout << maze.wall_style().at( square & Maze::wall_mask_ );
  } else if ( square & Maze::path_bit_ ) {
    std::cout << " ";
  } else {
    std::cerr << "Printed maze and a square was not categorized." << std::endl;
    abort();
  }
}

void print_maze( const Maze& maze )
{
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      print_square( maze, { row, col } );
    }
    std::cout << "\n";
  }
}

} // namespace Builder
