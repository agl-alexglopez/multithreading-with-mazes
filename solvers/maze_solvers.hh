#pragma once
#ifndef MAZE_SOLVERS_HH
#define MAZE_SOLVERS_HH
#include "maze.hh"
#include "solver_utilities.hh"

namespace Solver {

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

void solve_with_floodfs_thread_hunt( Builder::Maze& maze );
void solve_with_floodfs_thread_gather( Builder::Maze& maze );
void solve_with_floodfs_thread_corners( Builder::Maze& maze );

void animate_with_floodfs_thread_hunt( Builder::Maze& maze, Solver_speed speed );
void animate_with_floodfs_thread_gather( Builder::Maze& maze, Solver_speed speed );
void animate_with_floodfs_thread_corners( Builder::Maze& maze, Solver_speed speed );

void solve_with_bfs_thread_hunt( Builder::Maze& maze );
void solve_with_bfs_thread_gather( Builder::Maze& maze );
void solve_with_bfs_thread_corners( Builder::Maze& maze );

void animate_with_bfs_thread_hunt( Builder::Maze& maze, Solver_speed speed );
void animate_with_bfs_thread_gather( Builder::Maze& maze, Solver_speed speed );
void animate_with_bfs_thread_corners( Builder::Maze& maze, Solver_speed speed );


} // namespace Solver

#endif
