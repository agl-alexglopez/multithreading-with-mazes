#include "maze.hh"
#include "maze_algorithms.hh"
#include "maze_utilities.hh"
#include "speed.hh"

#include <climits>
#include <functional>
#include <optional>
#include <queue>
#include <random>
#include <unordered_map>
#include <vector>

namespace Builder {

namespace {

struct Priority_cell
{
  Maze::Point cell;
  int priority;
  bool operator==( const Priority_cell& rhs ) const
  {
    return this->priority == rhs.priority && this->cell == rhs.cell;
  }
  bool operator!=( const Priority_cell& rhs ) const { return !( *this == rhs ); }
  bool operator<( const Priority_cell& rhs ) const { return this->priority < rhs.priority; }
  bool operator>( const Priority_cell& rhs ) const { return this->priority > rhs.priority; }
  bool operator<=( const Priority_cell& rhs ) const { return !( *this > rhs ); }
  bool operator>=( const Priority_cell& rhs ) const { return !( *this < rhs ); }
};

Maze::Point pick_random_odd_point( Maze& maze )
{
  std::uniform_int_distribution<int> rand_row( 1, ( maze.row_size() - 2 ) / 2 );
  std::uniform_int_distribution<int> rand_col( 1, ( maze.col_size() - 2 ) / 2 );
  std::mt19937 generator( std::random_device {}() );
  return { 2 * rand_row( generator ) + 1, 2 * rand_col( generator ) + 1 };
}

} // namespace

void generate_prim( Maze& maze )
{
  fill_maze_with_walls( maze );
  std::unordered_map<Maze::Point, int> cell_cost {};
  std::uniform_int_distribution<int> random_cost( 0, 100 );
  std::mt19937 generator( std::random_device {}() );
  const Maze::Point odd_point = pick_random_odd_point( maze );
  std::priority_queue<Priority_cell, std::vector<Priority_cell>, std::greater<>> cells;
  cells.push( { odd_point, cell_cost[odd_point] } );
  while ( !cells.empty() ) {
    const Maze::Point& cur = cells.top().cell;
    maze[cur.row][cur.col] |= Maze::builder_bit;
    std::optional<Maze::Point> min_neighbor = {};
    int min_weight = INT_MAX;
    for ( const Maze::Point& p : Maze::build_dirs ) {
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };
      if ( !can_build_new_square( maze, next ) ) {
        continue;
      }
      // We can generate random costs as we go efficiently thanks to try_emplace not constructing if present.
      const auto cost = cell_cost.try_emplace( next, random_cost( generator ) );
      const int weight = cost.first->second;
      if ( weight < min_weight ) {
        min_weight = weight;
        min_neighbor = next;
      }
    }
    if ( min_neighbor ) {
      join_squares( maze, cur, min_neighbor.value() );
      cells.push( { min_neighbor.value(), min_weight } );
    } else {
      cells.pop();
    }
  }
  clear_and_flush_grid( maze );
}

void animate_prim( Maze& maze, Speed::Speed speed )
{
  const Speed::Speed_unit animation_speed = builder_speeds.at( static_cast<int>( speed ) );
  fill_maze_with_walls_animated( maze );
  clear_and_flush_grid( maze );
  std::unordered_map<Maze::Point, int> cell_cost {};
  std::uniform_int_distribution<int> random_cost( 0, 100 );
  std::mt19937 generator( std::random_device {}() );
  const Maze::Point odd_point = pick_random_odd_point( maze );
  std::priority_queue<Priority_cell, std::vector<Priority_cell>, std::greater<>> cells;
  cells.push( { odd_point, cell_cost[odd_point] } );
  while ( !cells.empty() ) {
    const Maze::Point& cur = cells.top().cell;
    maze[cur.row][cur.col] |= Maze::builder_bit;
    std::optional<Maze::Point> min_neighbor = {};
    int min_weight = INT_MAX;
    for ( const Maze::Point& p : Maze::build_dirs ) {
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };
      if ( !can_build_new_square( maze, next ) ) {
        continue;
      }
      const auto cost = cell_cost.try_emplace( next, random_cost( generator ) );
      const int weight = cost.first->second;
      if ( weight < min_weight ) {
        min_weight = weight;
        min_neighbor = next;
      }
    }
    if ( min_neighbor ) {
      join_squares_animated( maze, cur, min_neighbor.value(), animation_speed );
      cells.push( { min_neighbor.value(), min_weight } );
    } else {
      cells.pop();
    }
  }
}

} // namespace Builder
