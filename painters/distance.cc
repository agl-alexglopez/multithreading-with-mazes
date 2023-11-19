#include "maze.hh"
#include "my_queue.hh"
#include "painters.hh"
#include "print_utilities.hh"
#include "rgb.hh"
#include "speed.hh"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Paint {

namespace {
namespace Bd = Builder;

struct Distance_map
{
  uint64_t max;
  std::unordered_map<Bd::Maze::Point, uint64_t> distances;
  Distance_map( Bd::Maze::Point p, const uint64_t dist ) : max( dist ), distances( { { p, dist } } ) {}
};

struct Point_dist
{
  Bd::Maze::Point p;
  uint64_t dist;
};

struct Bfs_monitor
{
  std::mutex monitor {};
  uint64_t count { 0 };
  std::vector<My_queue<Bd::Maze::Point>> paths;
  std::vector<std::unordered_set<Bd::Maze::Point>> seen;
  Bfs_monitor() : paths { num_painters }, seen { num_painters }
  {
    for ( My_queue<Bd::Maze::Point>& p : paths ) {
      p.reserve( initial_path_len );
    }
  }
};

struct Thread_guide
{
  uint64_t bias;
  uint64_t color_i;
  Speed::Speed_unit animation;
  Bd::Maze::Point p;
};

void painter( Bd::Maze& maze, const Distance_map& map )
{
  std::mt19937 rng( std::random_device {}() );
  std::uniform_int_distribution<int> uid( 0, 2 );
  const int rand_color_choice = uid( rng );
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      const Bd::Maze::Point cur = { row, col };
      const auto path_point = map.distances.find( cur );
      if ( path_point != map.distances.end() ) {
        const auto intensity = static_cast<double>( map.max - path_point->second ) / static_cast<double>( map.max );
        const auto dark = static_cast<uint8_t>( 255.0 * intensity );
        const auto bright = static_cast<uint8_t>( 128 ) + static_cast<uint8_t>( 127.0 * intensity );
        Rgb color { dark, dark, dark };
        color.at( rand_color_choice ) = bright;
        print_rgb( color, cur );
      } else {
        print_wall( maze, cur );
      }
    }
  }
  std::cout << "\n";
}

void painter_animated( Bd::Maze& maze, const Distance_map& map, Bfs_monitor& monitor, Thread_guide guide )
{
  My_queue<Bd::Maze::Point>& bfs = monitor.paths[guide.bias];
  std::unordered_set<Bd::Maze::Point>& seen = monitor.seen[guide.bias];
  bfs.push( guide.p );
  while ( !bfs.empty() ) {
    const Bd::Maze::Point cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( monitor.count == map.distances.size() ) {
      monitor.monitor.unlock();
      return;
    }
    if ( !( maze[cur.row][cur.col] & paint ) ) {
      const uint64_t dist = map.distances.at( cur );
      const auto intensity = static_cast<double>( map.max - dist ) / static_cast<double>( map.max );
      const auto dark = static_cast<uint8_t>( 255.0 * intensity );
      const auto bright = static_cast<uint8_t>( 128 ) + static_cast<uint8_t>( 127.0 * intensity );
      Rgb color { dark, dark, dark };
      color.at( guide.color_i ) = bright;
      animate_rgb( color, cur );
      maze[cur.row][cur.col] |= paint;
    }
    monitor.monitor.unlock();

    std::this_thread::sleep_for( std::chrono::microseconds( guide.animation ) );

    for ( uint64_t count = 0, i = guide.bias; count < Bd::Maze::dirs.size();
          count++, ++i %= Bd::Maze::dirs.size() ) {
      const Bd::Maze::Point& p = Bd::Maze::dirs.at( i );
      const Bd::Maze::Point next = { cur.row + p.row, cur.col + p.col };

      monitor.monitor.lock();
      const bool is_path = maze[next.row][next.col] & Bd::Maze::path_bit;
      monitor.monitor.unlock();

      if ( is_path && !seen.contains( next ) ) {
        bfs.push( next );
        seen.insert( next );
      }
    }
  }
}

} // namespace

void paint_distance_from_center( Bd::Maze& maze )
{
  const int row_mid = maze.row_size() / 2;
  const int col_mid = maze.col_size() / 2;
  const Bd::Maze::Point start = { row_mid + 1 - ( row_mid % 2 ), col_mid + 1 - ( col_mid % 2 ) };
  Distance_map map( start, 0 );
  My_queue<Point_dist> bfs;
  bfs.push( { start, 0 } );
  maze[start.row][start.col] |= measure;
  while ( !bfs.empty() ) {
    const Point_dist cur = bfs.front();
    bfs.pop();
    map.max = std::max( map.max, cur.dist );
    for ( const Bd::Maze::Point& p : Bd::Maze::dirs ) {
      const Bd::Maze::Point next = { cur.p.row + p.row, cur.p.col + p.col };
      if ( !( maze[next.row][next.col] & Bd::Maze::path_bit ) || ( maze[next.row][next.col] & measure ) ) {
        continue;
      }
      maze[next.row][next.col] |= measure;
      map.distances.insert( { next, cur.dist + 1 } );
      bfs.push( { next, cur.dist + 1 } );
    }
  }
  painter( maze, map );
  std::cout << "\n";
}

void animate_distance_from_center( Bd::Maze& maze, Speed::Speed speed )
{
  const int row_mid = maze.row_size() / 2;
  const int col_mid = maze.col_size() / 2;
  const Bd::Maze::Point start = { row_mid + 1 - ( row_mid % 2 ), col_mid + 1 - ( col_mid % 2 ) };
  Distance_map map( start, 0 );
  My_queue<Point_dist> bfs;
  bfs.push( { start, 0 } );
  maze[start.row][start.col] |= measure;
  while ( !bfs.empty() ) {
    const Point_dist cur = bfs.front();
    bfs.pop();
    map.max = std::max( map.max, cur.dist );
    for ( const Bd::Maze::Point& p : Bd::Maze::dirs ) {
      const Bd::Maze::Point next = { cur.p.row + p.row, cur.p.col + p.col };
      if ( !( maze[next.row][next.col] & Bd::Maze::path_bit ) || ( maze[next.row][next.col] & measure ) ) {
        continue;
      }
      maze[next.row][next.col] |= measure;
      map.distances.insert( { next, cur.dist + 1 } );
      bfs.push( { next, cur.dist + 1 } );
    }
  }

  std::mt19937 rng( std::random_device {}() );
  std::uniform_int_distribution<uint64_t> uid( 0, 2 );
  const uint64_t rand_color_choice = uid( rng );
  std::array<std::thread, num_painters> handles;
  const Speed::Speed_unit animation = animation_speeds.at( static_cast<uint64_t>( speed ) );
  Bfs_monitor monitor;
  for ( uint64_t i = 0; i < handles.size(); i++ ) {
    Thread_guide this_thread = { i, rand_color_choice, animation, start };
    handles.at( i )
      = std::thread( painter_animated, std::ref( maze ), std::ref( map ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : handles ) {
    t.join();
  }
  Printer::set_cursor_position( { maze.row_size(), maze.col_size() } );
  std::cout << "\n";
}

} // namespace Paint
