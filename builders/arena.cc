#include "maze_algorithms.hh"

namespace Builder {

void generate_arena( Maze& maze )
{
  fill_maze_with_walls( maze );
  for ( int row = 2; row < maze.row_size() - 2; row++ ) {
    for ( int col = 2; col < maze.col_size() - 2; col++ ) {
      build_path( maze, { row, col } );
    }
  }
}

void animate_arena( Maze& maze, Builder_speed speed )
{
  fill_maze_with_walls( maze );
  clear_and_flush_grid( maze );
  Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  for ( int row = 2; row < maze.row_size() - 2; row++ ) {
    for ( int col = 2; col < maze.col_size() - 2; col++ ) {
      carve_path_walls_animated( maze, { row, col }, animation );
    }
  }
}

} // namespace Builder
