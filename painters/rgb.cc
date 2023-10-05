#include "rgb.hh"
#include "print_utilities.hh"

#include <iostream>
#include <string_view>

namespace Paint {

constexpr std::string_view rgb_escape = "\033[38;2;";
constexpr std::string_view brush = "m█\033[0m";

void print_rgb( Rgb rgb, Builder::Maze::Point p )
{
  Printer::set_cursor_position( p );
  std::cout << rgb_escape << rgb[R] << ";" << rgb[G] << ";" << rgb[B] << brush;
}

void animate_rgb( Rgb rgb, Builder::Maze::Point p )
{
  Printer::set_cursor_position( p );
  std::cout << rgb_escape << rgb[R] << ";" << rgb[G] << ";" << rgb[B] << brush << std::flush;
}

void print_wall( Builder::Maze& maze, Builder::Maze::Point p )
{
  Printer::set_cursor_position( p );
  const Builder::Maze::Square& square = maze[p.row][p.col];
  std::cout << maze.wall_style()[square & Builder::Maze::wall_mask_];
}

} // namespace Paint
