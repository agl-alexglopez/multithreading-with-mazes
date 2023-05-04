#include "disjoint_set.hh"
#include "maze_algorithms.hh"

#include <climits>
#include <queue>
#include <random>
#include <unordered_map>

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

std::unordered_map<Maze::Point, int> randomize_cell_costs( Maze& maze )
{
  std::unordered_map<Maze::Point, int> cell_cost = {};
  std::uniform_int_distribution<int> random_cost( 0, 100 );
  std::mt19937 generator( std::random_device {}() );
  for ( int row = 1; row < maze.row_size(); row += 2 ) {
    for ( int col = 1; col < maze.col_size(); col += 2 ) {
      cell_cost[{ row, col }] = random_cost( generator );
    }
  }
  return cell_cost;
}

} // namespace

void generate_prim_maze( Maze& maze )
{
  fill_maze_with_walls( maze );
  std::unordered_map<Maze::Point, int> cell_cost = randomize_cell_costs( maze );
  const Maze::Point odd_point = pick_random_odd_point( maze );
  std::priority_queue<Priority_cell, std::vector<Priority_cell>, std::greater<>> cells;
  cells.push( { odd_point, cell_cost[odd_point] } );
  while ( !cells.empty() ) {
    const Maze::Point& cur = cells.top().cell;
    maze[cur.row][cur.col] |= Maze::builder_bit_;
    Maze::Point min_neighbor = {};
    int min_weight = INT_MAX;
    for ( const Maze::Point& p : Maze::generate_directions_ ) {
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };
      if ( can_build_new_square( maze, next ) ) {
        const int weight = cell_cost[next];
        if ( weight < min_weight ) {
          min_weight = weight;
          min_neighbor = next;
        }
      }
    }
    if ( min_neighbor.row ) {
      join_squares( maze, cur, min_neighbor );
      cells.push( { min_neighbor, min_weight } );
    } else {
      cells.pop();
    }
  }
  clear_and_flush_grid( maze );
}

void animate_prim_maze( Maze& maze, Builder_speed speed )
{
  const Speed_unit animation_speed = builder_speeds_.at( static_cast<int>( speed ) );
  fill_maze_with_walls_animated( maze );
  clear_and_flush_grid( maze );
  std::unordered_map<Maze::Point, int> cell_cost = randomize_cell_costs( maze );
  const Maze::Point odd_point = pick_random_odd_point( maze );
  std::priority_queue<Priority_cell, std::vector<Priority_cell>, std::greater<>> cells;
  cells.push( { odd_point, cell_cost[odd_point] } );
  while ( !cells.empty() ) {
    const Maze::Point& cur = cells.top().cell;
    maze[cur.row][cur.col] |= Maze::builder_bit_;
    Maze::Point min_neighbor = {};
    int min_weight = INT_MAX;
    for ( const Maze::Point& p : Maze::generate_directions_ ) {
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };
      if ( can_build_new_square( maze, next ) ) {
        const int weight = cell_cost[next];
        if ( weight < min_weight ) {
          min_weight = weight;
          min_neighbor = next;
        }
      }
    }
    if ( min_neighbor.row ) {
      join_squares_animated( maze, cur, min_neighbor, animation_speed );
      cells.push( { min_neighbor, min_weight } );
    } else {
      cells.pop();
    }
  }
}

} // namespace Builder
