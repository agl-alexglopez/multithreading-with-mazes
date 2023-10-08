#include "maze.hh"
#include "print_utilities.hh"

#include <iostream>
#include <string>
#include <string_view>

namespace Printer {

namespace {

constexpr std::string_view ansi_clear_screen_ = "\033[2J\033[1;1H";

} // namespace

void clear_screen()
{
  std::cout << ansi_clear_screen_;
}

void set_cursor_position( const Builder::Maze::Point& p )
{
  std::cout << "\033[" + std::to_string( p.row + 1 ) + ";" + std::to_string( p.col + 1 ) + "f";
}

} // namespace Printer
