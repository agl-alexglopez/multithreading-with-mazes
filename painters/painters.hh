#pragma once
#ifndef PAINTERS_HH
#define PAINTERS_HH

#include "maze.hh"
#include "speed.hh"

namespace Paint {

void paint_distance_from_center( Builder::Maze& maze );
void animate_distance_from_center( Builder::Maze& maze, Speed::Speed speed );

void paint_runs( Builder::Maze& maze );
void animate_runs( Builder::Maze& maze, Speed::Speed speed );

} // namespace Paint

#endif // PAINTERS_HH
