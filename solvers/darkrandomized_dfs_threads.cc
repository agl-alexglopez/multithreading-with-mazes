module;
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <thread>
#include <vector>
export module labyrinth:dark_rdfs;
import :maze;
import :speed;
import :printers;
import :solver_utilities;
import :my_queue;

namespace {

struct Solver_monitor
{
  std::mutex monitor {};
  std::optional<Speed::Speed_unit> speed {};
  std::vector<Maze::Point> starts {};
  std::optional<int> winning_index {};
  std::vector<std::vector<Maze::Point>> thread_paths;
  Solver_monitor() : thread_paths { Sutil::num_threads, std::vector<Maze::Point> {} }
  {
    for ( std::vector<Maze::Point>& path : thread_paths ) {
      path.reserve( Sutil::initial_path_len );
    }
  }
};

void animate_hunt( Maze::Maze& maze, Solver_monitor& monitor, Sutil::Thread_id id )
{
  const Sutil::Thread_cache seen = id.bit << Sutil::thread_cache_shift;
  const Sutil::Thread_paint paint_bit = id.bit << Sutil::thread_paint_shift;
  std::vector<Maze::Point>& dfs = monitor.thread_paths.at( id.index );
  dfs.push_back( monitor.starts.at( id.index ) );
  Maze::Point cur = monitor.starts.at( id.index );
  std::vector<int> random_direction_indices( Sutil::dirs.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  std::mt19937 generator( std::random_device {}() );
  while ( !dfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( monitor.winning_index ) {
      return;
    }

    // Don't pop() yet!
    cur = dfs.back();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & Sutil::finish_bit ) {
      if ( !monitor.winning_index ) {
        monitor.winning_index = id.index;
        Sutil::flush_cursor_path_coordinate( maze, cur );
      }
      monitor.monitor.unlock();
      dfs.pop_back();
      return;
    }
    maze[cur.row][cur.col] |= seen;
    maze[cur.row][cur.col] |= paint_bit;
    Sutil::flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

    // Bias each thread's first choice towards orginal dispatch direction. More coverage.
    bool found_branch_to_explore = false;
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = Sutil::dirs.at( i );
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };

      monitor.monitor.lock();
      const bool push_next = !( maze[next.row][next.col] & seen ) && ( maze[next.row][next.col] & Maze::path_bit );
      monitor.monitor.unlock();

      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
    }

    if ( !found_branch_to_explore ) {
      monitor.monitor.lock();
      maze[cur.row][cur.col] &= static_cast<Sutil::Thread_paint>( ~paint_bit );
      Sutil::flush_cursor_path_coordinate( maze, cur );
      monitor.monitor.unlock();
      std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
      dfs.pop_back();
    }
  }
}

void animate_gather( Maze::Maze& maze, Solver_monitor& monitor, Sutil::Thread_id id )
{
  const Sutil::Thread_cache seen = id.bit << Sutil::thread_cache_shift;
  const Sutil::Thread_paint paint_bit = id.bit << Sutil::thread_paint_shift;
  std::vector<Maze::Point>& dfs = monitor.thread_paths.at( id.index );
  dfs.push_back( monitor.starts.at( 0 ) );
  Maze::Point cur = monitor.starts.at( 0 );
  std::vector<int> random_direction_indices( Sutil::dirs.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  std::mt19937 generator( std::random_device {}() );
  while ( !dfs.empty() ) {
    cur = dfs.back();

    monitor.monitor.lock();
    if ( maze[cur.row][cur.col] & Sutil::finish_bit
         && !( maze[cur.row][cur.col] & Sutil::cache_mask ) ) {
      maze[cur.row][cur.col] |= seen;
      Sutil::flush_cursor_path_coordinate( maze, cur );
      monitor.monitor.unlock();
      dfs.pop_back();
      return;
    }
    maze[cur.row][cur.col] |= seen;
    maze[cur.row][cur.col] |= paint_bit;
    Sutil::flush_cursor_path_coordinate( maze, cur );
    monitor.monitor.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );

    bool found_branch_to_explore = false;
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator );
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = Sutil::dirs.at( i );
      const Maze::Point next = { cur.row + p.row, cur.col + p.col };

      monitor.monitor.lock();
      const bool push_next = !( maze[next.row][next.col] & seen ) && ( maze[next.row][next.col] & Maze::path_bit );
      monitor.monitor.unlock();

      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
    }

    if ( !found_branch_to_explore ) {
      monitor.monitor.lock();
      maze[cur.row][cur.col] &= static_cast<Sutil::Thread_paint>( ~paint_bit );
      Sutil::flush_cursor_path_coordinate( maze, cur );
      monitor.monitor.unlock();
      std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
      dfs.pop_back();
    }
  }
}

} // namespace

/* * * * * * * * * * * *  Multithreaded Dispatcher Functions from Header Interface   * * * * * * * * * * * * * * */

export namespace Dark_rdfs {

void animate_darkrandomized_dfs_thread_hunt( Maze::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  Sutil::print_overlap_key();
  Sutil::deluminate_maze( maze );
  Solver_monitor monitor;
  monitor.speed = Sutil::solver_speeds.at( static_cast<int>( speed ) );
  monitor.starts
    = std::vector<Maze::Point>( Sutil::num_threads, Sutil::pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= Sutil::start_bit;
  const Maze::Point finish = Sutil::pick_random_point( maze );
  maze[finish.row][finish.col] |= Sutil::finish_bit;

  std::vector<std::thread> threads( Sutil::num_threads );
  for ( int i_thread = 0; i_thread < Sutil::num_threads; i_thread++ ) {
    const Sutil::Thread_id this_thread { i_thread, Sutil::thread_bits.at( i_thread ) };
    threads[i_thread] = std::thread( animate_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
  Printer::set_cursor_position( { maze.row_size() + Sutil::overlap_key_and_message_height, 0 } );
  Sutil::print_hunt_solution_message( monitor.winning_index );
  std::cout << "\n";
}

void animate_darkrandomized_dfs_thread_gather( Maze::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  Sutil::print_overlap_key();
  Sutil::deluminate_maze( maze );
  Solver_monitor monitor;
  monitor.speed = Sutil::solver_speeds.at( static_cast<int>( speed ) );
  monitor.starts
    = std::vector<Maze::Point>( Sutil::num_threads, Sutil::pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= Sutil::start_bit;
  for ( int finish_square = 0; finish_square < Sutil::num_gather_finishes; finish_square++ ) {
    const Maze::Point finish = Sutil::pick_random_point( maze );
    maze[finish.row][finish.col] |= Sutil::finish_bit;
  }

  std::vector<std::thread> threads( Sutil::num_threads );
  for ( int i_thread = 0; i_thread < Sutil::num_threads; i_thread++ ) {
    const Sutil::Thread_id this_thread { i_thread, Sutil::thread_bits.at( i_thread ) };
    threads[i_thread] = std::thread( animate_gather, std::ref( maze ), std::ref( monitor ), this_thread );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
  Printer::set_cursor_position( { maze.row_size() + Sutil::overlap_key_and_message_height, 0 } );
  Sutil::print_gather_solution_message();
  std::cout << "\n";
}

void animate_darkrandomized_dfs_thread_corners( Maze::Maze& maze, Speed::Speed speed )
{
  Printer::set_cursor_position( { maze.row_size(), 0 } );
  Sutil::print_overlap_key();
  Sutil::deluminate_maze( maze );
  Solver_monitor monitor;
  monitor.speed = Sutil::solver_speeds.at( static_cast<int>( speed ) );
  monitor.starts = Sutil::set_corner_starts( maze );
  for ( const Maze::Point& p : monitor.starts ) {
    maze[p.row][p.col] |= Sutil::start_bit;
  }
  const Maze::Point finish = { maze.row_size() / 2, maze.col_size() / 2 };
  for ( const Maze::Point& p : Sutil::all_dirs ) {
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
    threads[i_thread] = std::thread( animate_hunt, std::ref( maze ), std::ref( monitor ), this_thread );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }
  Printer::set_cursor_position( { maze.row_size() + Sutil::overlap_key_and_message_height, 0 } );
  Sutil::print_hunt_solution_message( monitor.winning_index );
  std::cout << "\n";
}

} // namespace Dark_rdfs
