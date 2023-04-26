#include "maze_solvers.hh"
#include "my_queue.hh"

#include <chrono>
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

}// namespace

} // namespace Solver
