#include "maze.hh"
#include "maze_utilities.hh"
#include "speed.hh"

namespace Builder {

void add_x( Maze& maze )
{
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      add_positive_slope( maze, { row, col } );
      add_negative_slope( maze, { row, col } );
    }
  }
}

void add_x_animated( Maze& maze, Speed::Speed speed )
{
  const Speed::Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      add_positive_slope_animated( maze, { row, col }, animation );
      add_negative_slope_animated( maze, { row, col }, animation );
    }
  }
}

void add_cross( Maze& maze )
{
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      if ( ( row == maze.row_size() / 2 && col > 1 && col < maze.col_size() - 2 )
           || ( col == maze.col_size() / 2 && row > 1 && row < maze.row_size() - 2 ) ) {
        build_path( maze, { row, col } );
        if ( col + 1 < maze.col_size() - 2 ) {
          build_path( maze, { row, col + 1 } );
        }
      }
    }
  }
}

void add_cross_animated( Maze& maze, Speed::Speed speed )
{
  const Speed::Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      if ( ( row == maze.row_size() / 2 && col > 1 && col < maze.col_size() - 2 )
           || ( col == maze.col_size() / 2 && row > 1 && row < maze.row_size() - 2 ) ) {
        build_path_animated( maze, { row, col }, animation );
        if ( col + 1 < maze.col_size() - 2 ) {
          build_path_animated( maze, { row, col + 1 }, animation );
        }
      }
    }
  }
}

} // namespace Builder
