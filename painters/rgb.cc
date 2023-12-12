module;
#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>
export module labyrinth:rgb;
import :maze;
import :printers;

export namespace Rgb {

using Speed_unit = int32_t;

using Rgb = std::array<uint16_t, 3>;

constexpr Maze::Square paint = 0b1'0000'0000;
constexpr Maze::Square measure = 0b10'0000'0000;
constexpr uint64_t num_painters = 4;
constexpr uint64_t initial_path_len = 1024;
constexpr std::array<Speed_unit, 8> animation_speeds = { 0, 10000, 5000, 2000, 1000, 500, 250, 50 };
constexpr uint64_t r = 0;
constexpr uint64_t g = 1;
constexpr uint64_t b = 2;
constexpr std::string_view rgb_escape = "\033[38;2;";
constexpr std::string_view brush = "m█\033[0m";

void print_rgb( Rgb rgb, Maze::Point p )
{
  Printer::set_cursor_position( p );
  std::cout << rgb_escape << rgb[r] << ";" << rgb[g] << ";" << rgb[b] << brush;
}

void animate_rgb( Rgb rgb, Maze::Point p )
{
  Printer::set_cursor_position( p );
  std::cout << rgb_escape << rgb[r] << ";" << rgb[g] << ";" << rgb[b] << brush << std::flush;
}

void print_wall( Maze::Maze& maze, Maze::Point p )
{
  Printer::set_cursor_position( p );
  const Maze::Square& square = maze[p.row][p.col];
  std::cout << maze.wall_style()[square & Maze::wall_mask];
}

} // namespace Rgb
