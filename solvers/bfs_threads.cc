#include "maze_solvers.hh"
#include "my_queue.hh"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Solver {

namespace
{

struct Solver_monitor {
  std::mutex monitor;
  std::optional<Speed_unit> speed {};
  std::vector<std::unordered_map<Maze::Point, Maze::Point>> thread_maps;
  std::vector<My_queue<Maze::Point>> thread_queues;
  std::vector<Maze::Point> starts {};
  std::optional<int> winning_index {};
  std::vector<std::vector<Maze::Point>> thread_paths;
  Solver_monitor()
    : thread_maps{ num_threads_ }
    , thread_queues{ num_threads_ }
    , thread_paths{ num_threads_, std::vector<Maze::Point>{} } {
    for ( std::vector<Maze::Point>& path : thread_paths ) { path.reserve( initial_path_len_ ); }
    for ( My_queue<Maze::Point>& q : thread_queues ) { q.reserve( initial_path_len_ ); }
  }
};

void complete_hunt( Maze& maze, Solver_monitor& monitor, const Thread_id& id )
{
  // This will be how we rebuild the path because queue does not represent the current path.
  std::unordered_map<Maze::Point, Maze::Point>& seen = monitor.thread_maps[id.index];
  seen[monitor.starts.at( 0 )] = { -1, -1 };
  My_queue<Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( 0 ) );
  Maze::Point cur = monitor.starts.at( 0 );
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
    int direction_index = id.index;
    do {
      const Maze::Point& p = cardinal_directions_.at( direction_index );
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      bool seen_next = seen.contains( next );

      monitor.monitor.lock();
      bool push_next = !seen_next && ( maze[next.row][next.col] & Maze::path_bit_ );
      monitor.monitor.unlock();

      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != id.index );
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    monitor.thread_paths[id.index].push_back( cur );
    cur = seen.at( cur );
  }
}

void animate_hunt( Maze& maze, Solver_monitor& monitor, const Thread_id& id )
{
  // This will be how we rebuild the path because queue does not represent the current path.
  std::unordered_map<Maze::Point, Maze::Point>& seen = monitor.thread_maps[id.index];
  seen[monitor.starts.at( 0 )] = { -1, -1 };
  My_queue<Maze::Point>& bfs = monitor.thread_queues[id.index];
  bfs.push( monitor.starts.at( 0 ) );
  Maze::Point cur = monitor.starts.at( 0 );
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
    int direction_index = id.index;
    do {
      const Maze::Point& p = cardinal_directions_.at( direction_index );
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      bool seen_next = seen.contains( next );

      monitor.monitor.lock();
      bool push_next = !seen_next && ( maze[next.row][next.col] & Maze::path_bit_ );
      monitor.monitor.unlock();

      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != id.index );
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    monitor.thread_paths[id.index].push_back( cur );
    cur = seen.at( cur );
  }
}

void complete_gather( Maze& maze, Solver_monitor& monitor, const Thread_id& id )
{
  std::unordered_map<Maze::Point, Maze::Point>& seen = monitor.thread_maps[id.index];
  Thread_cache seen_bit = id.paint << 4;
  seen[monitor.starts[ id.index ] ] = { -1, -1 };
  My_queue<Maze::Point>& bfs = monitor.thread_queues[ id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Maze::Point cur = monitor.starts.at( id.index );
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

    int direction_index = id.index;
    do {
      const Maze::Point& p = cardinal_directions_.at( direction_index );
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      bool seen_next = seen.contains( next );
      monitor.monitor.lock();
      bool push_next = !seen_next && ( maze[next.row][next.col] & Maze::path_bit_ );
      monitor.monitor.unlock();
      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != id.index );
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    monitor.thread_paths[id.index].push_back( cur );
    cur = seen.at( cur );
  }
  monitor.winning_index = id.index;
}

void animate_gather( Maze& maze, Solver_monitor& monitor, const Thread_id& id )
{
  std::unordered_map<Maze::Point, Maze::Point>& seen = monitor.thread_maps[id.index];
  Thread_cache seen_bit = id.paint << 4;
  seen[monitor.starts[ id.index ] ] = { -1, -1 };
  My_queue<Maze::Point>& bfs = monitor.thread_queues[ id.index];
  bfs.push( monitor.starts.at( id.index ) );
  Maze::Point cur = monitor.starts.at( id.index );
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

    int direction_index = id.index;
    do {
      const Maze::Point& p = cardinal_directions_.at( direction_index );
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      bool seen_next = seen.contains( next );
      monitor.monitor.lock();
      bool push_next = !seen_next && ( maze[next.row][next.col] & Maze::path_bit_ );
      monitor.monitor.unlock();
      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != id.index );
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    monitor.thread_paths[id.index].push_back( cur );
    cur = seen.at( cur );
  }
  monitor.winning_index = id.index;

}

}// namespace


/* * * * * * * * * * * *  Multithreaded Dispatcher Functions from Header Interface   * * * * * * * * * * * * * * */

void solve_with_bfs_thread_hunt( Maze& maze )
{
  Solver_monitor monitor;
  monitor.starts = std::vector<Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  Maze::Point finish = pick_random_point( maze );
  maze[finish.row][finish.col] |= finish_bit_;
  std::vector<std::thread> threads ( num_threads_ );
  for ( int thread = 0; thread < num_threads_; thread++ ) {
    const Thread_id this_thread { thread, thread_masks_.at( thread ) };
    threads[ thread ] = std::thread( [ & ] { complete_hunt( maze, monitor, this_thread ); } );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }
  if ( !monitor.winning_index ) {
    std::cerr << "error: no winner in bfs search." << std::endl;
    abort();
  }
  // It is cool to see the shortest path that the winning thread took to victory
  Thread_paint winner_color = thread_masks_.at( monitor.winning_index.value() );
  for ( const Maze::Point& p : monitor.thread_paths.at( monitor.winning_index.value() ) ) {
    maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
    maze[p.row][p.col] |= winner_color;
  }
  clear_screen();
  print_maze( maze );
  print_overlap_key();
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}

void animate_with_bfs_thread_hunt( Maze& maze, Solver_speed speed )
{
  Solver_monitor monitor;
  monitor.speed = solver_speeds_.at( static_cast<Speed_unit>( speed ) );
  monitor.starts.emplace_back( pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  Maze::Point finish = pick_random_point( maze );
  maze[finish.row][finish.col] |= finish_bit_;
  flush_cursor_path_coordinate( maze, finish );
  set_cursor_point( { maze.row_size(), 0 } );
  print_overlap_key();

  std::vector<std::thread> threads ( num_threads_ );
  for ( int thread = 0; thread < num_threads_; thread++ ) {
    const Thread_id this_thread { thread, thread_masks_.at( thread ) };
    threads[ thread ] = std::thread( [ & ] { animate_hunt( maze, monitor, this_thread ); } );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }

  if ( !monitor.winning_index ) {
    std::cerr << "error: no winner in bfs search." << std::endl;
    abort();
  }

  // It is cool to see the shortest path that the winning thread took to victory
  Thread_paint winner_color = thread_masks_.at( monitor.winning_index.value() );
  for ( const Maze::Point& p : monitor.thread_paths.at( monitor.winning_index.value() ) ) {
    maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
    maze[p.row][p.col] |= winner_color;
    flush_cursor_path_coordinate( maze, p );
  }
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}

void solve_with_bfs_thread_gather( Maze& maze )
{
  Solver_monitor monitor;
  monitor.starts = std::vector<Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  for ( int finish_square = 0; finish_square < num_gather_finishes_; finish_square++ ) {
    Maze::Point finish = pick_random_point( maze );
    maze[finish.row][finish.col] |= finish_bit_;
  }
  std::vector<std::thread> threads ( num_threads_ );
  for ( int thread = 0; thread < num_threads_; thread++ ) {
    const Thread_id this_thread { thread, thread_masks_.at( thread ) };
    threads[ thread ] = std::thread( [ & ] { complete_gather( maze, monitor, this_thread ); } );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
  int thread = 0;
  for ( const std::vector<Maze::Point>& path : monitor.thread_paths ) {
    Thread_paint color = thread_masks_.at( thread );
    for ( const Maze::Point& p : path ) {
      maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
      maze[p.row][p.col] |= color;
    }
    thread++;
  }
  clear_screen();
  print_maze( maze );
  print_overlap_key();
  print_gather_solution_message();
  std::cout << std::endl;
}

void animate_with_bfs_thread_gather( Maze& maze, Solver_speed speed )
{
  Solver_monitor monitor;
  monitor.speed = static_cast<Speed_unit>( speed );
  monitor.starts = std::vector<Maze::Point>( num_threads_, pick_random_point( maze ) );
  maze[monitor.starts.at( 0 ).row][monitor.starts.at( 0 ).col] |= start_bit_;
  for ( int finish_square = 0; finish_square < num_gather_finishes_; finish_square++ ) {
    Maze::Point finish = pick_random_point( maze );
    maze[finish.row][finish.col] |= finish_bit_;
    flush_cursor_path_coordinate( maze, finish );
  }
  set_cursor_point( { maze.row_size(), 0 } );
  print_overlap_key();

  std::vector<std::thread> threads ( num_threads_ );
  for ( int thread = 0; thread < num_threads_; thread++ ) {
    const Thread_id this_thread { thread, thread_masks_.at( thread ) };
    threads[ thread ] = std::thread( [ & ] { animate_gather( maze, monitor, this_thread ); } );
  }

  for ( std::thread& t : threads ) {
    t.join();
  }

  int thread = 0;
  for ( const std::vector<Maze::Point>& path : monitor.thread_paths ) {
    Thread_paint color = thread_masks_.at( thread );
    for ( const Maze::Point& p : path ) {
      maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
      maze[p.row][p.col] |= color;
      flush_cursor_path_coordinate( maze, p );
      std::this_thread::sleep_for( std::chrono::microseconds( monitor.speed.value_or( 0 ) ) );
    }
    thread++;
  }
  set_cursor_point( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_gather_solution_message();
  std::cout << std::endl;
}

void solve_with_bfs_thread_corners( Maze& maze )
{
  Solver_monitor monitor;
  monitor.starts = set_corner_starts( maze );
  for ( const Maze::Point& p : monitor.starts ) {
    maze[p.row][p.col] |= start_bit_;
  }
  Maze::Point finish = { maze.row_size() / 2, maze.col_size() / 2 };
  for ( const Maze::Point& p : all_directions_ ) {
    Maze::Point next = { finish.row + p.row, finish.col + p.col };
    maze[next.row][next.col] |= Maze::path_bit_;
  }
  maze[finish.row][finish.col] |= Maze::path_bit_;
  maze[finish.row][finish.col] |= finish_bit_;

  std::vector<std::thread> threads( num_threads_ );
  // Randomly shuffle thread start corners so colors mix differently each time.
  shuffle( begin( monitor.starts ), end( monitor.starts ), std::mt19937( std::random_device {}()) );
  for ( int thread = 0; thread < num_threads_; thread++ ) {
    const Thread_id this_thread = { thread, thread_masks_.at( thread ) };
    threads[ thread ] = std::thread( [ & ] { complete_hunt( maze, monitor, this_thread ); } );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }
  clear_screen();
  print_maze( maze );
  print_overlap_key();
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}

void animate_with_bfs_thread_corners( Maze& maze, Solver_speed speed )
{
  Solver_monitor monitor;
  monitor.speed = solver_speeds_.at( static_cast<int>( speed ) );
  monitor.starts = set_corner_starts( maze );
  for ( const Maze::Point& p : monitor.starts ) {
    maze[p.row][p.col] |= start_bit_;
  }
  Maze::Point finish = { maze.row_size() / 2, maze.col_size() / 2 };
  for ( const Maze::Point& p : all_directions_ ) {
    Maze::Point next = { finish.row + p.row, finish.col + p.col };
    maze[next.row][next.col] |= Maze::path_bit_;
  }
  maze[finish.row][finish.col] |= Maze::path_bit_;
  maze[finish.row][finish.col] |= finish_bit_;

  std::vector<std::thread> threads( num_threads_ );
  // Randomly shuffle thread start corners so colors mix differently each time.
  shuffle( begin( monitor.starts ), end( monitor.starts ), std::mt19937( std::random_device {}() ) );
  for ( int thread = 0; thread < num_threads_; thread++ ) {
    const Thread_id this_thread = { thread, thread_masks_.at( thread ) };
    threads[ thread ] = std::thread( [ & ] { complete_hunt( maze, monitor, this_thread ); } );
  }
  for ( std::thread& t : threads ) {
    t.join();
  }
  if ( !monitor.winning_index ) {
    std::cerr << "error: no winner in bfs search." << std::endl;
    abort();
  }

  // It is cool to see the shortest path that the winning thread took to victory
  Thread_paint winner_color = thread_masks_.at( monitor.winning_index.value() );
  for ( const Maze::Point& p : monitor.thread_paths.at( monitor.winning_index.value() ) ) {
    maze[p.row][p.col] &= static_cast<Thread_paint>( ~thread_mask_ );
    maze[p.row][p.col] |= winner_color;
    flush_cursor_path_coordinate( maze, p );
  }
  set_cursor_point( { maze.row_size() + overlap_key_and_message_height, 0 } );
  print_hunt_solution_message( monitor.winning_index );
  std::cout << std::endl;
}


} // namespace Solver
