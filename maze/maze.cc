module;
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>
export module labyrinth:maze;

export namespace Maze {

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

constexpr uint64_t wall_row = 16;
constexpr Square path_bit = 0b0010'0000'0000'0000;
constexpr Square clear_available_bits = 0b0001'1111'1111'0000;
constexpr Square start_bit = 0b0100'0000'0000'0000;
constexpr Square builder_bit = 0b0001'0000'0000'0000;
constexpr uint8_t marker_shift = 4;
constexpr Backtrack_marker markers_mask = 0b1111'0000;
constexpr Backtrack_marker is_origin = 0b0000'0000;
constexpr Backtrack_marker from_north = 0b0001'0000;
constexpr Backtrack_marker from_east = 0b0010'0000;
constexpr Backtrack_marker from_south = 0b0011'0000;
constexpr Backtrack_marker from_west = 0b0100'0000;
constexpr std::string_view from_north_mark = "\033[38;5;15m\033[48;5;1m↑\033[0m";
constexpr std::string_view from_east_mark = "\033[38;5;15m\033[48;5;2m→\033[0m";
constexpr std::string_view from_south_mark = "\033[38;5;15m\033[48;5;3m↓\033[0m";
constexpr std::string_view from_west_mark = "\033[38;5;15m\033[48;5;4m←\033[0m";

constexpr std::array<std::string_view, 5> backtracking_symbols
  = { { " ", from_north_mark, from_east_mark, from_south_mark, from_west_mark } };
constexpr std::array<Point, 5> backtracking_marks = { { { 0, 0 }, { -2, 0 }, { 0, 2 }, { 2, 0 }, { 0, -2 } } };
constexpr std::array<Point, 5> backtracking_half_marks = { { { 0, 0 }, { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } } };

constexpr Wall_line wall_mask = 0b1111;
constexpr Wall_line floating_wall = 0b0000;
constexpr Wall_line north_wall = 0b0001;
constexpr Wall_line east_wall = 0b0010;
constexpr Wall_line south_wall = 0b0100;
constexpr Wall_line west_wall = 0b1000;

/* Walls are constructed in terms of other walls they need to connect to. For example, read
 * 0b0011 as, "this is a wall square that must connect to other walls to the East and North."
 */
constexpr std::array<std::string_view, 96> wall_styles = { {
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
constexpr std::array<Point, 4> dirs = { { { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } } };
constexpr std::array<Point, 4> build_dirs = { { { -2, 0 }, { 0, 2 }, { 2, 0 }, { 0, -2 } } };
// south, south-east, east, north-east, north, north-west, west, south-west
constexpr std::array<Point, 8> all_dirs
  = { { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } } };

class Maze
{

public:
  explicit Maze( const Maze_args& args );
  std::span<Square> operator[]( uint64_t index );
  std::span<const Square> operator[]( uint64_t index ) const;
  int row_size() const;
  int col_size() const;
  std::span<const std::string_view> wall_style() const;

private:
  int maze_row_size_;
  int maze_col_size_;
  // Model a ROWxCOLUMN maze with a single flat array and manage indexing with []operators.
  std::vector<Square> maze_;
  int wall_style_index_;
};

Maze::Maze( const Maze_args& args )
  : maze_row_size_( static_cast<int>( args.odd_rows ) )
  , maze_col_size_( static_cast<int>( args.odd_cols ) )
  , maze_( args.odd_rows * args.odd_cols, 0 )
  , wall_style_index_( static_cast<int>( args.style ) )
{}

std::span<Square> Maze::operator[]( uint64_t index )
{
  return { &maze_.at( index * maze_col_size_ ), static_cast<uint64_t>( maze_col_size_ ) };
}

std::span<const Square> Maze::operator[]( uint64_t index ) const
{
  return { &maze_.at( index * maze_col_size_ ), static_cast<uint64_t>( maze_col_size_ ) };
}

int Maze::row_size() const
{
  return maze_row_size_;
}

int Maze::col_size() const
{
  return maze_col_size_;
}

std::span<const std::string_view> Maze::wall_style() const
{
  return { &wall_styles.at( wall_style_index_ * wall_row ), wall_row };
}

bool operator==( const Point& lhs, const Point& rhs )
{
  return lhs.row == rhs.row && lhs.col == rhs.col;
}

bool operator!=( const Point& lhs, const Point& rhs )
{
  return !( lhs == rhs );
}

} // namespace Maze

// Points should be hashable for ease of use in most containers. Helpful for Kruskal's algorithm.
namespace std {
template<>
struct hash<Maze::Point>
{
  inline size_t operator()( const Maze::Point& p ) const
  {
    const std::hash<int> hasher;
    return hasher( p.row ) ^ hasher( p.col );
  }
};
} // namespace std
