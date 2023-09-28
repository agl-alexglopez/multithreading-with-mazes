#include "maze_algorithms.hh"

#include <cstdint>
#include <numeric>
#include <random>
#include <span>
#include <unordered_map>
#include <vector>

namespace Builder {

namespace {

constexpr uint8_t window_height = 2;
constexpr int horizontal_bias = 2;

using Set_id = int;

struct Sliding_set_window
{
  uint64_t curr_row { 0 };
  uint64_t width { 0 };
  // This is a flat 2xmaze.col_size() vector of setids that we will overwrite with std::spans as we slide.
  std::vector<Set_id> sets;
  explicit Sliding_set_window( const Maze& maze ) : width( maze.col_size() ), sets( window_height * width, { 0 } )
  {}
};

struct Id_merge_request
{
  Set_id winning_id;
  Set_id losing_id;
};

void merge_sets( Sliding_set_window& sets, const Id_merge_request& merge, int col_in_row )
{
  for ( uint64_t set_elem = col_in_row + 2; set_elem < sets.width - 1; set_elem += 2 ) {
    if ( sets.sets[sets.curr_row * sets.width + set_elem] == merge.losing_id ) {
      sets.sets[sets.curr_row * sets.width + set_elem] = merge.winning_id;
    }
  }
  for ( int set_elem = col_in_row - 2; set_elem > 0; set_elem -= 2 ) {
    if ( sets.sets[sets.curr_row * sets.width + set_elem] == merge.losing_id ) {
      sets.sets[sets.curr_row * sets.width + set_elem] = merge.winning_id;
    }
  }
}

void complete_final_row( Maze& maze, Sliding_set_window& window )
{
  const int final_row = maze.row_size() - 2;
  for ( int col = 1; col < maze.col_size() - 2; col += 2 ) {
    const Maze::Point next = { final_row, col + 2 };
    const Set_id this_square_id = window.sets[window.curr_row * window.width + col];
    if ( this_square_id != window.sets[window.curr_row * window.width + col + 2] ) {
      join_squares( maze, { final_row, col }, next );
      const Set_id other_set_id = window.sets[window.curr_row * window.width + next.col];
      for ( int set_elem = next.col; set_elem < maze.col_size() - 1; set_elem += 2 ) {
        if ( window.sets[window.curr_row * window.width + set_elem] == other_set_id ) {
          window.sets[window.curr_row * window.width + set_elem] = this_square_id;
        }
      }
    }
  }
}

void complete_final_row_animated( Maze& maze, Sliding_set_window& window, Speed_unit animation )
{
  const int final_row = maze.row_size() - 2;
  for ( int col = 1; col < maze.col_size() - 2; col += 2 ) {
    const Maze::Point next = { final_row, col + 2 };
    const Set_id this_square_id = window.sets[window.curr_row * window.width + col];
    if ( this_square_id != window.sets[window.curr_row * window.width + col + 2] ) {
      join_squares_animated( maze, { final_row, col }, next, animation );
      const Set_id other_set_id = window.sets[window.curr_row * window.width + next.col];
      for ( int set_elem = next.col; set_elem < maze.col_size() - 1; set_elem += 2 ) {
        if ( window.sets[window.curr_row * window.width + set_elem] == other_set_id ) {
          window.sets[window.curr_row * window.width + set_elem] = this_square_id;
        }
      }
    }
  }
}

} // namespace

/* There are two fun details about this implementation: the auxillary memory requirement is a constant determined
 * by the width of a row and the randomness is thorough when determining how many squares per set should drop below.
 * The downside is the way I am doing it now is somewhat slow. I want to keep the memory footprint low and a good
 * randomized technique to choose dropping squares. Find a better strategy.
 */

void generate_eller_maze( Maze& maze )
{
  fill_maze_with_walls( maze );
  std::mt19937 gen( std::random_device {}() );
  std::uniform_int_distribution<int> coin( 0, horizontal_bias );

  Sliding_set_window window( maze );
  std::span<Set_id> init_sets( window.sets.data(), window.width );
  std::iota( std::begin( init_sets ), std::end( init_sets ), 0 );
  Set_id unique_ids = maze.col_size();
  std::unordered_map<Set_id, std::vector<Maze::Point>> sets_in_this_row {};
  for ( int row = 1; row < maze.row_size() - 2; row += 2 ) {
    const uint64_t next_row = ( window.curr_row + 1 ) % window_height;
    std::span<Set_id> fill_sets( &window.sets[next_row * window.width], window.width );
    std::iota( std::begin( fill_sets ), std::end( fill_sets ), unique_ids );
    unique_ids += maze.col_size();

    for ( int col = 1; col < maze.col_size() - 1; col += 2 ) {
      const Maze::Point next = { row, col + 2 };
      const Set_id this_square_id = window.sets[window.curr_row * window.width + col];
      if ( is_square_within_perimeter_walls( maze, next )
           && this_square_id != window.sets[window.curr_row * window.width + next.col] && coin( gen ) ) {
        join_squares( maze, { row, col }, next );
        merge_sets( window, { this_square_id, window.sets[window.curr_row * window.width + next.col] }, col );
      }
    }

    for ( int col = 1; col < maze.col_size() - 1; col += 2 ) {
      const Set_id this_square_id = window.sets[window.curr_row * window.width + col];
      sets_in_this_row[this_square_id].push_back( { row, col } );
    }

    for ( const auto& s : sets_in_this_row ) {
      std::uniform_int_distribution<uint64_t> num_drops( 1, s.second.size() );
      const uint64_t drops = num_drops( gen );
      for ( uint64_t drop = 0; drop < drops; drop++ ) {
        std::uniform_int_distribution<uint64_t> rand_drop( 0, s.second.size() - 1 );
        const Maze::Point chosen = s.second[rand_drop( gen )];
        // We already linked this up and rondomness dropped us here again. More important for animated version.
        if ( !( maze[chosen.row + 2][chosen.col] & Maze::builder_bit_ ) ) {
          window.sets[next_row * window.width + chosen.col] = s.first;
          join_squares( maze, chosen, { chosen.row + 2, chosen.col } );
        }
      }
    }
    window.curr_row = next_row;
    sets_in_this_row.clear();
  }
  complete_final_row( maze, window );
  clear_and_flush_grid( maze );
}

void animate_eller_maze( Maze& maze, Builder_speed speed )
{
  fill_maze_with_walls_animated( maze );
  clear_and_flush_grid( maze );
  const Speed_unit animation = builder_speeds_.at( static_cast<int>( speed ) );
  std::mt19937 gen( std::random_device {}() );
  std::uniform_int_distribution<int> coin( 0, horizontal_bias );

  Sliding_set_window window( maze );
  std::span<Set_id> init_sets( window.sets.data(), window.width );
  std::iota( std::begin( init_sets ), std::end( init_sets ), 0 );
  Set_id unique_ids = maze.col_size();
  std::unordered_map<Set_id, std::vector<Maze::Point>> sets_in_this_row {};
  for ( int row = 1; row < maze.row_size() - 2; row += 2 ) {
    const uint64_t next_row = ( window.curr_row + 1 ) % window_height;
    std::span<Set_id> fill_sets( &window.sets[next_row * window.width], window.width );
    std::iota( std::begin( fill_sets ), std::end( fill_sets ), unique_ids );
    unique_ids += maze.col_size();

    for ( int col = 1; col < maze.col_size() - 1; col += 2 ) {
      const Maze::Point next = { row, col + 2 };
      const Set_id this_square_id = window.sets[window.curr_row * window.width + col];
      if ( is_square_within_perimeter_walls( maze, next )
           && this_square_id != window.sets[window.curr_row * window.width + next.col] && coin( gen ) ) {
        join_squares_animated( maze, { row, col }, next, animation );
        merge_sets( window, { this_square_id, window.sets[window.curr_row * window.width + next.col] }, col );
      }
    }

    for ( int col = 1; col < maze.col_size() - 1; col += 2 ) {
      const Set_id this_square_id = window.sets[window.curr_row * window.width + col];
      sets_in_this_row[this_square_id].push_back( { row, col } );
    }

    for ( const auto& s : sets_in_this_row ) {
      std::uniform_int_distribution<uint64_t> num_drops( 1, s.second.size() );
      const uint64_t drops = num_drops( gen );
      for ( uint64_t drop = 0; drop < drops; drop++ ) {
        std::uniform_int_distribution<uint64_t> rand_drop( 0, s.second.size() - 1 );
        const Maze::Point chosen = s.second[rand_drop( gen )];
        // We already linked this up and rondomness dropped us here again. Save pointless cursor movements.
        if ( !( maze[chosen.row + 2][chosen.col] & Maze::builder_bit_ ) ) {
          window.sets[next_row * window.width + chosen.col] = s.first;
          join_squares_animated( maze, chosen, { chosen.row + 2, chosen.col }, animation );
        }
      }
    }
    window.curr_row = next_row;
    sets_in_this_row.clear();
  }
  complete_final_row_animated( maze, window, animation );
}

} // namespace Builder
