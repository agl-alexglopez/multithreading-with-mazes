#pragma once
#ifndef MAZE_ALGORITHMS_HH
#define MAZE_ALGORITHMS_HH
#include "maze_utilities.hh"
#include "speed.hh"

namespace Builder {

void generate_recursive_backtracker_maze( Maze& maze );
void animate_recursive_backtracker_maze( Maze& maze, Speed::Speed speed );

void generate_recursive_subdivision_maze( Maze& maze );
void animate_recursive_subdivision_maze( Maze& maze, Speed::Speed speed );

void generate_wilson_path_carver_maze( Maze& maze );
void animate_wilson_path_carver_maze( Maze& maze, Speed::Speed speed );

void generate_wilson_wall_adder_maze( Maze& maze );
void animate_wilson_wall_adder_maze( Maze& maze, Speed::Speed speed );

void generate_kruskal_maze( Maze& maze );
void animate_kruskal_maze( Maze& maze, Speed::Speed speed );

void generate_prim_maze( Maze& maze );
void animate_prim_maze( Maze& maze, Speed::Speed speed );

void generate_eller_maze( Maze& maze );
void animate_eller_maze( Maze& maze, Speed::Speed speed );

void generate_grid_maze( Maze& maze );
void animate_grid_maze( Maze& maze, Speed::Speed speed );

void generate_arena( Maze& maze );
void animate_arena( Maze& maze, Speed::Speed speed );

} // namespace Builder

#endif
