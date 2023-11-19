#pragma once
#ifndef SOLVER_UTILITIES_HH
#define SOLVER_UTILITIES_HH
#include "maze.hh"
#include "speed.hh"

#include <array>
#include <optional>
#include <string_view>

namespace Solver {

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
using Thread_bit = uint16_t;
using Thread_paint = uint16_t;
using Thread_cache = uint16_t;

struct Thread_id
{
  int index;
  Thread_bit bit;
};

enum class Maze_game
{
  hunt,
  gather,
  corners,
};

/* * * * * * * * * * * * *   Generic Read-Only Helpers Available to All Solvers  * * * * * * * * * * * * * * * * */

std::vector<Builder::Maze::Point> set_corner_starts( const Builder::Maze& maze );
Builder::Maze::Point pick_random_point( const Builder::Maze& maze );
Builder::Maze::Point find_nearest_square( const Builder::Maze& maze, const Builder::Maze::Point& choice );
void print_point( const Builder::Maze& maze, const Builder::Maze::Point& point );
void print_maze( const Builder::Maze& maze );
void flush_cursor_path_coordinate( const Builder::Maze& maze, const Builder::Maze::Point& point );
void deluminate_maze( Builder::Maze& maze );
void clear_and_flush_paths( const Builder::Maze& maze );
void print_hunt_solution_message( std::optional<int> winning_index );
void print_gather_solution_message();
void print_overlap_key();

/* * * * * * * * * * * * *     Helpful Read-Only Data Available to All Solvers   * * * * * * * * * * * * * * * * */

constexpr int num_threads = 4;
constexpr Thread_bit start_bit = 0b0100'0000'0000'0000;
constexpr Thread_bit finish_bit = 0b1000'0000'0000'0000;
constexpr Thread_bit zero_bit = 0b0001;
constexpr Thread_bit one_bit = 0b0010;
constexpr Thread_bit two_bit = 0b0100;
constexpr Thread_bit three_bit = 0b1000;
constexpr std::array<Thread_bit, 4> thread_bits = { zero_bit, one_bit, two_bit, three_bit };
constexpr int initial_path_len = 1024;

constexpr Thread_paint thread_paint_shift = 4;
constexpr Thread_paint thread_paint_mask = 0b1111'0000;
constexpr int num_gather_finishes = 4;

constexpr Thread_cache clear_cache = 0b0001'1111'1111'0000;
constexpr Thread_cache cache_mask = 0b1111'0000'0000;
constexpr Thread_cache zero_seen = 0b0001'0000'0000;
constexpr Thread_cache one_seen = 0b0010'0000'0000;
constexpr Thread_cache two_seen = 0b0100'0000'0000;
constexpr Thread_cache three_seen = 0b1000'0000'0000;
constexpr Thread_paint thread_cache_shift = 8;

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
constexpr Thread_paint all_threads_failed_index = 0;
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
constexpr std::array<Builder::Maze::Point, 4> dirs = { { { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } } };
// north, north-east, east, south-east, south, south-west, west, north-west
constexpr std::array<Builder::Maze::Point, 7> all_dirs
  = { { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 } } };
constexpr int overlap_key_and_message_height = 9;
constexpr std::array<Speed::Speed_unit, 8> solver_speeds = { 0, 20000, 10000, 5000, 2000, 1000, 500, 250 };

} // namespace Solver

#endif
