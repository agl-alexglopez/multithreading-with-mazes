module;
#include <array>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <string_view>
#include <unordered_set>
#include <vector>
export module labyrinth:rgb;
import :maze;
import :speed;
import :my_queue;
import :printers;

export namespace Rgb {

using Speed_unit = int32_t;
using Rgb = std::array<uint16_t, 3>;
constexpr uint64_t num_painters = 4;
constexpr Maze::Square paint = 0b1'0000'0000;
constexpr Maze::Square measure = 0b10'0000'0000;
constexpr uint64_t initial_path_len = 1024;
constexpr std::array<Speed_unit, 8> animation_speeds = { 0, 10000, 5000, 2000, 1000, 500, 250, 50 };
constexpr uint64_t r = 0;
constexpr uint64_t g = 1;
constexpr uint64_t b = 2;
constexpr std::string_view rgb_escape = "\033[38;2;";
constexpr std::string_view brush = "m█\033[0m";

struct Bfs_monitor
{
  std::mutex monitor {};
  uint64_t count { 0 };
  std::vector<My_queue<Maze::Point>> paths;
  std::vector<std::unordered_set<Maze::Point>> seen;
  Bfs_monitor() : paths { num_painters }, seen { num_painters }
  {
    for ( My_queue<Maze::Point>& p : paths ) {
      p.reserve( initial_path_len );
    }
  }
};

struct Thread_guide
{
  uint64_t bias;
  uint64_t color_i;
  Speed::Speed_unit animation;
  Maze::Point p;
};

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
