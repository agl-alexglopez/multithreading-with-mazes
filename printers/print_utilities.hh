#pragma once
#ifndef PRINT_UTILITIES_HH
#define PRINT_UTILITIES_HH
#include "maze.hh"

namespace Printer {

void clear_screen();
void set_cursor_position( const Builder::Maze::Point& p );

} // namespace Printer

#endif
