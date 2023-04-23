#include "maze_algorithms.hh"

void generate_arena( Maze& maze ) {
  for ( int row = 2; row < maze.row_size() - 2; row++ ) {
    for ( int col = 2; col < maze.col_size() - 2; col++ ) {
      maze.build_path( row, col );
    }
  }
}

void animate_arena ( Maze& maze ) {
  for ( int row = 2; row < maze.row_size() - 2; row++ ) {
    for ( int col = 2; col < maze.col_size() - 2; col++ ) {
      maze.carve_path_walls_animated( row, col );
    }
  }
}
