export module labyrinth:arena;
import :maze;
import :speed;
import :maze_utilities;

export namespace Arena {

void generate_arena( Maze::Maze& maze )
{
  Maze_utilities::fill_maze_with_walls( maze );
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      Maze_utilities::build_path( maze, { row, col } );
    }
  }
  Maze_utilities::clear_and_flush_grid( maze );
}

void animate_arena( Maze::Maze& maze, Speed::Speed speed )
{
  Maze_utilities::fill_maze_with_walls( maze );
  Maze_utilities::clear_and_flush_grid( maze );
  const Speed::Speed_unit animation = Maze_utilities::builder_speeds.at( static_cast<int>( speed ) );
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      Maze_utilities::carve_path_walls_animated( maze, { row, col }, animation );
    }
  }
}

} // namespace Arena
