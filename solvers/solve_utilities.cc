module;
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>
module labyrinth:solve_utilities;
import :maze;
import :my_queue;
import :speed;
import :printers;

namespace Sutil {

/* The maze builder is responsible for zeroing out the direction bits as part of the
 * building process. When solving the maze we adjust how we use the middle bits.
 *
 * wall structure----------------------||||
 * ------------------------------------||||
 * 0 thread paint--------------------| ||||
 * 1 thread paint-------------------|| ||||
 * 2 thread paint------------------||| ||||
 * 3 thread paint-----------------|||| ||||
 * -------------------------------|||| ||||
 * 0 thread cache---------------| |||| ||||
 * 1 thread cache--------------|| |||| ||||
 * 2 thread cache-------------||| |||| ||||
 * 3 thread cache------------|||| |||| ||||
 * --------------------------|||| |||| ||||
 * maze build bit----------| |||| |||| ||||
 * maze paths bit---------|| |||| |||| ||||
 * maze start bit--------||| |||| |||| ||||
 * maze goals bit-------|||| |||| |||| ||||
 *                    0b0000 0000 0000 0000
 */
using Thread_bit = Maze::Square;
using Thread_paint = Maze::Square;
using Thread_cache = Maze::Square;

struct Thread_id
{
  uint16_t index;
  Thread_bit bit;
};

enum class Maze_game
{
  hunt,
  gather,
  corners,
};

/* * * * * * * * * * * * *     Helpful Read-Only Data Available to All Solvers   * * * * * * * * * * * * * * * * */

constexpr int num_threads = 4;
constexpr Thread_bit no_winner { UINT16_MAX };
constexpr Thread_bit start_bit { 0b0100'0000'0000'0000 };
constexpr Thread_bit finish_bit { 0b1000'0000'0000'0000 };
constexpr Thread_bit zero_bit { 0b0001 };
constexpr Thread_bit one_bit { 0b0010 };
constexpr Thread_bit two_bit { 0b0100 };
constexpr Thread_bit three_bit { 0b1000 };
constexpr std::array<Thread_bit, 4> thread_bits
  = { Thread_bit { 0b0001 }, Thread_bit { 0b0010 }, Thread_bit { 0b0100 }, Thread_bit { 0b1000 } };
constexpr int initial_path_len = 1024;

constexpr uint16_t thread_paint_shift { 4 };
constexpr Thread_paint thread_paint_mask { 0b1111'0000 };
constexpr int num_gather_finishes { 4 };

constexpr Thread_cache clear_cache { 0b0001'1111'1111'0000 };
constexpr Thread_cache cache_mask { 0b1111'0000'0000 };
constexpr Thread_cache zero_seen { 0b0001'0000'0000 };
constexpr Thread_cache one_seen { 0b0010'0000'0000 };
constexpr Thread_cache two_seen { 0b0100'0000'0000 };
constexpr Thread_cache three_seen { 0b1000'0000'0000 };
constexpr uint16_t thread_cache_shift { 8 };

constexpr std::string_view ansi_red = "\033[38;5;1m█\033[0m";
constexpr std::string_view ansi_grn = "\033[38;5;2m█\033[0m";
constexpr std::string_view ansi_yel = "\033[38;5;3m█\033[0m";
constexpr std::string_view ansi_blu = "\033[38;5;4m█\033[0m";
constexpr std::string_view ansi_prp = "\033[38;5;183m█\033[0m";
constexpr std::string_view ansi_mag = "\033[38;5;201m█\033[0m";
constexpr std::string_view ansi_cyn = "\033[38;5;87m█\033[0m";
constexpr std::string_view ansi_wit = "\033[38;5;231m█\033[0m";
constexpr std::string_view ansi_prp_red = "\033[38;5;204m█\033[0m";
constexpr std::string_view ansi_blu_mag = "\033[38;5;105m█\033[0m";
constexpr std::string_view ansi_red_grn_blu = "\033[38;5;121m█\033[0m";
constexpr std::string_view ansi_grn_prp = "\033[38;5;106m█\033[0m";
constexpr std::string_view ansi_grn_blu_prp = "\033[38;5;60m█\033[0m";
constexpr std::string_view ansi_red_grn_prp = "\033[38;5;105m█\033[0m";
constexpr std::string_view ansi_red_blu_prp = "\033[38;5;89m█\033[0m";
constexpr std::string_view ansi_dark_blu_mag = "\033[38;5;57m█\033[0m";
constexpr std::string_view ansi_bold = "\033[1m";
constexpr std::string_view ansi_nil = "\033[0m";
constexpr std::string_view ansi_no_solution = "\033[38;5;15m\033[48;255;0;0m╳ no thread won..\033[0m";
constexpr std::string_view ansi_finish = "\033[1m\033[38;5;87mF\033[0m";
constexpr std::string_view ansi_start = "\033[1m\033[38;5;87mS\033[0m";
constexpr Thread_paint all_threads_failed_index { 0 };
constexpr std::array<std::string_view, 16> thread_colors = {
  ansi_no_solution,
  // Threads Overlaps. The zero thread is the zero index bit with a value of 1.
  // 0b0001   0b0010     0b0011     0b0100     0b0101     0b0110        0b0111
  ansi_red,
  ansi_grn,
  ansi_yel,
  ansi_blu,
  ansi_mag,
  ansi_cyn,
  ansi_red_grn_blu,
  // 0b1000    0b1001          0b1010           0b1011            0b1100
  ansi_prp,
  ansi_prp_red,
  ansi_grn_prp,
  ansi_red_grn_prp,
  ansi_dark_blu_mag,
  // 0b1101              0b1110          0b1111
  ansi_red_blu_prp,
  ansi_grn_blu_prp,
  ansi_wit,
};
// north, east, south, west
constexpr std::array<Maze::Point, 4> dirs = { { { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } } };
// north, north-east, east, south-east, south, south-west, west, north-west
constexpr std::array<Maze::Point, 7> all_dirs
  = { { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 } } };
constexpr int overlap_key_and_message_height = 9;
constexpr std::array<Speed::Speed_unit, 8> solver_speeds = { 0, 20000, 10000, 5000, 2000, 1000, 500, 250 };

struct Dfs_monitor
{
  std::mutex monitor {};
  std::optional<Speed::Speed_unit> speed {};
  std::vector<Maze::Point> starts {};
  Maze::Square winning_index { no_winner };
  std::vector<std::vector<Maze::Point>> thread_paths;
  Dfs_monitor() : thread_paths { num_threads, std::vector<Maze::Point> {} }
  {
    for ( std::vector<Maze::Point>& path : thread_paths ) {
      path.reserve( initial_path_len );
    }
  }
};

struct Bfs_monitor
{
  std::mutex monitor {};
  std::optional<Speed::Speed_unit> speed {};
  std::vector<std::unordered_map<Maze::Point, Maze::Point>> thread_maps;
  std::vector<My_queue<Maze::Point>> thread_queues;
  std::vector<Maze::Point> starts {};
  Maze::Square winning_index { no_winner };
  std::vector<std::vector<Maze::Point>> thread_paths;
  Bfs_monitor()
    : thread_maps { num_threads }
    , thread_queues { num_threads }
    , thread_paths { num_threads, std::vector<Maze::Point> {} }
  {
    for ( std::vector<Maze::Point>& path : thread_paths ) {
      path.reserve( initial_path_len );
    }
    for ( My_queue<Maze::Point>& q : thread_queues ) {
      q.reserve( initial_path_len );
    }
  }
};

bool is_valid_start_or_finish( const Maze::Maze& maze, const Maze::Point& choice )
{
  return choice.row > 0 && choice.row < maze.row_size() - 1 && choice.col > 0 && choice.col < maze.col_size() - 1
         && ( maze[choice.row][choice.col] & Maze::path_bit ) && !( maze[choice.row][choice.col] & finish_bit )
         && !( maze[choice.row][choice.col] & start_bit );
}

void print_point( const Maze::Maze& maze, const Maze::Point& point )
{
  const Maze::Square& square = maze[point.row][point.col];
  if ( square & finish_bit ) {
    std::cout << ansi_finish;
    return;
  }
  if ( square & start_bit ) {
    std::cout << ansi_start;
    return;
  }
  if ( square & thread_paint_mask ) {
    const Thread_paint thread_color = ( square & thread_paint_mask ) >> thread_paint_shift;
    std::cout << thread_colors.at( thread_color.load() );
    return;
  }
  if ( !( square & Maze::path_bit ) ) {
    std::cout << maze.wall_style()[( square & Maze::wall_mask ).load()];
    return;
  }
  if ( square & Maze::path_bit ) {
    std::cout << " ";
    return;
  }
  std::cerr << "Printed maze and a square was not categorized."
            << "\n";
  std::abort();
}

void print_maze( const Maze::Maze& maze )
{
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      print_point( maze, { row, col } );
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

void flush_cursor_path_coordinate( const Maze::Maze& maze, const Maze::Point& point )
{
  Printer::set_cursor_position( point );
  print_point( maze, point );
  std::cout << std::flush;
}

void clear_and_flush_paths( const Maze::Maze& maze )
{
  Printer::clear_screen();
  print_maze( maze );
}

Maze::Point find_nearest_square( const Maze::Maze& maze, const Maze::Point& choice )
{
  // Fanning out from a starting point should work on any medium to large maze.
  for ( const Maze::Point& p : all_dirs ) {
    const Maze::Point next = { choice.row + p.row, choice.col + p.col };
    if ( is_valid_start_or_finish( maze, next ) ) {
      return next;
    }
  }
  // Getting desperate here. We should only need this for very small mazes.
  for ( int row = 1; row < maze.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze.col_size() - 1; col++ ) {
      if ( is_valid_start_or_finish( maze, { row, col } ) ) {
        return { row, col };
      }
    }
  }
  std::cerr << "Could not place a point. Bad point = "
            << "{" << choice.row << "," << choice.col << "}"
            << "\n";
  print_maze( maze );
  std::abort();
}

std::vector<Maze::Point> set_corner_starts( const Maze::Maze& maze )
{
  Maze::Point point1 = { 1, 1 };
  if ( !( maze[point1.row][point1.col] & Maze::path_bit ) ) {
    point1 = find_nearest_square( maze, point1 );
  }
  Maze::Point point2 = { 1, maze.col_size() - 2 };
  if ( !( maze[point2.row][point2.col] & Maze::path_bit ) ) {
    point2 = find_nearest_square( maze, point2 );
  }
  Maze::Point point3 = { maze.row_size() - 2, 1 };
  if ( !( maze[point3.row][point3.col] & Maze::path_bit ) ) {
    point3 = find_nearest_square( maze, point3 );
  }
  Maze::Point point4 = { maze.row_size() - 2, maze.col_size() - 2 };
  if ( !( maze[point4.row][point4.col] & Maze::path_bit ) ) {
    point4 = find_nearest_square( maze, point4 );
  }
  return { point1, point2, point3, point4 };
}

Maze::Point pick_random_point( const Maze::Maze& maze )
{
  std::mt19937 generator( std::random_device {}() );
  std::uniform_int_distribution<int> row_random( 1, maze.row_size() - 2 );
  std::uniform_int_distribution<int> col_random( 1, maze.col_size() - 2 );
  Maze::Point choice = { row_random( generator ), col_random( generator ) };
  if ( !is_valid_start_or_finish( maze, choice ) ) {
    choice = find_nearest_square( maze, choice );
  }
  return choice;
}

void print_hunt_solution_message( const Maze::Square& winning_index )
{
  if ( !winning_index ) {
    std::cout << thread_colors.at( all_threads_failed_index.load() );
    return;
  }
  std::cout << ( thread_colors.at( thread_bits.at( winning_index.load() ).load() ) ) << " thread won!\n";
}

void print_gather_solution_message()
{
  for ( const Thread_paint& mask : thread_bits ) {
    std::cout << thread_colors.at( mask.load() );
  }
  std::cout << " All threads found their finish squares!\n";
}

void deluminate_maze( Maze::Maze& maze )
{
  for ( int row = 0; row < maze.row_size(); ++row ) {
    for ( int col = 0; col < maze.col_size(); ++col ) {
      Printer::set_cursor_position( { row, col } );
      std::cout << " ";
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

void print_overlap_key()
{
  std::cout << "┌────────────────────────────────────────────────────────────────┐\n"
            << "│     Overlap Key: 3_THREAD | 2_THREAD | 1_THREAD | 0_THREAD     │\n"
            << "├────────────┬────────────┬────────────┬────────────┬────────────┤\n"
            << "│ " << thread_colors.at( 1 ) << " = 0      │ " << thread_colors.at( 2 ) << " = 1      │ "
            << thread_colors.at( 3 ) << " = 1|0    │ " << thread_colors.at( 4 ) << " = 2      │ "
            << thread_colors.at( 5 ) << " = 2|0    │\n"
            << "├────────────┼────────────┼────────────┼────────────┼────────────┤\n"
            << "│ " << thread_colors.at( 6 ) << " = 2|1    │ " << thread_colors.at( 7 ) << " = 2|1|0  │ "
            << thread_colors.at( 8 ) << " = 3      │ " << thread_colors.at( 9 ) << " = 3|0    │ "
            << thread_colors.at( 10 ) << " = 3|1    │\n"
            << "├────────────┼────────────┼────────────┼────────────┼────────────┤\n"
            << "│ " << thread_colors.at( 11 ) << " = 3|1|0  │ " << thread_colors.at( 12 ) << " = 3|2    │ "
            << thread_colors.at( 13 ) << " = 3|2|0  │ " << thread_colors.at( 14 ) << " = 3|2|1  │ "
            << thread_colors.at( 15 ) << " = 3|2|1|0│\n"
            << "└────────────┴────────────┴────────────┴────────────┴────────────┘\n";
}

} // namespace Sutil
