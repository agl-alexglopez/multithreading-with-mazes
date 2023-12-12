module;
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>
export module labyrinth:bfs;
import :maze;
import :speed;
import :printers;
import :solve_utilities;
import :my_queue;

namespace {

struct Solver_monitor
{
  std::mutex monitor {};
  std::optional<Speed::Speed_unit> speed {};
  std::vector<std::unordered_map<Maze::Point, Maze::Point>> thread_maps;
  std::vector<My_queue<Maze::Point>> thread_queues;
  std::vector<Maze::Point> starts {};
  std::optional<int> winning_index {};
  std::vector<std::vector<Maze::Point>> thread_paths;
  Solver_monitor()
    : thread_maps { Sutil::num_threads }
    , thread_queues { Sutil::num_threads }
    , thread_paths { Sutil::num_threads, std::vector<Maze::Point> {} }
  {
    for ( std::vector<Maze::Point>& path : thread_paths ) {
      path.reserve( Sutil::initial_path_len );
    }
    for ( My_queue<Maze::Point>& q : thread_queues ) {
      q.reserve( Sutil::initial_path_len );
    }
  }
};

void hunter( Maze::Maze& maze, Solver_monitor& monitor, Sutil::Thread_id id )
{
  const Sutil::Thread_paint paint_bit = id.bit << Sutil::thread_paint_shift;
  // This will be how we rebuild the path because queue does not represent the current path.
  std::unordered_map<Maze::Point, Maze::Point>& seen = monitor.thread_maps[id.index];
  seen[monitor.starts.at( id.index )] = { -1, -1 };
  My_queue<Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Maze::Point cur = monitor.starts.at( id.index );
  while ( !bfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( monitor.winning_index ) {
      break;
    }

    cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & Sutil::finish_bit ) {
      if ( !monitor.winning_index ) {
        monitor.winning_index = id.index;
      }
      monitor.monitor.unlock();
      break;
    }
    // This creates a nice fanning out of mixed color for each searching thread.
    maze[cur.row][cur.col] |= paint_bit;
    monitor.monitor.unlock();

    // Bias each thread towards the direction it was dispatched when we first sent it.
    for ( uint64_t count = 0, i = id.index; count < Sutil::dirs.size(); count++, ++i %= Sutil::dirs.size() ) {
      const Maze::Point& p = Sutil::dirs.at( i );
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };
      const bool seen_next = seen.contains( next );

      monitor.monitor.lock();
      const bool push_next = !seen_next && ( maze[next.row][next.col] & Maze::path_bit );
      monitor.monitor.unlock();

      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
    }
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    monitor.thread_paths[id.index].push_back( cur );
    cur = seen.at( cur );
  }
}

void animate_hunter( Maze::Maze& maze, Solver_monitor& monitor, Sutil::Thread_id id )
{
  const Sutil::Thread_paint paint_bit = id.bit << Sutil::thread_paint_shift;
  // This will be how we rebuild the path because queue does not represent the current path.
  std::unordered_map<Maze::Point, Maze::Point>& seen = monitor.thread_maps[id.index];
  seen[monitor.starts.at( id.index )] = { -1, -1 };
  My_queue<Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Maze::Point cur = monitor.starts.at( id.index );
  while ( !bfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( monitor.winning_index ) {
      break;
    }

    cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & Sutil::finish_bit ) {
      if ( !monitor.winning_index ) {
        monitor.winning_index = id.index;
      }
      monitor.monitor.unlock();
      break;
    }
    // This creates a nice fanning out of mixed color for each searching thread.
    maze[cur.row][cur.col] |= paint_bit;
    Sutil::flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

    // Bias each thread towards the direction it was dispatched when we first sent it.
    for ( uint64_t count = 0, i = id.index; count < Sutil::dirs.size(); count++, ++i %= Sutil::dirs.size() ) {
      const Maze::Point& p = Sutil::dirs.at( i );
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };
      const bool seen_next = seen.contains( next );

      monitor.monitor.lock();
      const bool push_next = !seen_next && ( maze[next.row][next.col] & Maze::path_bit );
      monitor.monitor.unlock();

      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
    }
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    monitor.thread_paths[id.index].push_back( cur );
    cur = seen.at( cur );
  }
}

void gatherer( Maze::Maze& maze, Solver_monitor& monitor, Sutil::Thread_id id )
{
  std::unordered_map<Maze::Point, Maze::Point>& seen = monitor.thread_maps[id.index];
  const Sutil::Thread_cache seen_bit = id.bit << Sutil::thread_cache_shift;
  const Sutil::Thread_paint paint_bit = id.bit << Sutil::thread_paint_shift;
  seen[monitor.starts[id.index]] = { -1, -1 };
  My_queue<Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Maze::Point cur = monitor.starts.at( id.index );
  while ( !bfs.empty() ) {
    cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & Sutil::finish_bit && !( maze[cur.row][cur.col] & Sutil::cache_mask ) ) {
      maze[cur.row][cur.col] |= seen_bit;
      monitor.monitor.unlock();
      break;
    }
    maze[cur.row][cur.col] |= paint_bit;
    maze[cur.row][cur.col] |= seen_bit;
    monitor.monitor.unlock();

    for ( uint64_t count = 0, i = id.index; count < Sutil::dirs.size(); count++, ++i %= Sutil::dirs.size() ) {
      const Maze::Point& p = Sutil::dirs.at( i );
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };
      const bool seen_next = seen.contains( next );
      monitor.monitor.lock();
      const bool push_next = !seen_next && ( maze[next.row][next.col] & Maze::path_bit );
      monitor.monitor.unlock();
      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
    }
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    monitor.thread_paths[id.index].push_back( cur );
    cur = seen.at( cur );
  }
  monitor.winning_index = id.index;
}

void animate_gatherer( Maze::Maze& maze, Solver_monitor& monitor, Sutil::Thread_id id )
{
  std::unordered_map<Maze::Point, Maze::Point>& seen = monitor.thread_maps[id.index];
  const Sutil::Thread_cache seen_bit = id.bit << Sutil::thread_cache_shift;
  const Sutil::Thread_cache paint_bit = id.bit << Sutil::thread_paint_shift;
  seen[monitor.starts[id.index]] = { -1, -1 };
  My_queue<Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Maze::Point cur = monitor.starts.at( id.index );
  while ( !bfs.empty() ) {
    cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & Sutil::finish_bit && !( maze[cur.row][cur.col] & Sutil::cache_mask ) ) {
      maze[cur.row][cur.col] |= seen_bit;
      monitor.monitor.unlock();
      break;
    }
    maze[cur.row][cur.col] |= paint_bit;
    maze[cur.row][cur.col] |= seen_bit;
    Sutil::flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

    for ( uint64_t count = 0, i = id.index; count < Sutil::dirs.size(); count++, ++i %= Sutil::dirs.size() ) {
      const Maze::Point& p = Sutil::dirs.at( i );
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };
      const bool seen_next = seen.contains( next );
      monitor.monitor.lock();
      const bool push_next = !seen_next && ( maze[next.row][next.col] & Maze::path_bit );
      monitor.monitor.unlock();
      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
    }
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    monitor.thread_paths[id.index].push_back( cur );
    cur = seen.at( cur );
  }
  monitor.winning_index = id.index;
}

} // namespace

/* * * * * * * * * * * *  Multithreaded Dispatcher Functions from Header Interface   * * * * * * * * * * * * * * */

export namespace Bfs {

void hunt( Maze::Maze& maze )
{
  Solver_monitor monitor;
  monitor.starts = std::vector<Maze::Point>( Sutil::num_threads, Sutil::pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= Sutil::start_bit;
  const Maze::Point finish = Sutil::pick_random_point( maze );
  maze[finish.row][finish.col] |= Sutil::finish_bit;
  std::vector<std::thread> threads( Sutil::num_threads );
  for ( int i_thread = 0; i_thread < Sutil::num_threads; i_thread++ ) {
    const Sutil::Thread_id this_thread { i_thread, Sutil::thread_bits.at( i_thread ) };
    threads[i_thread] = std::thread( hunter, std::ref( maze ), std::ref( monitor ), this_thread );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }

  if ( monitor.winning_index ) {
    // It is cool to see the shortest path that the winning thread took to victory
    const Sutil::Thread_paint winner_color
      = ( Sutil::thread_bits.at( monitor.winning_index.value() ) << Sutil::thread_paint_shift );
    for ( const Maze::Point& p : monitor.thread_paths.at( monitor.winning_index.value() ) ) {
      maze[p.row][p.col] &= static_cast<Sutil::Thread_paint>( ~Sutil::thread_paint_mask );
      maze[p.row][p.col] |= winner_color;
    }
  }

  Sutil::print_maze( maze );
  Sutil::print_overlap_key();
  Sutil::print_hunt_solution_message( monitor.winning_index );
  std::cout << "\n";
}

void animate_hunt( Maze::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  Sutil::print_overlap_key();
  Solver_monitor monitor;
  monitor.speed = Sutil::solver_speeds.at( static_cast<Speed::Speed_unit>( speed ) );
  monitor.starts = std::vector<Maze::Point>( Sutil::num_threads, Sutil::pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= Sutil::start_bit;
  const Maze::Point finish = Sutil::pick_random_point( maze );
  maze[finish.row][finish.col] |= Sutil::finish_bit;
  Sutil::flush_cursor_path_coordinate( maze, finish );
  std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

  std::vector<std::thread> threads( Sutil::num_threads );
  for ( int i_thread = 0; i_thread < Sutil::num_threads; i_thread++ ) {
    const Sutil::Thread_id this_thread { i_thread, Sutil::thread_bits.at( i_thread ) };
    threads[i_thread] = std::thread( animate_hunter, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }

  if ( monitor.winning_index ) {
    // It is cool to see the shortest path that the winning thread took to victory
    const Sutil::Thread_paint winner_color
      = ( Sutil::thread_bits.at( monitor.winning_index.value() ) << Sutil::thread_paint_shift );
    for ( const Maze::Point& p : monitor.thread_paths.at( monitor.winning_index.value() ) ) {
      maze[p.row][p.col] &= static_cast<Sutil::Thread_paint>( ~Sutil::thread_paint_mask );
      maze[p.row][p.col] |= winner_color;
      Sutil::flush_cursor_path_coordinate( maze, p );
      std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
    }
  }

  Printer::set_cursor_position( { maze.row_size() + Sutil::overlap_key_and_message_height, 0 } );
  Sutil::print_hunt_solution_message( monitor.winning_index );
  std::cout << "\n";
}

void gather( Maze::Maze& maze )
{
  Solver_monitor monitor;
  monitor.starts = std::vector<Maze::Point>( Sutil::num_threads, Sutil::pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= Sutil::start_bit;
  for ( int finish_square = 0; finish_square < Sutil::num_gather_finishes; finish_square++ ) {
    const Maze::Point finish = Sutil::pick_random_point( maze );
    maze[finish.row][finish.col] |= Sutil::finish_bit;
  }
  std::vector<std::thread> threads( Sutil::num_threads );
  for ( int i_thread = 0; i_thread < Sutil::num_threads; i_thread++ ) {
    const Sutil::Thread_id this_thread { i_thread, Sutil::thread_bits.at( i_thread ) };
    threads[i_thread] = std::thread( gatherer, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
  int thread = 0;
  for ( const std::vector<Maze::Point>& path : monitor.thread_paths ) {
    const Sutil::Thread_paint color = ( Sutil::thread_bits.at( thread ) << Sutil::thread_paint_shift );
    const Maze::Point& p = path.front();
    maze[p.row][p.col] &= static_cast<Sutil::Thread_paint>( ~Sutil::thread_paint_mask );
    maze[p.row][p.col] |= color;
    thread++;
  }
  Sutil::print_maze( maze );
  Sutil::print_overlap_key();
  Sutil::print_gather_solution_message();
  std::cout << "\n";
}

void animate_gather( Maze::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  Sutil::print_overlap_key();
  Solver_monitor monitor;
  monitor.speed = Sutil::solver_speeds.at( static_cast<int>( speed ) );
  monitor.starts = std::vector<Maze::Point>( Sutil::num_threads, Sutil::pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= Sutil::start_bit;
  for ( int finish_square = 0; finish_square < Sutil::num_gather_finishes; finish_square++ ) {
    const Maze::Point finish = Sutil::pick_random_point( maze );
    maze[finish.row][finish.col] |= Sutil::finish_bit;
    Sutil::flush_cursor_path_coordinate( maze, finish );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
  }

  std::vector<std::thread> threads( Sutil::num_threads );
  for ( int i_thread = 0; i_thread < Sutil::num_threads; i_thread++ ) {
    const Sutil::Thread_id this_thread { i_thread, Sutil::thread_bits.at( i_thread ) };
    threads[i_thread] = std::thread( animate_gatherer, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }

  int i_thread = 0;
  for ( const std::vector<Maze::Point>& path : monitor.thread_paths ) {
    const Sutil::Thread_paint color = ( Sutil::thread_bits.at( i_thread ) << Sutil::thread_paint_shift );
    const Maze::Point& p = path.front();
    maze[p.row][p.col] &= static_cast<Sutil::Thread_paint>( ~Sutil::thread_paint_mask );
    maze[p.row][p.col] |= color;
    Sutil::flush_cursor_path_coordinate( maze, p );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
    ++i_thread;
  }
  Printer::set_cursor_position( { maze.row_size() + Sutil::overlap_key_and_message_height, 0 } );
  Sutil::print_gather_solution_message();
  std::cout << "\n";
}

void corners( Maze::Maze& maze )
{
  Solver_monitor monitor;
  monitor.starts = Sutil::set_corner_starts( maze );
  for ( const Maze::Point& p : monitor.starts ) {
    maze[p.row][p.col] |= Sutil::start_bit;
  }
  const Maze::Point finish = { maze.row_size() / 2, maze.col_size() / 2 };
  for ( const Maze::Point& p : Sutil::dirs ) {
    const Maze::Point next = { finish.row + p.row, finish.col + p.col };
    maze[next.row][next.col] |= Maze::path_bit;
  }
  maze[finish.row][finish.col] |= Maze::path_bit;
  maze[finish.row][finish.col] |= Sutil::finish_bit;

  std::vector<std::thread> threads( Sutil::num_threads );
  // Randomly shuffle thread start corners so colors mix differently each time.
  shuffle( begin( monitor.starts ), end( monitor.starts ), std::mt19937( std::random_device {}() ) );
  for ( int i_thread = 0; i_thread < Sutil::num_threads; i_thread++ ) {
    const Sutil::Thread_id this_thread = { i_thread, Sutil::thread_bits.at( i_thread ) };
    threads[i_thread] = std::thread( hunter, std::ref( maze ), std::ref( monitor ), this_thread );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }
  Sutil::print_maze( maze );
  Sutil::print_overlap_key();
  Sutil::print_hunt_solution_message( monitor.winning_index );
  std::cout << "\n";
}

void animate_corners( Maze::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  Sutil::print_overlap_key();
  Solver_monitor monitor;
  monitor.speed = Sutil::solver_speeds.at( static_cast<int>( speed ) );
  monitor.starts = Sutil::set_corner_starts( maze );
  for ( const Maze::Point& p : monitor.starts ) {
    maze[p.row][p.col] |= Sutil::start_bit;
    Sutil::flush_cursor_path_coordinate( maze, p );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
  }
  const Maze::Point finish = { maze.row_size() / 2, maze.col_size() / 2 };
  for ( const Maze::Point& p : Sutil::all_dirs ) {
    const Maze::Point next = { finish.row + p.row, finish.col + p.col };
    maze[next.row][next.col] |= Maze::path_bit;
    Sutil::flush_cursor_path_coordinate( maze, next );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
  }
  maze[finish.row][finish.col] |= Maze::path_bit;
  maze[finish.row][finish.col] |= Sutil::finish_bit;
  Sutil::flush_cursor_path_coordinate( maze, finish );
  std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

  std::vector<std::thread> threads( Sutil::num_threads );
  // Randomly shuffle thread start corners so colors mix differently each time.
  shuffle( begin( monitor.starts ), end( monitor.starts ), std::mt19937( std::random_device {}() ) );
  for ( int i_thread = 0; i_thread < Sutil::num_threads; i_thread++ ) {
    const Sutil::Thread_id this_thread = { i_thread, Sutil::thread_bits.at( i_thread ) };
    threads[i_thread] = std::thread( animate_hunter, std::ref( maze ), std::ref( monitor ), this_thread );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }

  if ( monitor.winning_index ) {
    // It is cool to see the shortest path that the winning thread took to victory
    const Sutil::Thread_paint winner_color
      = ( Sutil::thread_bits.at( monitor.winning_index.value() ) << Sutil::thread_paint_shift );
    for ( const Maze::Point& p : monitor.thread_paths.at( monitor.winning_index.value() ) ) {
      maze[p.row][p.col] &= static_cast<Sutil::Thread_paint>( ~Sutil::thread_paint_mask );
      maze[p.row][p.col] |= winner_color;
      Sutil::flush_cursor_path_coordinate( maze, p );
      std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
    }
  }

  Printer::set_cursor_position( { maze.row_size() + Sutil::overlap_key_and_message_height, 0 } );
  Sutil::print_hunt_solution_message( monitor.winning_index );
  std::cout << "\n";
}

} // namespace Bfs
