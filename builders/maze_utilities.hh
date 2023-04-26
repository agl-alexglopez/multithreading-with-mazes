#pragma once
#ifndef MAZE_UTILITIES_HH
#define MAZE_UTILITIES_HH

#include "maze.hh"

#include <array>

namespace Builder
{

using Speed_unit = int;

enum class Builder_speed
{
  instant = 0,
  speed_1,
  speed_2,
  speed_3,
  speed_4,
  speed_5,
  speed_6,
  speed_7,
};

enum class Parity_point
{
  even,
  odd,
};

constexpr std::array<Speed_unit, 8> builder_speeds_ = { 0, 5000, 2500, 1000, 500, 250, 100, 1 };

void add_positive_slope( Maze& maze, const Maze::Point& p );
void add_positive_slope_animated( Maze& maze, const Maze::Point& p, Speed_unit speed );
void add_negative_slope( Maze& maze,  const Maze::Point& p );
void add_negative_slope_animated( Maze& maze,  const Maze::Point& p, Speed_unit speed );
void build_wall_line( Maze& maze,  const Maze::Point& p );
void build_wall_line_animated( Maze& maze,  const Maze::Point& p, Speed_unit speed );
void carve_path_walls( Maze& maze,  const Maze::Point& p );
void carve_path_walls_animated( Maze& maze, const Maze::Point& p, Speed_unit speed );
void carve_path_markings( Maze& maze,  const Maze::Point& cur, const Maze::Point& next );
void carve_path_markings_animated( Maze& maze,  const Maze::Point& cur, const Maze::Point& next, Speed_unit speed );
void build_wall_outline( Maze& maze );
void fill_maze_with_walls( Maze& maze );
void fill_maze_with_walls_animated( Maze& maze );
void add_cross( Maze& maze, const Maze::Point& p );
void add_cross_animated( Maze& maze, const Maze::Point& p, Speed_unit speed );
void add_x( Maze& maze, const Maze::Point& p );
void add_x_animated( Maze& maze, const Maze::Point& p, Speed_unit speed );
void join_squares( Maze& maze, const Maze::Point& cur, const Maze::Point& next );
void join_squares_animated( Maze& maze, const Maze::Point& cur, const Maze::Point& next, Speed_unit speed );
void mark_origin( Maze& maze, const Maze::Point& walk, const Maze::Point& next );
void mark_origin_animated( Maze& maze, const Maze::Point& walk, const Maze::Point& next, Speed_unit speed );
void build_path( Maze& maze, const Maze::Point& p );
void build_path_animated( Maze& maze, const Maze::Point& p, Speed_unit speed );
void build_wall( Maze& maze, const Maze::Point& p );
void build_wall_carefully( Maze& maze, const Maze::Point& p );
Maze::Point find_nearest_square( const Maze& maze, Maze::Point choice );
Maze::Point choose_arbitrary_point( const Maze& maze, Parity_point parity );
void clear_and_flush_grid( Maze& maze );
void clear_for_wall_adders( Maze& maze );
void flush_cursor_maze_coordinate( const Maze& maze, const Maze::Point& p );
void print_square( const Maze& maze, const Maze::Point& p );
void print_maze( const Maze& maze );
void print_maze_square( const Maze& maze, const Maze::Point& p );
bool can_build_new_square( const Maze& maze, const Maze::Point& next );
bool has_builder_bit( const Maze& maze, const Maze::Point& next );
bool is_square_within_perimeter_walls( const Maze& maze, const Maze::Point& next );

void clear_screen();
void set_cursor_position( const Maze::Point& p );

} // namespace Builder

#endif
