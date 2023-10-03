#ifndef MAZE_HH
#define MAZE_HH

#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Builder {

class Maze
{

public:
  enum class Maze_modification
  {
    none,
    add_cross,
    add_x
  };

  enum class Maze_style
  {
    sharp = 0,
    round,
    doubles,
    bold,
    contrast,
    spikes,
  };

  struct Point
  {
    int row;
    int col;
  };

  struct Maze_args
  {
    uint64_t odd_rows = 31;
    uint64_t odd_cols = 111;
    Maze_style style = Maze_style::sharp;
  };

  /* Here is the scheme we will use to store tons of data in a square.
   *
   * When building the maze here is how we will use the available bits.
   *
   * wall structure----------------------||||
   * ------------------------------------||||
   * 0 backtrack marker bit------------| ||||
   * 1 backtrack marker bit ----------|| ||||
   * 2 backtrack marker bit----------||| ||||
   * 3 unused-----------------------|||| ||||
   * -------------------------------|||| ||||
   * 0 unused bit-----------------| |||| ||||
   * 1 unused bit----------------|| |||| ||||
   * 2 unused bit---------------||| |||| ||||
   * 3 unused bit--------------|||| |||| ||||
   * --------------------------|||| |||| ||||
   * maze build bit----------| |||| |||| ||||
   * maze paths bit---------|| |||| |||| ||||
   * maze start bit--------||| |||| |||| ||||
   * maze goals bit-------|||| |||| |||| ||||
   *                    0b0000 0000 0000 0000
   *
   * The maze builder is responsible for zeroing out the direction bits as part of the
   * building process. When solving the maze we adjust how we use the middle bits.
   *
   * wall structure----------------------||||
   * ------------------------------------||||
   * 0 thread paint--------------------| ||||
   * 1 thread paint-------------------|| ||||
   * 2 thread paint------------------||| ||||
   * 3 thread paint-----------------|||| ||||
   * -------------------------------|||| ||||
   * 0 thread cache---------------| |||| ||||
   * 1 thread cache--------------|| |||| ||||
   * 2 thread cache-------------||| |||| ||||
   * 3 thread cache------------|||| |||| ||||
   * --------------------------|||| |||| ||||
   * maze build bit----------| |||| |||| ||||
   * maze paths bit---------|| |||| |||| ||||
   * maze start bit--------||| |||| |||| ||||
   * maze goals bit-------|||| |||| |||| ||||
   *                    0b0000 0000 0000 0000
   */
  using Square = uint16_t;
  using Wall_line = uint16_t;
  using Backtrack_marker = uint16_t;

  explicit Maze( const Maze_args& args );

  std::span<Square> operator[]( uint64_t index );
  std::span<const Square> operator[]( uint64_t index ) const;
  int row_size() const;
  int col_size() const;
  static constexpr uint64_t wall_row_ = 16;
  std::span<const std::string_view> wall_style() const;

  static constexpr Square path_bit_ = 0b0010'0000'0000'0000;
  static constexpr Square clear_available_bits_ = 0b0001'1111'1111'0000;
  static constexpr Square start_bit_ = 0b0100'0000'0000'0000;
  static constexpr Square builder_bit_ = 0b0001'0000'0000'0000;
  static constexpr uint8_t marker_shift_ = 4;
  static constexpr Backtrack_marker markers_mask_ = 0b1111'0000;
  static constexpr Backtrack_marker is_origin_ = 0b0000'0000;
  static constexpr Backtrack_marker from_north_ = 0b0001'0000;
  static constexpr Backtrack_marker from_east_ = 0b0010'0000;
  static constexpr Backtrack_marker from_south_ = 0b0011'0000;
  static constexpr Backtrack_marker from_west_ = 0b0100'0000;
  static constexpr std::string_view from_north_mark_ = "\033[38;5;15m\033[48;5;1m↑\033[0m";
  static constexpr std::string_view from_east_mark_ = "\033[38;5;15m\033[48;5;2m→\033[0m";
  static constexpr std::string_view from_south_mark_ = "\033[38;5;15m\033[48;5;3m↓\033[0m";
  static constexpr std::string_view from_west_mark_ = "\033[38;5;15m\033[48;5;4m←\033[0m";

  static constexpr std::array<std::string_view, 5> backtracking_symbols_
    = { { " ", from_north_mark_, from_east_mark_, from_south_mark_, from_west_mark_ } };
  static constexpr std::array<Point, 5> backtracking_marks_
    = { { { 0, 0 }, { -2, 0 }, { 0, 2 }, { 2, 0 }, { 0, -2 } } };
  static constexpr std::array<Point, 5> backtracking_half_marks_
    = { { { 0, 0 }, { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } } };

  static constexpr Wall_line wall_mask_ = 0b1111;
  static constexpr Wall_line floating_wall_ = 0b0000;
  static constexpr Wall_line north_wall_ = 0b0001;
  static constexpr Wall_line east_wall_ = 0b0010;
  static constexpr Wall_line south_wall_ = 0b0100;
  static constexpr Wall_line west_wall_ = 0b1000;

  /* Walls are constructed in terms of other walls they need to connect to. For example, read
   * 0b0011 as, "this is a wall square that must connect to other walls to the East and North."
   */
  static constexpr std::array<std::string_view, 96> wall_styles_ = { {
    // 0bWestSouthEastNorth. Note: 0b0000 is a floating wall with no walls around.
    // 0b0000  0b0001  0b0010  0b0011  0b0100  0b0101  0b0110  0b0111
    // 0b1000  0b1001  0b1010  0b1011  0b1100  0b1101  0b1110  0b1111
    "■", "╵", "╶", "└", "╷", "│", "┌", "├", "╴", "┘", "─", "┴", "┐", "┤", "┬", "┼", // standard
    "●", "╵", "╶", "╰", "╷", "│", "╭", "├", "╴", "╯", "─", "┴", "╮", "┤", "┬", "┼", // rounded
    "◫", "║", "═", "╚", "║", "║", "╔", "╠", "═", "╝", "═", "╩", "╗", "╣", "╦", "╬", // doubles
    "■", "╹", "╺", "┗", "╻", "┃", "┏", "┣", "╸", "┛", "━", "┻", "┓", "┫", "┳", "╋", // bold
    "█", "█", "█", "█", "█", "█", "█", "█", "█", "█", "█", "█", "█", "█", "█", "█", // contrast
    "✸", "╀", "┾", "╊", "╁", "╂", "╆", "╊", "┽", "╃", "┿", "╇", "╅", "╉", "╈", "╋", // spikes
  } };

  // north, east, south, west
  static constexpr std::array<Point, 4> cardinal_directions_ = { { { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } } };
  static constexpr std::array<Point, 4> generate_directions_ = { { { -2, 0 }, { 0, 2 }, { 2, 0 }, { 0, -2 } } };
  // south, south-east, east, north-east, north, north-west, west, south-west
  static constexpr std::array<Point, 8> all_directions_
    = { { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } } };

private:
  int maze_row_size_;
  int maze_col_size_;
  // Model a ROWxCOLUMN maze with a single flat array and manage indexing with []operators.
  std::vector<Square> maze_;
  int wall_style_index_;
};

bool operator==( const Maze::Point& lhs, const Builder::Maze::Point& rhs );
bool operator!=( const Maze::Point& lhs, const Builder::Maze::Point& rhs );

} // namespace Builder

// Points should be hashable for ease of use in most containers. Helpful for Kruskal's algorithm.
namespace std {
template<>
struct hash<Builder::Maze::Point>
{
  inline size_t operator()( const Builder::Maze::Point& p ) const
  {
    const std::hash<int> hasher;
    return hasher( p.row ) ^ hasher( p.col );
  }
};
} // namespace std

#endif
