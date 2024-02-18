module;
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
export module labyrinth:runs;
import :maze;
import :speed;
import :rgb;
import :my_queue;
import :printers;

/////////////////////////////////////   Exported Interface   /////////////////////////////////////

export namespace Runs {
void paint_runs( Maze::Maze& maze );
void animate_runs( Maze::Maze& maze, Speed::Speed speed );
} // namespace Runs

/////////////////////////////////////     Implementation     /////////////////////////////////////

namespace {

struct Run_map
{
  uint32_t max;
  std::unordered_map<Maze::Point, uint32_t> runs;
  Run_map( Maze::Point p, uint32_t run ) : max( run ), runs( { { p, run } } ) {}
};

struct Run_point
{
  uint32_t len;
  Maze::Point prev;
  Maze::Point cur;
};

void painter( Maze::Maze& maze, const Run_map& map )
{
  std::mt19937 rng( std::random_device {}() );
  std::uniform_int_distribution<int> uid( 0, 2 );
  const int rand_color_choice = uid( rng );
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      const Maze::Point cur = { row, col };
      const auto path_point = map.runs.find( cur );
      if ( path_point != map.runs.end() ) {
        const auto intensity = static_cast<double>( map.max - path_point->second ) / static_cast<double>( map.max );
        const auto dark = static_cast<uint8_t>( 255.0 * intensity );
        const auto bright = static_cast<uint8_t>( 128 ) + static_cast<uint8_t>( 127.0 * intensity );
        Rgb::Rgb color { dark, dark, dark };
        color.at( rand_color_choice ) = bright;
        Rgb::print_rgb( color, cur );
      } else {
        Rgb::print_wall( maze, cur );
      }
    }
  }
  std::cout << "\n";
}

void painter_animated( Maze::Maze& maze, const Run_map& map, Rgb::Bfs_monitor& monitor, Rgb::Thread_guide guide )
{
  My_queue<Maze::Point>& bfs = monitor.paths[guide.bias];
  std::unordered_set<Maze::Point>& seen = monitor.seen[guide.bias];
  bfs.push( guide.p );
  while ( !bfs.empty() ) {
    const Maze::Point cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( monitor.count == map.runs.size() ) {
      monitor.monitor.unlock();
      return;
    }
    if ( !( maze[cur.row][cur.col] & Rgb::paint ) ) {
      const uint64_t dist = map.runs.at( cur );
      const auto intensity = static_cast<double>( map.max - dist ) / static_cast<double>( map.max );
      const auto dark = static_cast<uint8_t>( 255.0 * intensity );
      const auto bright = static_cast<uint8_t>( 128 ) + static_cast<uint8_t>( 127.0 * intensity );
      Rgb::Rgb color { dark, dark, dark };
      color.at( guide.color_i ) = bright;
      Rgb::animate_rgb( color, cur );
      maze[cur.row][cur.col] |= Rgb::paint;
    }
    monitor.monitor.unlock();

    std::this_thread::sleep_for( std::chrono::microseconds( guide.animation ) );

    for ( uint64_t count = 0, i = guide.bias; count < Maze::dirs.size(); ++count, ++i %= Maze::dirs.size() ) {
      const Maze::Point& p = Maze::dirs.at( i );
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };

      monitor.monitor.lock();
      const bool is_path = ( maze[next.row][next.col] & Maze::path_bit ).load();
      monitor.monitor.unlock();

      if ( is_path && !seen.contains( next ) ) {
        bfs.push( next );
        seen.insert( next );
      }
    }
  }
}

} // namespace

namespace Runs {

void paint_runs( Maze::Maze& maze )
{
  const int row_mid = maze.row_size() / 2;
  const int col_mid = maze.col_size() / 2;
  const Maze::Point start = { row_mid + 1 - ( row_mid % 2 ), col_mid + 1 - ( col_mid % 2 ) };
  Run_map map( start, 0 );
  My_queue<Run_point> bfs;
  bfs.push( { 0, start, start } );
  maze[start.row][start.col] |= Rgb::measure;
  while ( !bfs.empty() ) {
    const Run_point cur = bfs.front();
    bfs.pop();
    map.max = std::max( map.max, cur.len );
    for ( const Maze::Point& p : Maze::dirs ) {
      const Maze::Point next = { cur.cur.row + p.row, cur.cur.col + p.col };
      if ( !( maze[next.row][next.col] & Maze::path_bit ) || ( maze[next.row][next.col] & Rgb::measure ) ) {
        continue;
      }
      const uint32_t len
        = std::abs( next.row - cur.prev.row ) == std::abs( next.col - cur.prev.col ) ? 1 : cur.len + 1;
      maze[next.row][next.col] |= Rgb::measure;
      map.runs.insert( { next, len } );
      bfs.push( { len, cur.cur, next } );
    }
  }
  painter( maze, map );
  std::cout << "\n";
}

void animate_runs( Maze::Maze& maze, Speed::Speed speed )
{
  const int row_mid = maze.row_size() / 2;
  const int col_mid = maze.col_size() / 2;
  const Maze::Point start = { row_mid + 1 - ( row_mid % 2 ), col_mid + 1 - ( col_mid % 2 ) };
  Run_map map( start, 0 );
  My_queue<Run_point> bfs;
  bfs.push( { 0, start, start } );
  maze[start.row][start.col] |= Rgb::measure;
  while ( !bfs.empty() ) {
    const Run_point cur = bfs.front();
    bfs.pop();
    map.max = std::max( map.max, cur.len );
    for ( const Maze::Point& p : Maze::dirs ) {
      const Maze::Point next = { cur.cur.row + p.row, cur.cur.col + p.col };
      if ( !( maze[next.row][next.col] & Maze::path_bit ) || ( maze[next.row][next.col] & Rgb::measure ) ) {
        continue;
      }
      const uint32_t len
        = std::abs( next.row - cur.prev.row ) == std::abs( next.col - cur.prev.col ) ? 1 : cur.len + 1;
      maze[next.row][next.col] |= Rgb::measure;
      map.runs.insert( { next, len } );
      bfs.push( { len, cur.cur, next } );
    }
  }

  std::mt19937 rng( std::random_device {}() );
  std::uniform_int_distribution<uint64_t> uid( 0, 2 );
  const uint64_t rand_color_choice = uid( rng );
  std::array<std::thread, Rgb::num_painters> handles;
  const Speed::Speed_unit animation = Rgb::animation_speeds.at( static_cast<uint64_t>( speed ) );
  Rgb::Bfs_monitor monitor;
  for ( uint64_t i = 0; i < handles.size(); i++ ) {
    Rgb::Thread_guide this_thread = { i, rand_color_choice, animation, start };
    handles.at( i )
      = std::thread( painter_animated, std::ref( maze ), std::ref( map ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : handles ) {
    t.join();
  }
  Printer::set_cursor_position( { maze.row_size(), maze.col_size() } );
  std::cout << "\n";
}

} // namespace Runs
