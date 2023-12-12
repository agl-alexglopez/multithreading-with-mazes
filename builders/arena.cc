export module labyrinth:arena;
import :maze;
import :speed;
import :build_utilities;

export namespace Arena {

void generate_maze( Maze::Maze& maze )
{
  Butil::fill_maze_with_walls( maze );
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      Butil::build_path( maze, { row, col } );
    }
  }
  Butil::clear_and_flush_grid( maze );
}

void animate_maze( Maze::Maze& maze, Speed::Speed speed )
{
  Butil::fill_maze_with_walls( maze );
  Butil::clear_and_flush_grid( maze );
  const Speed::Speed_unit animation = Butil::builder_speeds.at( static_cast<int>( speed ) );
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      Butil::carve_path_walls_animated( maze, { row, col }, animation );
    }
  }
}

} // namespace Arena
