#pragma once
#ifndef MAZE_SOLVERS_HH
#define MAZE_SOLVERS_HH
#include "maze.hh"
#include "solver_utilities.hh"
#include "speed.hh"

namespace Solver {

void dfs_thread_hunt( Builder::Maze& maze );
void dfs_thread_gather( Builder::Maze& maze );
void dfs_thread_corners( Builder::Maze& maze );

void animate_dfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed );
void animate_dfs_thread_gather( Builder::Maze& maze, Speed::Speed speed );
void animate_dfs_thread_corners( Builder::Maze& maze, Speed::Speed speed );

void randomized_dfs_thread_hunt( Builder::Maze& maze );
void randomized_dfs_thread_gather( Builder::Maze& maze );
void randomized_dfs_thread_corners( Builder::Maze& maze );

void animate_randomized_dfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed );
void animate_randomized_dfs_thread_gather( Builder::Maze& maze, Speed::Speed speed );
void animate_randomized_dfs_thread_corners( Builder::Maze& maze, Speed::Speed speed );

void floodfs_thread_hunt( Builder::Maze& maze );
void floodfs_thread_gather( Builder::Maze& maze );
void floodfs_thread_corners( Builder::Maze& maze );

void animate_floodfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed );
void animate_floodfs_thread_gather( Builder::Maze& maze, Speed::Speed speed );
void animate_floodfs_thread_corners( Builder::Maze& maze, Speed::Speed speed );

void bfs_thread_hunt( Builder::Maze& maze );
void bfs_thread_gather( Builder::Maze& maze );
void bfs_thread_corners( Builder::Maze& maze );

void animate_bfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed );
void animate_bfs_thread_gather( Builder::Maze& maze, Speed::Speed speed );
void animate_bfs_thread_corners( Builder::Maze& maze, Speed::Speed speed );

void animate_darkdfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed );
void animate_darkdfs_thread_gather( Builder::Maze& maze, Speed::Speed speed );
void animate_darkdfs_thread_corners( Builder::Maze& maze, Speed::Speed speed );

void animate_darkbfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed );
void animate_darkbfs_thread_gather( Builder::Maze& maze, Speed::Speed speed );
void animate_darkbfs_thread_corners( Builder::Maze& maze, Speed::Speed speed );

void animate_darkfloodfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed );
void animate_darkfloodfs_thread_gather( Builder::Maze& maze, Speed::Speed speed );
void animate_darkfloodfs_thread_corners( Builder::Maze& maze, Speed::Speed speed );

void animate_darkrandomized_dfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed );
void animate_darkrandomized_dfs_thread_gather( Builder::Maze& maze, Speed::Speed speed );
void animate_darkrandomized_dfs_thread_corners( Builder::Maze& maze, Speed::Speed speed );

} // namespace Solver

#endif
