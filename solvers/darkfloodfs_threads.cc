#include "maze.hh"
#include "maze_solvers.hh"
#include "print_utilities.hh"
#include "solver_utilities.hh"
#include "speed.hh"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <vector>

namespace Solver {

/* * * * * * * * * * * *   Flood First Search Solving Algorithms and Helper Types    * * * * * * * * * * * * * * */

namespace {

struct Solver_monitor
{
  std::mutex monitor {};
  std::optional<Speed::Speed_unit> speed {};
  std::vector<Builder::Maze::Point> starts {};
  std::optional<int> winning_index {};
  std::vector<std::vector<Builder::Maze::Point>> thread_paths;
  Solver_monitor() : thread_paths { num_threads_, std::vector<Builder::Maze::Point> {} }
  {
    for ( std::vector<Builder::Maze::Point>& path : thread_paths ) {
      path.reserve( initial_path_len_ );
    }
  }
};

void animate_hunt( Builder::Maze& maze, Solver_monitor& monitor, Thread_id id )
{
  const Thread_cache seen = id.bit << thread_cache_shift_;
  const Thread_paint paint_bit = id.bit << thread_paint_shift_;
  std::vector<Builder::Maze::Point>& dfs = monitor.thread_paths.at( id.index );
  dfs.push_back( monitor.starts.at( id.index ) );
  Builder::Maze::Point cur = monitor.starts.at( id.index );
  while ( !dfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( monitor.winning_index ) {
      return;
    }

    // Don't pop() yet!
    cur = dfs.back();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & finish_bit_ ) {
      if ( !monitor.winning_index ) {
        flush_cursor_path_coordinate( maze, cur );
        monitor.winning_index = id.index;
      }
      monitor.monitor.unlock();
      dfs.pop_back();
      return;
    }
    maze[cur.row][cur.col] |= seen;
    maze[cur.row][cur.col] |= paint_bit;
    flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

    // Bias each thread's first choice towards orginal dispatch direction. More coverage.
    bool found_branch_to_explore = false;
    for ( uint64_t count = 0, i = id.index; count < dirs_.size(); count++, ++i %= dirs_.size() ) {
      const Builder::Maze::Point& p = dirs_.at( i );
      const Builder::Maze::Point next = { cur.row + p.row, cur.col + p.col };
      monitor.monitor.lock();
      const bool push_next
        = !( maze[next.row][next.col] & seen ) && ( maze[next.row][next.col] & Builder::Maze::path_bit_ );
      monitor.monitor.unlock();
      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
    }

    if ( !found_branch_to_explore ) {
      dfs.pop_back();
    }
  }
}

void animate_gather( Builder::Maze& maze, Solver_monitor& monitor, Thread_id id )
{
  const Thread_cache seen = id.bit << thread_cache_shift_;
  const Thread_paint paint_bit = id.bit << thread_paint_shift_;
  std::vector<Builder::Maze::Point>& dfs = monitor.thread_paths.at( id.index );
  dfs.push_back( monitor.starts.at( id.index ) );
  Builder::Maze::Point cur = monitor.starts.at( id.index );
  while ( !dfs.empty() ) {
    cur = dfs.back();

    monitor.monitor.lock();
    if ( ( maze[cur.row][cur.col] & finish_bit_ ) && !( maze[cur.row][cur.col] & cache_mask_ ) ) {
      maze[cur.row][cur.col] |= seen;
      flush_cursor_path_coordinate( maze, cur );
      monitor.monitor.unlock();
      dfs.pop_back();
      return;
    }

    maze[cur.row][cur.col] |= seen;
    maze[cur.row][cur.col] |= paint_bit;
    flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

    bool found_branch_to_explore = false;
    for ( uint64_t count = 0, i = id.index; count < dirs_.size(); count++, ++i %= dirs_.size() ) {
      const Builder::Maze::Point& p = dirs_.at( i );
      const Builder::Maze::Point next = { cur.row + p.row, cur.col + p.col };
      monitor.monitor.lock();
      const bool push_next
        = !( maze[next.row][next.col] & seen ) && ( maze[next.row][next.col] & Builder::Maze::path_bit_ );
      monitor.monitor.unlock();
      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
    }

    if ( !found_branch_to_explore ) {
      dfs.pop_back();
    }
  }
}

} // namespace

/* * * * * * * * * * * *  Multithreaded Dispatcher Functions from Header Interface   * * * * * * * * * * * * * * */

void animate_darkfloodfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  print_overlap_key();
  deluminate_maze( maze );
  Solver_monitor monitor;
  monitor.speed = solver_speeds_.at( static_cast<int>( speed ) );
  monitor.starts = std::vector<Builder::Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  const Builder::Maze::Point finish = pick_random_point( maze );
  maze[finish.row][finish.col] |= finish_bit_;

  std::vector<std::thread> threads( num_threads_ );
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_id this_thread { i_thread, thread_bits_.at( i_thread ) };
    threads[i_thread] = std::thread( animate_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }

  if ( monitor.winning_index ) {
    const Thread_paint winner_color = ( thread_bits_.at( monitor.winning_index.value() ) << thread_paint_shift_ );
    const Builder::Maze::Point& before_finish = monitor.thread_paths.at( monitor.winning_index.value() ).back();
    maze[before_finish.row][before_finish.col] |= winner_color;
    flush_cursor_path_coordinate( maze, before_finish );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
  }
  Printer::set_cursor_position( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_hunt_solution_message( monitor.winning_index );
  std::cout << "\n";
}

void animate_darkfloodfs_thread_gather( Builder::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  print_overlap_key();
  deluminate_maze( maze );
  Solver_monitor monitor;
  monitor.speed = solver_speeds_.at( static_cast<int>( speed ) );
  monitor.starts = std::vector<Builder::Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  for ( int finish_square = 0; finish_square < num_gather_finishes_; finish_square++ ) {
    const Builder::Maze::Point finish = pick_random_point( maze );
    maze[finish.row][finish.col] |= finish_bit_;
  }

  std::vector<std::thread> threads( num_threads_ );
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_id this_thread { i_thread, thread_bits_.at( i_thread ) };
    threads[i_thread] = std::thread( animate_gather, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }

  int i_thread = 0;
  for ( const std::vector<Builder::Maze::Point>& path : monitor.thread_paths ) {
    const Thread_paint color = ( thread_bits_.at( i_thread ) << thread_paint_shift_ );
    const Builder::Maze::Point& p = path.back();
    maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_paint_mask_ );
    maze[p.row][p.col] |= color;
    flush_cursor_path_coordinate( maze, p );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
    ++i_thread;
  }
  Printer::set_cursor_position( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_gather_solution_message();
  std::cout << "\n";
}

void animate_darkfloodfs_thread_corners( Builder::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  print_overlap_key();
  deluminate_maze( maze );
  Solver_monitor monitor;
  monitor.speed = solver_speeds_.at( static_cast<int>( speed ) );
  monitor.starts = set_corner_starts( maze );
  for ( const Builder::Maze::Point& p : monitor.starts ) {
    maze[p.row][p.col] |= start_bit_;
  }
  const Builder::Maze::Point finish = { maze.row_size() / 2, maze.col_size() / 2 };
  for ( const Builder::Maze::Point& p : all_dirs_ ) {
    const Builder::Maze::Point next = { finish.row + p.row, finish.col + p.col };
    maze[next.row][next.col] |= Builder::Maze::path_bit_;
  }
  maze[finish.row][finish.col] |= Builder::Maze::path_bit_;
  maze[finish.row][finish.col] |= finish_bit_;

  std::vector<std::thread> threads( num_threads_ );
  // Randomly shuffle thread start corners so colors mix differently each time.
  shuffle( begin( monitor.starts ), end( monitor.starts ), std::mt19937( std::random_device {}() ) );
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_id this_thread = { i_thread, thread_bits_.at( i_thread ) };
    threads[i_thread] = std::thread( animate_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }

  if ( monitor.winning_index ) {
    const Thread_paint winner_color = ( thread_bits_.at( monitor.winning_index.value() ) << thread_paint_shift_ );
    const Builder::Maze::Point& before_finish = monitor.thread_paths.at( monitor.winning_index.value() ).back();
    maze[before_finish.row][before_finish.col] |= winner_color;
    flush_cursor_path_coordinate( maze, before_finish );
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
  }

  Printer::set_cursor_position( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_hunt_solution_message( monitor.winning_index );
  std::cout << "\n";
}

} // namespace Solver