#pragma once
#ifndef PRINT_UTILITIES_HH
#define PRINT_UTILITIES_HH
#include "maze.hh"
#include <iostream>
#include <string>
namespace Printer {

inline void clear_screen()
{
  std::cout << "\033[2J\033[1;1H";
}

inline void set_cursor_position( const Builder::Maze::Point& p )
{
  std::cout << "\033[" + std::to_string( p.row + 1 ) + ";" + std::to_string( p.col + 1 ) + "f";
}

} // namespace Printer

#endif
