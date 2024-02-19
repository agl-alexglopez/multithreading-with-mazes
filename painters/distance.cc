module;
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
export module labyrinth:distance;
import :maze;
import :speed;
import :rgb;
import :my_queue;
import :printers;

/////////////////////////////////////   Exported Interface   /////////////////////////////////////

export namespace Distance {
void paint_distance_from_center( Maze::Maze& maze );
void animate_distance_from_center( Maze::Maze& maze, Speed::Speed speed );
} // namespace Distance

/////////////////////////////////////     Implementation     /////////////////////////////////////

namespace {

struct Distance_map
{
  uint64_t max;
  std::unordered_map<Maze::Point, uint64_t> distances;
  Distance_map( Maze::Point p, const uint64_t dist ) : max( dist ), distances( { { p, dist } } ) {}
};

struct Point_dist
{
  Maze::Point p;
  uint64_t dist;
};

void painter( Maze::Maze& maze, const Distance_map& map )
{
  std::mt19937 rng( std::random_device {}() );
  std::uniform_int_distribution<int> uid( 0, 2 );
  const int rand_color_choice = uid( rng );
  for ( int row = 0; row < maze.row_size(); row++ ) {
    for ( int col = 0; col < maze.col_size(); col++ ) {
      const Maze::Point cur = { row, col };
      const auto path_point = map.distances.find( cur );
      if ( path_point != map.distances.end() ) {
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

void painter_animated( Maze::Maze& maze,
                       const Distance_map& map,
                       Rgb::Bfs_monitor& monitor,
                       Rgb::Thread_guide guide )
{
  My_queue<Maze::Point>& bfs = monitor.paths[guide.bias];
  std::unordered_set<Maze::Point>& seen = monitor.seen[guide.bias];
  bfs.push( guide.p );
  while ( !bfs.empty() ) {
    const Maze::Point cur = bfs.front();
    bfs.pop();

    if ( monitor.count.load() >= map.distances.size() ) {
      return;
    }

    const uint16_t square = maze[cur.row][cur.col].load();
    const uint16_t painted = ( square | Rgb::paint );
    const uint16_t not_painted = square & static_cast<uint16_t>( ~Rgb::paint );
    if ( maze[cur.row][cur.col].ces( not_painted, painted ) ) {
      const uint64_t dist = map.distances.at( cur );
      const auto intensity = static_cast<double>( map.max - dist ) / static_cast<double>( map.max );
      const auto dark = static_cast<uint8_t>( 255.0 * intensity );
      const auto bright = static_cast<uint8_t>( 128 ) + static_cast<uint8_t>( 127.0 * intensity );
      Rgb::Rgb color { dark, dark, dark };
      color.at( guide.color_i ) = bright;

      monitor.monitor.lock();
      Rgb::animate_rgb( color, cur );
      monitor.monitor.unlock();

      ++monitor.count;
      std::this_thread::sleep_for( std::chrono::microseconds( guide.animation ) );
    }

    for ( uint64_t count = 0, i = guide.bias; count < Maze::dirs.size(); count++, ++i %= Maze::dirs.size() ) {
      const Maze::Point& p = Maze::dirs.at( i );
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };

      const bool is_path = ( maze[next.row][next.col] & Maze::path_bit ).load();

      if ( is_path && !seen.contains( next ) ) {
        bfs.push( next );
        seen.insert( next );
      }
    }
  }
}

} // namespace

namespace Distance {

void paint_distance_from_center( Maze::Maze& maze )
{
  const int row_mid = maze.row_size() / 2;
  const int col_mid = maze.col_size() / 2;
  const Maze::Point start = { row_mid + 1 - ( row_mid % 2 ), col_mid + 1 - ( col_mid % 2 ) };
  Distance_map map( start, 0 );
  My_queue<Point_dist> bfs;
  bfs.push( { start, 0 } );
  maze[start.row][start.col] |= Rgb::measure;
  while ( !bfs.empty() ) {
    const Point_dist cur = bfs.front();
    bfs.pop();
    map.max = std::max( map.max, cur.dist );
    for ( const Maze::Point& p : Maze::dirs ) {
      const Maze::Point next = { cur.p.row + p.row, cur.p.col + p.col };
      if ( !( maze[next.row][next.col] & Maze::path_bit ) || ( maze[next.row][next.col] & Rgb::measure ) ) {
        continue;
      }
      maze[next.row][next.col] |= Rgb::measure;
      map.distances.insert( { next, cur.dist + 1 } );
      bfs.push( { next, cur.dist + 1 } );
    }
  }
  painter( maze, map );
  std::cout << "\n";
}

void animate_distance_from_center( Maze::Maze& maze, Speed::Speed speed )
{
  const int row_mid = maze.row_size() / 2;
  const int col_mid = maze.col_size() / 2;
  const Maze::Point start = { row_mid + 1 - ( row_mid % 2 ), col_mid + 1 - ( col_mid % 2 ) };
  Distance_map map( start, 0 );
  My_queue<Point_dist> bfs;
  bfs.push( { start, 0 } );
  maze[start.row][start.col] |= Rgb::measure;
  while ( !bfs.empty() ) {
    const Point_dist cur = bfs.front();
    bfs.pop();
    map.max = std::max( map.max, cur.dist );
    for ( const Maze::Point& p : Maze::dirs ) {
      const Maze::Point next = { cur.p.row + p.row, cur.p.col + p.col };
      if ( !( maze[next.row][next.col] & Maze::path_bit ) || ( maze[next.row][next.col] & Rgb::measure ) ) {
        continue;
      }
      maze[next.row][next.col] |= Rgb::measure;
      map.distances.insert( { next, cur.dist + 1 } );
      bfs.push( { next, cur.dist + 1 } );
    }
  }

  std::mt19937 rng( std::random_device {}() );
  std::uniform_int_distribution<uint64_t> uid( 0, 2 );
  const uint64_t rand_color_choice = uid( rng );
  std::array<std::thread, Rgb::num_painters> handles;
  const Speed::Speed_unit animation = Rgb::animation_speeds.at( static_cast<uint64_t>( speed ) );
  Rgb::Bfs_monitor monitor;
  for ( uint64_t i = 0; i < handles.size(); i++ ) {
    const Rgb::Thread_guide this_thread = { i, rand_color_choice, animation, start };
    handles.at( i )
      = std::thread( painter_animated, std::ref( maze ), std::ref( map ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : handles ) {
    t.join();
  }
  Printer::set_cursor_position( { maze.row_size(), maze.col_size() } );
  std::cout << "\n";
}

} // namespace Distance
