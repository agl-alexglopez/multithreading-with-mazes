#include "maze_solvers.hh"
#include "my_queue.hh"
#include "print_utilities.hh"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Solver {

namespace {

struct Solver_monitor
{
  std::mutex monitor {};
  std::optional<Speed_unit> speed {};
  std::vector<std::unordered_map<Builder::Maze::Point, Builder::Maze::Point>> thread_maps;
  std::vector<My_queue<Builder::Maze::Point>> thread_queues;
  std::vector<Builder::Maze::Point> starts {};
  std::optional<int> winning_index {};
  std::vector<std::vector<Builder::Maze::Point>> thread_paths;
  Solver_monitor()
    : thread_maps { num_threads_ }
    , thread_queues { num_threads_ }
    , thread_paths { num_threads_, std::vector<Builder::Maze::Point> {} }
  {
    for ( std::vector<Builder::Maze::Point>& path : thread_paths ) {
      path.reserve( initial_path_len_ );
    }
    for ( My_queue<Builder::Maze::Point>& q : thread_queues ) {
      q.reserve( initial_path_len_ );
    }
  }
};

void complete_hunt( Builder::Maze& maze, Solver_monitor& monitor, Thread_id id )
{
  // This will be how we rebuild the path because queue does not represent the current path.
  std::unordered_map<Builder::Maze::Point, Builder::Maze::Point>& seen = monitor.thread_maps[id.index];
  seen[monitor.starts.at( id.index )] = { -1, -1 };
  My_queue<Builder::Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Builder::Maze::Point cur = monitor.starts.at( id.index );
  while ( !bfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( monitor.winning_index ) {
      break;
    }

    cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & finish_bit_ ) {
      if ( !monitor.winning_index ) {
        monitor.winning_index = id.index;
      }
      monitor.monitor.unlock();
      break;
    }
    // This creates a nice fanning out of mixed color for each searching thread.
    maze[cur.row][cur.col] |= id.paint;
    monitor.monitor.unlock();

    // Bias each thread towards the direction it was dispatched when we first sent it.
    for ( uint64_t count = 0, i = id.index; count < n_e_s_w_.size(); count++, ++i %= n_e_s_w_.size() ) {
      const Builder::Maze::Point& p = n_e_s_w_.at( i );
      const Builder::Maze::Point next = { cur.row + p.row, cur.col + p.col };
      const bool seen_next = seen.contains( next );

      monitor.monitor.lock();
      const bool push_next = !seen_next && ( maze[next.row][next.col] & Builder::Maze::path_bit_ );
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

void animate_hunt( Builder::Maze& maze, Solver_monitor& monitor, Thread_id id )
{
  // This will be how we rebuild the path because queue does not represent the current path.
  std::unordered_map<Builder::Maze::Point, Builder::Maze::Point>& seen = monitor.thread_maps[id.index];
  seen[monitor.starts.at( id.index )] = { -1, -1 };
  My_queue<Builder::Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Builder::Maze::Point cur = monitor.starts.at( id.index );
  while ( !bfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( monitor.winning_index ) {
      break;
    }

    cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & finish_bit_ ) {
      if ( !monitor.winning_index ) {
        monitor.winning_index = id.index;
      }
      monitor.monitor.unlock();
      break;
    }
    // This creates a nice fanning out of mixed color for each searching thread.
    maze[cur.row][cur.col] |= id.paint;
    flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

    // Bias each thread towards the direction it was dispatched when we first sent it.
    for ( uint64_t count = 0, i = id.index; count < n_e_s_w_.size(); count++, ++i %= n_e_s_w_.size() ) {
      const Builder::Maze::Point& p = n_e_s_w_.at( i );
      const Builder::Maze::Point next = { cur.row + p.row, cur.col + p.col };
      const bool seen_next = seen.contains( next );

      monitor.monitor.lock();
      const bool push_next = !seen_next && ( maze[next.row][next.col] & Builder::Maze::path_bit_ );
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

void complete_gather( Builder::Maze& maze, Solver_monitor& monitor, Thread_id id )
{
  std::unordered_map<Builder::Maze::Point, Builder::Maze::Point>& seen = monitor.thread_maps[id.index];
  const Thread_cache seen_bit = id.paint << 4;
  seen[monitor.starts[id.index]] = { -1, -1 };
  My_queue<Builder::Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Builder::Maze::Point cur = monitor.starts.at( id.index );
  while ( !bfs.empty() ) {
    cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & finish_bit_ && !( maze[cur.row][cur.col] & cache_mask_ ) ) {
      maze[cur.row][cur.col] |= seen_bit;
      monitor.monitor.unlock();
      break;
    }
    maze[cur.row][cur.col] |= id.paint;
    maze[cur.row][cur.col] |= seen_bit;
    monitor.monitor.unlock();

    for ( uint64_t count = 0, i = id.index; count < n_e_s_w_.size(); count++, ++i %= n_e_s_w_.size() ) {
      const Builder::Maze::Point& p = n_e_s_w_.at( i );
      const Builder::Maze::Point next = { cur.row + p.row, cur.col + p.col };
      const bool seen_next = seen.contains( next );
      monitor.monitor.lock();
      const bool push_next = !seen_next && ( maze[next.row][next.col] & Builder::Maze::path_bit_ );
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

void animate_gather( Builder::Maze& maze, Solver_monitor& monitor, Thread_id id )
{
  std::unordered_map<Builder::Maze::Point, Builder::Maze::Point>& seen = monitor.thread_maps[id.index];
  const Thread_cache seen_bit = id.paint << 4;
  seen[monitor.starts[id.index]] = { -1, -1 };
  My_queue<Builder::Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Builder::Maze::Point cur = monitor.starts.at( id.index );
  while ( !bfs.empty() ) {
    cur = bfs.front();
    bfs.pop();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & finish_bit_ && !( maze[cur.row][cur.col] & cache_mask_ ) ) {
      maze[cur.row][cur.col] |= seen_bit;
      monitor.monitor.unlock();
      break;
    }
    maze[cur.row][cur.col] |= id.paint;
    maze[cur.row][cur.col] |= seen_bit;
    flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

    for ( uint64_t count = 0, i = id.index; count < n_e_s_w_.size(); count++, ++i %= n_e_s_w_.size() ) {
      const Builder::Maze::Point& p = n_e_s_w_.at( i );
      const Builder::Maze::Point next = { cur.row + p.row, cur.col + p.col };
      const bool seen_next = seen.contains( next );
      monitor.monitor.lock();
      const bool push_next = !seen_next && ( maze[next.row][next.col] & Builder::Maze::path_bit_ );
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

void solve_with_bfs_thread_hunt( Builder::Maze& maze )
{
  Solver_monitor monitor;
  monitor.starts = std::vector<Builder::Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  const Builder::Maze::Point finish = pick_random_point( maze );
  maze[finish.row][finish.col] |= finish_bit_;
  std::vector<std::thread> threads( num_threads_ );
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_id this_thread { i_thread, thread_masks_.at( i_thread ) };
    threads[i_thread] = std::thread( complete_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }

  if ( monitor.winning_index ) {
    // It is cool to see the shortest path that the winning thread took to victory
    const Thread_paint winner_color = thread_masks_.at( monitor.winning_index.value() );
    for ( const Builder::Maze::Point& p : monitor.thread_paths.at( monitor.winning_index.value() ) ) {
      maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
      maze[p.row][p.col] |= winner_color;
    }
  }

  print_maze( maze );
  print_overlap_key();
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}

void animate_with_bfs_thread_hunt( Builder::Maze& maze, Solver_speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  print_overlap_key();
  Solver_monitor monitor;
  monitor.speed = solver_speeds_.at( static_cast<Speed_unit>( speed ) );
  monitor.starts = std::vector<Builder::Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  const Builder::Maze::Point finish = pick_random_point( maze );
  maze[finish.row][finish.col] |= finish_bit_;
  flush_cursor_path_coordinate( maze, finish );
  std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

  std::vector<std::thread> threads( num_threads_ );
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_id this_thread { i_thread, thread_masks_.at( i_thread ) };
    threads[i_thread] = std::thread( animate_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }

  if ( monitor.winning_index ) {
    // It is cool to see the shortest path that the winning thread took to victory
    const Thread_paint winner_color = thread_masks_.at( monitor.winning_index.value() );
    for ( const Builder::Maze::Point& p : monitor.thread_paths.at( monitor.winning_index.value() ) ) {
      maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
      maze[p.row][p.col] |= winner_color;
      flush_cursor_path_coordinate( maze, p );
      std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
    }
  }

  Printer::set_cursor_position( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}

void solve_with_bfs_thread_gather( Builder::Maze& maze )
{
  Solver_monitor monitor;
  monitor.starts = std::vector<Builder::Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  for ( int finish_square = 0; finish_square < num_gather_finishes_; finish_square++ ) {
    const Builder::Maze::Point finish = pick_random_point( maze );
    maze[finish.row][finish.col] |= finish_bit_;
  }
  std::vector<std::thread> threads( num_threads_ );
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_id this_thread { i_thread, thread_masks_.at( i_thread ) };
    threads[i_thread] = std::thread( complete_gather, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
  int thread = 0;
  for ( const std::vector<Builder::Maze::Point>& path : monitor.thread_paths ) {
    const Thread_paint color = thread_masks_.at( thread );
    for ( const Builder::Maze::Point& p : path ) {
      maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
      maze[p.row][p.col] |= color;
    }
    thread++;
  }
  print_maze( maze );
  print_overlap_key();
  print_gather_solution_message();
  std::cout << std::endl;
}

void animate_with_bfs_thread_gather( Builder::Maze& maze, Solver_speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  print_overlap_key();
  Solver_monitor monitor;
  monitor.speed = solver_speeds_.at( static_cast<int>( speed ) );
  monitor.starts = std::vector<Builder::Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  for ( int finish_square = 0; finish_square < num_gather_finishes_; finish_square++ ) {
    const Builder::Maze::Point finish = pick_random_point( maze );
    maze[finish.row][finish.col] |= finish_bit_;
    flush_cursor_path_coordinate( maze, finish );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
  }

  std::vector<std::thread> threads( num_threads_ );
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_id this_thread { i_thread, thread_masks_.at( i_thread ) };
    threads[i_thread] = std::thread( animate_gather, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }

  int i_thread = 0;
  for ( const std::vector<Builder::Maze::Point>& path : monitor.thread_paths ) {
    const Thread_paint color = thread_masks_.at( i_thread++ );
    const Builder::Maze::Point& p = path.front();
    maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
    maze[p.row][p.col] |= color;
    flush_cursor_path_coordinate( maze, p );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
  }
  Printer::set_cursor_position( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_gather_solution_message();
  std::cout << std::endl;
}

void solve_with_bfs_thread_corners( Builder::Maze& maze )
{
  Solver_monitor monitor;
  monitor.starts = set_corner_starts( maze );
  for ( const Builder::Maze::Point& p : monitor.starts ) {
    maze[p.row][p.col] |= start_bit_;
  }
  const Builder::Maze::Point finish = { maze.row_size() / 2, maze.col_size() / 2 };
  for ( const Builder::Maze::Point& p : all_directions_ ) {
    const Builder::Maze::Point next = { finish.row + p.row, finish.col + p.col };
    maze[next.row][next.col] |= Builder::Maze::path_bit_;
  }
  maze[finish.row][finish.col] |= Builder::Maze::path_bit_;
  maze[finish.row][finish.col] |= finish_bit_;

  std::vector<std::thread> threads( num_threads_ );
  // Randomly shuffle thread start corners so colors mix differently each time.
  shuffle( begin( monitor.starts ), end( monitor.starts ), std::mt19937( std::random_device {}() ) );
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_id this_thread = { i_thread, thread_masks_.at( i_thread ) };
    threads[i_thread] = std::thread( complete_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }
  print_maze( maze );
  print_overlap_key();
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}

void animate_with_bfs_thread_corners( Builder::Maze& maze, Solver_speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  print_overlap_key();
  Solver_monitor monitor;
  monitor.speed = solver_speeds_.at( static_cast<int>( speed ) );
  monitor.starts = set_corner_starts( maze );
  for ( const Builder::Maze::Point& p : monitor.starts ) {
    maze[p.row][p.col] |= start_bit_;
    flush_cursor_path_coordinate( maze, p );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
  }
  const Builder::Maze::Point finish = { maze.row_size() / 2, maze.col_size() / 2 };
  for ( const Builder::Maze::Point& p : all_directions_ ) {
    const Builder::Maze::Point next = { finish.row + p.row, finish.col + p.col };
    maze[next.row][next.col] |= Builder::Maze::path_bit_;
    flush_cursor_path_coordinate( maze, next );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
  }
  maze[finish.row][finish.col] |= Builder::Maze::path_bit_;
  maze[finish.row][finish.col] |= finish_bit_;
  flush_cursor_path_coordinate( maze, finish );
  std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

  std::vector<std::thread> threads( num_threads_ );
  // Randomly shuffle thread start corners so colors mix differently each time.
  shuffle( begin( monitor.starts ), end( monitor.starts ), std::mt19937( std::random_device {}() ) );
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_id this_thread = { i_thread, thread_masks_.at( i_thread ) };
    threads[i_thread] = std::thread( animate_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }

  if ( monitor.winning_index ) {
    // It is cool to see the shortest path that the winning thread took to victory
    const Thread_paint winner_color = thread_masks_.at( monitor.winning_index.value() );
    for ( const Builder::Maze::Point& p : monitor.thread_paths.at( monitor.winning_index.value() ) ) {
      maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
      maze[p.row][p.col] |= winner_color;
      flush_cursor_path_coordinate( maze, p );
      std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
    }
  }

  Printer::set_cursor_position( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}

} // namespace Solver
