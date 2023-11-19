#pragma once
#ifndef RGB_HH
#define RGB_HH

#include "maze.hh"

#include <array>
#include <cstdint>

namespace Paint {

using Speed_unit = int32_t;

using Rgb = std::array<uint16_t, 3>;

void print_rgb( Rgb rgb, Builder::Maze::Point p );
void animate_rgb( Rgb rgb, Builder::Maze::Point p );
void print_wall( Builder::Maze& maze, Builder::Maze::Point p );

constexpr Builder::Maze::Square paint = 0b1'0000'0000;
constexpr Builder::Maze::Square measure = 0b10'0000'0000;
constexpr uint64_t num_painters = 4;
constexpr uint64_t initial_path_len = 1024;
constexpr std::array<Speed_unit, 8> animation_speeds = { 0, 10000, 5000, 2000, 1000, 500, 250, 50 };
constexpr uint64_t r = 0;
constexpr uint64_t g = 1;
constexpr uint64_t b = 2;

} // namespace Paint

#endif // RGB_HH
