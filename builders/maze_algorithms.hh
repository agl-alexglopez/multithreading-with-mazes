#pragma once
#ifndef MAZE_ALGORITHMS_HH
#define MAZE_ALGORITHMS_HH
#include "maze_utilities.hh"
#include "speed.hh"

namespace Builder {

void generate_recursive_backtracker( Maze& maze );
void animate_recursive_backtracker( Maze& maze, Speed::Speed speed );

void generate_recursive_subdivision( Maze& maze );
void animate_recursive_subdivision( Maze& maze, Speed::Speed speed );

void generate_wilson_path_carver( Maze& maze );
void animate_wilson_path_carver( Maze& maze, Speed::Speed speed );

void generate_wilson_wall_adder( Maze& maze );
void animate_wilson_wall_adder( Maze& maze, Speed::Speed speed );

void generate_kruskal( Maze& maze );
void animate_kruskal( Maze& maze, Speed::Speed speed );

void generate_prim( Maze& maze );
void animate_prim( Maze& maze, Speed::Speed speed );

void generate_eller( Maze& maze );
void animate_eller( Maze& maze, Speed::Speed speed );

void generate_grid( Maze& maze );
void animate_grid( Maze& maze, Speed::Speed speed );

void generate_arena( Maze& maze );
void animate_arena( Maze& maze, Speed::Speed speed );

} // namespace Builder

#endif
