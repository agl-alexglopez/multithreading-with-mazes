#include "disjoint_set.hh"
#include "maze_algorithms.hh"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Builder {

namespace {

std::vector<Maze::Point> load_shuffled_walls( Maze& maze )
{
  std::vector<Maze::Point> walls = {};
  // The walls between cells to the left and right. If row is odd look left and right.
  for ( int row = 1; row < maze.row_size() - 1; row += 2 ) {
    // Cells will be odd walls will be even within a col.
    for ( int col = 2; col < maze.col_size() - 1; col += 2 ) {
      walls.push_back( { row, col } );
    }
  }
  // The walls between cells above and below. If row is even look above and below.
  for ( int row = 2; row < maze.row_size() - 1; row += 2 ) {
    for ( int col = 1; col < maze.col_size() - 1; col += 2 ) {
      walls.push_back( { row, col } );
    }
  }
  std::shuffle( walls.begin(), walls.end(), std::mt19937( std::random_device {}() ) );
  return walls;
}

std::unordered_map<Maze::Point, int> tag_cells( Maze& maze )
{
  std::unordered_map<Maze::Point, int> set_ids = {};
  int id = 0;
  for ( int row = 1; row < maze.row_size() - 1; row += 2 ) {
    // Cells will be odd walls will be even within a col.
    for ( int col = 1; col < maze.col_size() - 1; col += 2 ) {
      set_ids[{ row, col }] = id;
      id++;
    }
  }
  return set_ids;
}

} // namespace

void generate_kruskal_maze( Maze& maze )
{
  fill_maze_with_walls( maze );
  const std::vector<Maze::Point> walls = load_shuffled_walls( maze );
  const std::unordered_map<Maze::Point, int> set_ids = tag_cells( maze );
  Disjoint_set sets( set_ids.size() );
  for ( const Maze::Point& p : walls ) {
    if ( p.row % 2 == 0 ) {
      const Maze::Point above_cell = { p.row - 1, p.col };
      const Maze::Point below_cell = { p.row + 1, p.col };
      if ( sets.made_union( set_ids.at( above_cell ), set_ids.at( below_cell ) ) ) {
        join_squares( maze, above_cell, below_cell );
      }
    } else {
      const Maze::Point left_cell = { p.row, p.col - 1 };
      const Maze::Point right_cell = { p.row, p.col + 1 };
      if ( sets.made_union( set_ids.at( left_cell ), set_ids.at( right_cell ) ) ) {
        join_squares( maze, left_cell, right_cell );
      }
    }
  }
  clear_and_flush_grid( maze );
}

void animate_kruskal_maze( Maze& maze, Builder_speed speed )
{
  const Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  fill_maze_with_walls_animated( maze );
  clear_and_flush_grid( maze );
  const std::vector<Maze::Point> walls = load_shuffled_walls( maze );
  const std::unordered_map<Maze::Point, int> set_ids = tag_cells( maze );
  Disjoint_set sets( set_ids.size() );

  for ( const Maze::Point& p : walls ) {
    if ( p.row % 2 == 0 ) {
      const Maze::Point above_cell = { p.row - 1, p.col };
      const Maze::Point below_cell = { p.row + 1, p.col };
      if ( sets.made_union( set_ids.at( above_cell ), set_ids.at( below_cell ) ) ) {
        join_squares_animated( maze, above_cell, below_cell, animation );
      }
    } else {
      const Maze::Point left_cell = { p.row, p.col - 1 };
      const Maze::Point right_cell = { p.row, p.col + 1 };
      if ( sets.made_union( set_ids.at( left_cell ), set_ids.at( right_cell ) ) ) {
        join_squares_animated( maze, left_cell, right_cell, animation );
      }
    }
  }
}

} // namespace Builder
