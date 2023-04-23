#ifndef MAZE_ALGORITHMS_HH
#define MAZE_ALGORITHMS_HH
#include "maze.hh"

void generate_recursive_backtracker_maze( Maze& maze );
void animate_recursive_backtracker_maze( Maze& maze );

void generate_recursive_subdivision_maze( Maze& maze );
void animate_recursive_subdivision_maze( Maze& maze );

void generate_wilson_path_carver_maze( Maze& maze );
void animate_wilson_path_carver_maze( Maze& maze );

void generate_wilson_wall_adder_maze( Maze& maze );
void animate_wilson_wall_adder_maze( Maze& maze );

void generate_kruskal_maze( Maze& maze );
void animate_kruskal_maze( Maze& maze );

#endif
