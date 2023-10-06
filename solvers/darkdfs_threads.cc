#include "maze_solvers.hh"
#include "print_utilities.hh"
#include "solver_utilities.hh"
#include <array>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_set>

namespace Solver {

namespace {

struct Thread_light
{
  int index;
  uint16_t bit;
};

struct Solver_monitor
{
  std::mutex monitor {};
  Speed::Speed_unit speed {};
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

void animate_hunt( Builder::Maze& maze, Solver_monitor& monitor, Thread_light id )
{
  const Thread_cache seen = id.bit << thread_cache_offset_;
  const Thread_paint paint = id.bit << thread_tag_offset_;
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
        monitor.winning_index = id.index;
        flush_cursor_path_coordinate( maze, cur );
      }
      monitor.monitor.unlock();
      dfs.pop_back();
      return;
    }
    maze[cur.row][cur.col] |= paint;
    maze[cur.row][cur.col] |= seen;
    flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed ) );

    // Bias each thread's first choice towards orginal dispatch direction. More coverage.
    bool found_branch_to_explore = false;
    for ( uint64_t count = 0, i = id.index; count < n_e_s_w_.size(); count++, ++i %= n_e_s_w_.size() ) {
      const Builder::Maze::Point& p = n_e_s_w_.at( i );
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
      monitor.monitor.lock();
      maze[cur.row][cur.col] &= static_cast<Thread_paint>( ~paint );
      flush_cursor_path_coordinate( maze, cur );
      monitor.monitor.unlock();
      std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed ) );
      dfs.pop_back();
    }
  }
}

void animate_gather( Builder::Maze& maze, Solver_monitor& monitor, Thread_light id )
{
  const Thread_cache seen = id.bit << thread_cache_offset_;
  const Thread_paint paint = id.bit << thread_tag_offset_;
  std::vector<Builder::Maze::Point>& dfs = monitor.thread_paths.at( id.index );
  dfs.push_back( monitor.starts.at( id.index ) );
  Builder::Maze::Point cur = monitor.starts.at( id.index );
  while ( !dfs.empty() ) {
    cur = dfs.back();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & finish_bit_ && !( maze[cur.row][cur.col] & cache_mask_ ) ) {
      maze[cur.row][cur.col] |= seen;
      flush_cursor_path_coordinate( maze, cur );
      monitor.monitor.unlock();
      dfs.pop_back();
      return;
    }
    maze[cur.row][cur.col] |= seen;
    maze[cur.row][cur.col] |= paint;
    flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed ) );

    bool found_branch_to_explore = false;
    for ( uint64_t count = 0, i = id.index; count < n_e_s_w_.size(); count++, ++i %= n_e_s_w_.size() ) {
      const Builder::Maze::Point& p = n_e_s_w_.at( i );
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
      monitor.monitor.lock();
      maze[cur.row][cur.col] &= static_cast<Thread_paint>( ~paint );
      flush_cursor_path_coordinate( maze, cur );
      monitor.monitor.unlock();
      std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed ) );
      dfs.pop_back();
    }
  }
}

} // namespace

void animate_darkdfs_thread_hunt( Builder::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  print_overlap_key();
  deluminate_maze( maze );
  Printer::set_cursor_position( { 0, 0 } );
  Solver_monitor monitor;
  monitor.speed = solver_speeds_.at( static_cast<int>( speed ) );
  monitor.starts = std::vector<Builder::Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  const Builder::Maze::Point finish = pick_random_point( maze );
  maze[finish.row][finish.col] |= finish_bit_;

  std::array<std::thread, num_threads_> threads;
  for ( int i_thread = 0; i_thread < num_threads_; i_thread++ ) {
    const Thread_light this_thread { i_thread, thread_bits_.at( i_thread ) };
    threads[i_thread] = std::thread( animate_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
  Printer::set_cursor_position( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}

void animate_darkdfs_thread_gather( Builder::Maze& maze, Speed::Speed speed )
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
    const Thread_light this_thread { i_thread, thread_bits_.at( i_thread ) };
    threads[i_thread] = std::thread( animate_gather, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
  Printer::set_cursor_position( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_gather_solution_message();
  std::cout << std::endl;
}

void animate_darkdfs_thread_corners( Builder::Maze& maze, Speed::Speed speed )
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
    const Thread_light this_thread = { i_thread, thread_bits_.at( i_thread ) };
    threads[i_thread] = std::thread( animate_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }
  Printer::set_cursor_position( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}

} // namespace Solver
