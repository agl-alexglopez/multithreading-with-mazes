#include "maze_algorithms.hh"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

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
  std::shuffle( walls.begin(), walls.end(), std::mt19937( std::random_device {} () ) );
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

void generate_kruskal_maze( Maze& maze ) {
  std::vector<Maze::Point> walls = load_shuffled_walls( maze );
  std::unordered_map<Maze::Point, int> set_ids = tag_cells( maze );
  std::vector<int> indices( set_ids.size() );
  std::iota( begin( indices ), end( indices ), 0 );
  Disjoint_set sets( indices );
  for ( const Maze::Point& p : walls ) {
    if ( p.row % 2 == 0 ) {
      Maze::Point above_cell = { p.row - 1, p.col };
      Maze::Point below_cell = { p.row + 1, p.col };
      if ( sets.made_union( set_ids[above_cell], set_ids[below_cell] ) ) {
        maze.join_squares( above_cell, below_cell );
      }
    } else {
      Maze::Point left_cell = { p.row, p.col - 1 };
      Maze::Point right_cell = { p.row, p.col + 1 };
      if ( sets.made_union( set_ids[left_cell], set_ids[right_cell] ) ) {
        maze.join_squares( left_cell, right_cell );
      }
    }
  }
}
