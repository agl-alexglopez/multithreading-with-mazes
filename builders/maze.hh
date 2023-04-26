#ifndef MAZE_HH
#define MAZE_HH

#include <array>
#include <cmath>
#include <cstdint>
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

  enum class Builder_speed
  {
    instant = 0,
    speed_1,
    speed_2,
    speed_3,
    speed_4,
    speed_5,
    speed_6,
    speed_7,
  };

  struct Point
  {
    int row;
    int col;
  };

  enum class Parity_point
  {
    even,
    odd,
  };

  struct Maze_args
  {
    uint64_t odd_rows = 31;
    uint64_t odd_cols = 111;
    Maze_modification modification = Maze_modification::none;
    Maze_style style = Maze_style::sharp;
    Builder_speed builder_speed = Builder_speed::instant;
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
  using Animation_speed = int;

  explicit Maze(const Maze_args& args);

  std::vector<Square>& operator[]( uint64_t index );
  const std::vector<Square>& operator[]( uint64_t index ) const;
  int row_size() const;
  int col_size() const;
  Animation_speed build_speed() const;
  void add_positive_slope( int row, int col );
  void add_positive_slope_animated( int row, int col );
  void add_negative_slope( int row, int col );
  void add_negative_slope_animated( int row, int col );
  void build_wall_line( int row, int col );
  void build_wall_line_animated( int row, int col );
  void carve_path_walls( int row, int col );
  void carve_path_walls_animated( int row, int col );
  void carve_path_markings( const Point& cur, const Point& next );
  void carve_path_markings_animated( const Point& cur, const Point& next );
  void build_wall_outline();
  void fill_maze_with_walls();
  void fill_maze_with_walls_animated();
  void add_modification( int row, int col );
  void add_modification_animated( int row, int col );
  void join_squares( const Point& cur, const Point& next );
  void join_squares_animated( const Point& cur, const Point& next );
  void mark_origin( const Point& walk, const Point& next );
  void mark_origin_animated( const Point& walk, const Point& next );
  void build_path( int row, int col );
  void build_path_animated( int row, int col );
  void build_wall( int row, int col );
  void build_wall_carefully( int row, int col );
  Point find_nearest_square( Point choice );
  Point choose_arbitrary_point( Parity_point parity ) const;
  void clear_and_flush_grid() const;
  void clear_for_wall_adders();
  void flush_cursor_maze_coordinate( int row, int col ) const;
  void print_square( int row, int col ) const;
  void print_maze() const;
  void print_maze_square( int row, int col ) const;
  bool can_build_new_square( const Point& next ) const;
  bool has_builder_bit( const Point& next ) const;
  bool is_square_within_perimeter_walls( const Point& next ) const;

  static void clear_screen();
  static void set_cursor_position( int row, int col );



  static constexpr Square path_bit_ = 0b0010'0000'0000'0000;
  static constexpr Square clear_available_bits_ = 0b0001'1111'1111'0000;



  static constexpr Square start_bit_ = 0b0100'0000'0000'0000;
  static constexpr Square builder_bit_ = 0b0001'0000'0000'0000;
  static constexpr int marker_shift_ = 4;
  static constexpr Backtrack_marker markers_mask_ = 0b1111'0000;
  static constexpr Backtrack_marker is_origin_ = 0b0000'0000;
  static constexpr Backtrack_marker from_north_ = 0b0001'0000;
  static constexpr Backtrack_marker from_east_ = 0b0010'0000;
  static constexpr Backtrack_marker from_south_ = 0b0011'0000;
  static constexpr Backtrack_marker from_west_ = 0b0100'0000;
  static constexpr std::string_view ansi_clear_screen_ = "\033[2J\033[1;1H";
  static constexpr std::string_view from_north_mark_ = "\033[38;5;15m\033[48;5;1m↑\033[0m";
  static constexpr std::string_view from_east_mark_ = "\033[38;5;15m\033[48;5;2m→\033[0m";
  static constexpr std::string_view from_south_mark_ = "\033[38;5;15m\033[48;5;3m↓\033[0m";
  static constexpr std::string_view from_west_mark_ = "\033[38;5;15m\033[48;5;4m←\033[0m";

  static constexpr std::array<std::string_view, 5> backtracking_symbols_
    = { { " ", from_north_mark_, from_east_mark_, from_south_mark_, from_west_mark_ } };
  static constexpr std::array<Point, 5> backtracking_marks_ = { {
    { 0, 0 },
    { -2, 0 },
    { 0, 2 },
    { 2, 0 },
    { 0, -2 },
  } };

  static constexpr Wall_line wall_mask_ = 0b1111;
  static constexpr Wall_line floating_wall_ = 0b0000;
  static constexpr Wall_line north_wall_ = 0b0001;
  static constexpr Wall_line east_wall_ = 0b0010;
  static constexpr Wall_line south_wall_ = 0b0100;
  static constexpr Wall_line west_wall_ = 0b1000;

  /* Walls are constructed in terms of other walls they need to connect to. For example, read
   * 0b0011 as, "this is a wall square that must connect to other walls to the East and North."
   */
  static constexpr std::array<const std::array<std::string_view, 16>, 6> wall_styles_ = { {
    { // 0bWestSouthEastNorth. Note: 0b0000 is a floating wall with no walls around.
      // 0b0000  0b0001  0b0010  0b0011  0b0100  0b0101  0b0110  0b0111
      // 0b1000  0b1001  0b1010  0b1011  0b1100  0b1101  0b1110  0b1111
      "■",
      "╵",
      "╶",
      "└",
      "╷",
      "│",
      "┌",
      "├",
      "╴",
      "┘",
      "─",
      "┴",
      "┐",
      "┤",
      "┬",
      "┼" },
    { // Same but with rounded corners.
      "●",
      "╵",
      "╶",
      "╰",
      "╷",
      "│",
      "╭",
      "├",
      "╴",
      "╯",
      "─",
      "┴",
      "╮",
      "┤",
      "┬",
      "┼" },
    { // Same but with double lines.
      "◫",
      "║",
      "═",
      "╚",
      "║",
      "║",
      "╔",
      "╠",
      "═",
      "╝",
      "═",
      "╩",
      "╗",
      "╣",
      "╦",
      "╬" },
    { // Same but with bold lines.
      "■",
      "╹",
      "╺",
      "┗",
      "╻",
      "┃",
      "┏",
      "┣",
      "╸",
      "┛",
      "━",
      "┻",
      "┓",
      "┫",
      "┳",
      "╋" },
    {
      // Simpler approach that creates well-connected high contrast black/white.
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
      "█",
    },
    { // Same but with crosses that create spikes.
      "✸",
      "╀",
      "┾",
      "╊",
      "╁",
      "╂",
      "╆",
      "╊",

      "┽",
      "╃",
      "┿",
      "╇",
      "╅",
      "╉",
      "╈",
      "╋" },
  } };

  //                                                               n      e     s      w
  static constexpr std::array<Point, 4> cardinal_directions_ = { { { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } } };
  static constexpr std::array<Point, 4> generate_directions_ = { { { -2, 0 }, { 0, 2 }, { 2, 0 }, { 0, -2 } } };
  static constexpr std::array<Point, 7> all_directions_
    = { { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 } } };

  static constexpr std::array<Animation_speed, 8> builder_speeds_ = { 0, 5000, 2500, 1000, 500, 250, 100, 1 };

private:

  const int maze_row_size_;
  const int maze_col_size_;
  std::vector<std::vector<Square>> maze_;
  const int builder_speed_units_;
  const int wall_style_index_;
  const Maze_modification modification_;

};


} // namespace Builder


bool operator==( const Builder::Maze::Point& lhs, const Builder::Maze::Point& rhs );
bool operator!=( const Builder::Maze::Point& lhs, const Builder::Maze::Point& rhs );

// Points should be hashable for ease of use in most containers. Helpful for Kruskal's algorithm.
namespace std {
template<>
struct hash<Builder::Maze::Point>
{
  inline size_t operator()( const Builder::Maze::Point& p ) const
  {
    std::hash<int> hasher;
    return hasher( p.row ) ^ hasher( p.col );
  }
};
} // namespace std


#endif
