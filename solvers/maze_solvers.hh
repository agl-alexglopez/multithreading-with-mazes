#pragma once
#include <optional>
#ifndef MAZE_SOLVERS_HH
#define MAZE_SOLVERS_HH
#include "maze.hh"
#include <array>
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
using Thread_paint = uint16_t;
using Thread_cache = uint16_t;
using Speed_unit = int;

struct Thread_id
{
  int index;
  Thread_paint paint;
};

enum class Maze_game
{
  hunt,
  gather,
  corners,
};

enum class Solver_speed
{
  instant,
  speed_1,
  speed_2,
  speed_3,
  speed_4,
  speed_5,
  speed_6,
  speed_7,
};

/* * * * * * * * * * * * * * * * * * * *     Solving Algorithms      * * * * * * * * * * * * * * * * * * * * * * */

void solve_with_dfs_thread_hunt( Builder::Maze& maze );
void solve_with_dfs_thread_gather( Builder::Maze& maze );
void solve_with_dfs_thread_corners( Builder::Maze& maze );

void animate_with_dfs_thread_hunt( Builder::Maze& maze, Solver_speed speed );
void animate_with_dfs_thread_gather( Builder::Maze& maze, Solver_speed speed );
void animate_with_dfs_thread_corners( Builder::Maze& maze, Solver_speed speed );

void solve_with_randomized_dfs_thread_hunt( Builder::Maze& maze );
void solve_with_randomized_dfs_thread_gather( Builder::Maze& maze );
void solve_with_randomized_dfs_thread_corners( Builder::Maze& maze );

void animate_with_randomized_dfs_thread_hunt( Builder::Maze& maze, Solver_speed speed );
void animate_with_randomized_dfs_thread_gather( Builder::Maze& maze, Solver_speed speed );
void animate_with_randomized_dfs_thread_corners( Builder::Maze& maze, Solver_speed speed );

void solve_with_bfs_thread_hunt( Builder::Maze& maze );
void solve_with_bfs_thread_gather( Builder::Maze& maze );
void solve_with_bfs_thread_corners( Builder::Maze& maze );

void animate_with_bfs_thread_hunt( Builder::Maze& maze, Solver_speed speed );
void animate_with_bfs_thread_gather( Builder::Maze& maze, Solver_speed speed );
void animate_with_bfs_thread_corners( Builder::Maze& maze, Solver_speed speed );

/* * * * * * * * * * * * *   Generic Read-Only Helpers Available to All Solvers  * * * * * * * * * * * * * * * * */

std::vector<Builder::Maze::Point> set_corner_starts( const Builder::Maze& maze );
Builder::Maze::Point pick_random_point( const Builder::Maze& maze );
Builder::Maze::Point find_nearest_square( const Builder::Maze& maze, const Builder::Maze::Point& choice );
void clear_screen();
void print_point( const Builder::Maze& maze, const Builder::Maze::Point& point );
void print_maze( const Builder::Maze& maze );
void flush_cursor_path_coordinate( const Builder::Maze& maze, const Builder::Maze::Point& point );
void set_cursor_point( const Builder::Maze::Point& point );
void clear_and_flush_paths( const Builder::Maze& maze );
void print_hunt_solution_message( std::optional<int> winning_index );
void print_gather_solution_message();
void print_overlap_key();

/* * * * * * * * * * * * *     Helpful Read-Only Data Available to All Solvers   * * * * * * * * * * * * * * * * */

constexpr Thread_paint start_bit_ = 0b0100'0000'0000'0000;
constexpr Thread_paint finish_bit_ = 0b1000'0000'0000'0000;
constexpr int num_threads_ = 4;
constexpr Thread_paint thread_tag_offset_ = 4;
constexpr int num_gather_finishes_ = 4;
constexpr int initial_path_len_ = 1024;
constexpr Thread_paint thread_mask_ = 0b1111'0000;
constexpr Thread_paint zero_thread_ = 0b0001'0000;
constexpr Thread_paint one_thread_ = 0b0010'0000;
constexpr Thread_paint two_thread_ = 0b0100'0000;
constexpr Thread_paint three_thread_ = 0b1000'0000;
constexpr Thread_paint error_thread_ = 0b0000'0000;
constexpr std::array<Thread_paint, 4> thread_masks_ = { zero_thread_, one_thread_, two_thread_, three_thread_ };

constexpr Thread_cache clear_cache_ = 0b0001'1111'1111'0000;
constexpr Thread_cache cache_mask_ = 0b1111'0000'0000;
constexpr Thread_cache zero_seen_ = 0b0001'0000'0000;
constexpr Thread_cache one_seen_ = 0b0010'0000'0000;
constexpr Thread_cache two_seen_ = 0b0100'0000'0000;
constexpr Thread_cache three_seen_ = 0b1000'0000'0000;

constexpr int starting_path_len_ = 2048;
constexpr std::string_view ansi_clear_screen_ = "\033[2J\033[1;1H";
constexpr std::string_view ansi_red_ = "\033[38;5;1m█\033[0m";
constexpr std::string_view ansi_grn_ = "\033[38;5;2m█\033[0m";
constexpr std::string_view ansi_yel_ = "\033[38;5;3m█\033[0m";
constexpr std::string_view ansi_blu_ = "\033[38;5;4m█\033[0m";
constexpr std::string_view ansi_prp_ = "\033[38;5;183m█\033[0m";
constexpr std::string_view ansi_mag_ = "\033[38;5;201m█\033[0m";
constexpr std::string_view ansi_cyn_ = "\033[38;5;87m█\033[0m";
constexpr std::string_view ansi_wit_ = "\033[38;5;231m█\033[0m";
constexpr std::string_view ansi_prp_red_ = "\033[38;5;204m█\033[0m";
constexpr std::string_view ansi_blu_mag_ = "\033[38;5;105m█\033[0m";
constexpr std::string_view ansi_red_grn_blu_ = "\033[38;5;121m█\033[0m";
constexpr std::string_view ansi_grn_prp_ = "\033[38;5;106m█\033[0m";
constexpr std::string_view ansi_grn_blu_prp_ = "\033[38;5;60m█\033[0m";
constexpr std::string_view ansi_red_grn_prp_ = "\033[38;5;105m█\033[0m";
constexpr std::string_view ansi_red_blu_prp_ = "\033[38;5;89m█\033[0m";
constexpr std::string_view ansi_dark_blu_mag_ = "\033[38;5;57m█\033[0m";
constexpr std::string_view ansi_bold_ = "\033[1m";
constexpr std::string_view ansi_nil_ = "\033[0m";
constexpr std::string_view ansi_no_solution_ = "\033[38;5;15m\033[255;0;0m╳\033[0m";
constexpr std::string_view ansi_finish_ = "\033[1m\033[38;5;87mF\033[0m";
constexpr std::string_view ansi_start_ = "\033[1m\033[38;5;87mS\033[0m";
constexpr Thread_paint all_threads_failed_index_ = 0;
constexpr std::array<std::string_view, 16> thread_colors_ = {
  ansi_no_solution_,
  // Threads Overlaps. The zero thread is the zero index bit with a value of 1.
  // 0b0001   0b0010     0b0011     0b0100     0b0101     0b0110        0b0111
  ansi_red_,
  ansi_grn_,
  ansi_yel_,
  ansi_blu_,
  ansi_mag_,
  ansi_cyn_,
  ansi_red_grn_blu_,
  // 0b1000    0b1001          0b1010           0b1011            0b1100
  ansi_prp_,
  ansi_prp_red_,
  ansi_grn_prp_,
  ansi_red_grn_prp_,
  ansi_dark_blu_mag_,
  // 0b1101              0b1110          0b1111
  ansi_red_blu_prp_,
  ansi_grn_blu_prp_,
  ansi_wit_,
};
// north, east, south, west
constexpr std::array<Builder::Maze::Point, 4> cardinal_directions_
  = { { { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } } };
// north, east, south, west
constexpr std::array<Builder::Maze::Point, 4> generate_directions_
  = { { { -2, 0 }, { 0, 2 }, { 2, 0 }, { 0, -2 } } };
// north, north-east, east, south-east, south, south-west, west, north-west
constexpr std::array<Builder::Maze::Point, 7> all_directions_
  = { { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 } } };
constexpr int overlap_key_and_message_height = 11;
constexpr std::array<Speed_unit, 8> solver_speeds_ = { 0, 20000, 10000, 5000, 2000, 1000, 500, 250 };

} // namespace Solver

#endif
