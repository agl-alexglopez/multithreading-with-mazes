#include "maze_algorithms.hh"

namespace Builder {

void generate_arena( Maze& maze )
{
  fill_maze_with_walls( maze );
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      build_path( maze, { row, col } );
    }
  }
  clear_and_flush_grid( maze );
}

void animate_arena( Maze& maze, Builder_speed speed )
{
  fill_maze_with_walls( maze );
  clear_and_flush_grid( maze );
  const Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      carve_path_walls_animated( maze, { row, col }, animation );
    }
  }
}

} // namespace Builder
