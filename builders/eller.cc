#include "disjoint_set.hh"
#include "maze_algorithms.hh"
#include "maze_utilities.hh"

#include <cstdint>
#include <numeric>
#include <random>
#include <unordered_map>
#include <vector>

namespace Builder {

namespace {

using Set_id = int;


} // namespace

void generate_eller_maze( Maze& maze )
{
  (void) maze;
}

void animate_eller_maze( Maze& maze, Builder_speed speed )
{
  fill_maze_with_walls_animated( maze );
  clear_and_flush_grid( maze );
  Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  uint64_t global_index = 0;
  std::vector<std::vector<Set_id>> cur_next_rows( 2, std::vector<Set_id>( maze.col_size(), { 0 } ) );
  std::iota( std::begin( cur_next_rows[0] ), std::end( cur_next_rows[0] ),  0 );
  Set_id unique_ids = maze.col_size();

  std::mt19937 gen( std::random_device{}() );
  std::uniform_int_distribution<int> coin( 0, 1 );
  for ( int row = 1; row < maze.row_size() - 2; row += 2 ) {
    uint64_t cur_row_set = global_index;
    uint64_t next_row_set = ( global_index + 1 ) % cur_next_rows.size();
    std::iota( std::begin( cur_next_rows[next_row_set] ), std::end( cur_next_rows[next_row_set] ), unique_ids );
    unique_ids += maze.col_size();

    std::unordered_map<Set_id, std::vector<Maze::Point>> sets_in_this_row {};

    for ( int col = 1; col < maze.col_size() - 1; col += 2 ) {
      Maze::Point next = { row, col + 2 };
      Set_id this_square_id = cur_next_rows[cur_row_set][col];
      sets_in_this_row[this_square_id].push_back( { row, col } );
      if ( is_square_within_perimeter_walls( maze, next ) &&
          this_square_id != cur_next_rows[cur_row_set][col + 2] && coin( gen ) ) {
        cur_next_rows[cur_row_set][col + 2] = this_square_id;
        join_squares_animated( maze, { row, col }, next, animation );
      }
    }

    for ( const auto& s : sets_in_this_row ) {
      std::uniform_int_distribution<uint64_t> random_dropdown( 0, s.second.size() - 1 );
      Maze::Point chosen = s.second[random_dropdown( gen )];
      cur_next_rows[next_row_set][chosen.col] = s.first;
      join_squares_animated( maze, chosen, { chosen.row + 2, chosen.col }, animation );
    }
    ++global_index %= cur_next_rows.size();
  }

  int final_row = maze.row_size() - 2;
  for ( int col = 1; col < maze.col_size() - 2; col += 2 ) {
    Maze::Point next = { final_row, col + 2 };
    Set_id this_square_id = cur_next_rows[global_index][col];
    if ( this_square_id != cur_next_rows[global_index][col + 2] ) {
      join_squares_animated( maze, { final_row, col }, next, animation );
    }
  }
}


} // namespace Builder
