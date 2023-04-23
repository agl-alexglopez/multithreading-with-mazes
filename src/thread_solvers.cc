#include "thread_solvers.hh"
#include <algorithm>
#include <iostream>
#include <thread>

Thread_solvers::Thread_solvers( Maze& maze, const Solver_args& args )
  : maze_( maze )
  , solver_mutex_()
  , game_( args.game )
  , solver_( args.solver )
  , solver_speed_( solver_speeds_[static_cast<size_t>( args.speed )] )
  , start_( {} )
  , finish_( {} )
  , corner_starts_( {} )
  , escape_path_index_( -1 )
  , thread_paths_( num_threads_ )
  , thread_queues_( num_threads_ )
  , thread_bfs_maps( num_threads_ )
  , generator_( std::random_device {}() )
  , row_random_( 1, maze_.row_size() - 2 )
  , col_random_( 1, maze_.col_size() - 2 )
{
  for ( size_t i = 0; i < thread_paths_.size(); i++ ) {
    thread_paths_[i].reserve( starting_path_len_ );
    thread_queues_[i].reserve( starting_path_len_ );
  }
}

void Thread_solvers::place_start_finish()
{
  // Dimensions of a maze vary based on random build generation. We need this atrocity.
  auto set_corner = [&]( int row_init,
                         int col_init,
                         int row_end,
                         int col_end,
                         bool increment_row,
                         bool increment_col,
                         int corner_array_index ) {
    for ( int row = row_init; ( increment_row ? row < row_end : row > row_end ); increment_row ? row++ : row-- ) {
      for ( int col = col_init; ( increment_col ? col < col_end : col > col_end ); increment_col ? col++ : col-- ) {
        if ( maze_[row][col] & Maze::path_bit_ ) {
          maze_[row][col] |= start_bit_;
          corner_starts_[corner_array_index] = { row, col };
          return;
        }
      }
    }
  };

  if ( game_ == Maze_game::corners ) {
    set_corner( 1, 1, maze_.row_size() - 2, maze_.col_size() - 2, true, true, 0 );
    set_corner( 1, maze_.col_size() - 2, maze_.row_size() - 2, 0, true, false, 1 );
    set_corner( maze_.row_size() - 2, 1, 0, maze_.col_size() - 2, false, true, 2 );
    set_corner( maze_.row_size() - 2, maze_.col_size() - 2, 0, 0, false, false, 3 );
    int middle_row = maze_.row_size() / 2;
    int middle_col = maze_.col_size() / 2;
    finish_ = { middle_row, middle_col };
    for ( const Maze::Point& p : all_directions_ ) {
      Maze::Point next = { finish_.row + p.row, finish_.col + p.col };
      maze_[next.row][next.col] |= Maze::path_bit_;
    }
    maze_[middle_row][middle_col] |= Maze::path_bit_;
    maze_[middle_row][middle_col] |= finish_bit_;
  } else {
    start_ = pick_random_point();
    maze_[start_.row][start_.col] |= start_bit_;
    int num_finishes = game_ == Maze_game::gather ? 4 : 1;
    for ( int placement = 0; placement < num_finishes; placement++ ) {
      finish_ = pick_random_point();
      maze_[finish_.row][finish_.col] |= finish_bit_;
    }
  }
}

void Thread_solvers::place_start_finish_animated()
{
  // Dimensions of a maze vary based on random build generation. We need this atrocity.
  auto set_corner = [&]( int row_init,
                         int col_init,
                         int row_end,
                         int col_end,
                         bool increment_row,
                         bool increment_col,
                         int corner_array_index ) {
    for ( int row = row_init; ( increment_row ? row < row_end : row > row_end ); increment_row ? row++ : row-- ) {
      for ( int col = col_init; ( increment_col ? col < col_end : col > col_end ); increment_col ? col++ : col-- ) {
        if ( maze_[row][col] & Maze::path_bit_ ) {
          maze_[row][col] |= start_bit_;
          corner_starts_[corner_array_index] = { row, col };
          flush_cursor_path_coordinate( row, col );
          return;
        }
      }
    }
  };

  if ( game_ == Maze_game::corners ) {
    set_corner( 1, 1, maze_.row_size() - 2, maze_.col_size() - 2, true, true, 0 );
    set_corner( 1, maze_.col_size() - 2, maze_.row_size() - 2, 0, true, false, 1 );
    set_corner( maze_.row_size() - 2, 1, 0, maze_.col_size() - 2, false, true, 2 );
    set_corner( maze_.row_size() - 2, maze_.col_size() - 2, 0, 0, false, false, 3 );
    int middle_row = maze_.row_size() / 2;
    int middle_col = maze_.col_size() / 2;
    finish_ = { middle_row, middle_col };
    for ( const Maze::Point& p : all_directions_ ) {
      Maze::Point next = { finish_.row + p.row, finish_.col + p.col };
      maze_[next.row][next.col] |= Maze::path_bit_;
      flush_cursor_path_coordinate( next.row, next.col );
    }
    maze_[middle_row][middle_col] |= Maze::path_bit_;
    maze_[middle_row][middle_col] |= finish_bit_;
    flush_cursor_path_coordinate( middle_row, middle_col );
  } else {
    start_ = pick_random_point();
    maze_[start_.row][start_.col] |= start_bit_;
    int num_finishes = game_ == Maze_game::gather ? 4 : 1;
    for ( int placement = 0; placement < num_finishes; placement++ ) {
      finish_ = pick_random_point();
      maze_[finish_.row][finish_.col] |= finish_bit_;
      flush_cursor_path_coordinate( finish_.row, finish_.col );
    }
  }
}

/* * * * * * * * * * * * * * * * *      Maze Solvers Caller      * * * * * * * * * * * * * * * * */

void Thread_solvers::solve_maze()
{
  if ( solver_speed_ ) {
    if ( !maze_.is_animated() ) {
      clear_and_flush_paths();
    }
    place_start_finish_animated();
    set_cursor_point( maze_.row_size(), 0 );
    print_overlap_key();
    maze_.print_builder();
    print_solver();
    std::cout << std::flush;
  } else {
    place_start_finish();
  }
  if ( solver_ == Solver_algorithm::depth_first_search ) {
    if ( solver_speed_ ) {
      animate_with_dfs_threads();
    } else {
      solve_with_dfs_threads();
    }
  } else if ( solver_ == Solver_algorithm::randomized_depth_first_search ) {
    if ( solver_speed_ ) {
      animate_with_randomized_dfs_threads();
    } else {
      solve_with_randomized_dfs_threads();
    }
  } else if ( solver_ == Solver_algorithm::breadth_first_search ) {
    if ( solver_speed_ ) {
      animate_with_bfs_threads();
    } else {
      solve_with_bfs_threads();
    }
  } else {
    std::cerr << "Invalid solver?" << std::endl;
    abort();
  }
  if ( solver_speed_ ) {
    set_cursor_point( maze_.row_size() + overlap_key_and_message_height, 0 );
  } else {
    clear_screen();
    print_maze();
    print_overlap_key();
    maze_.print_builder();
    print_solver();
  }
  print_solution_path();
  std::cout << std::endl;
}

/* * * * * * * * * * * * * * * * *      Depth First Search       * * * * * * * * * * * * * * * * */

void Thread_solvers::solve_with_dfs_threads()
{
  if ( escape_path_index_ != -1 ) {
    clear_paths();
  }
  std::vector<std::thread> threads( cardinal_directions_.size() );
  if ( game_ == Maze_game::hunt ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread( [this, i, thread_mask] { dfs_thread_hunt( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::gather ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread( [this, i, thread_mask] { dfs_thread_gather( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::corners ) {
    // Randomly shuffle thread start corners so colors mix differently each time.
    std::vector<int> random_starting_corners( corner_starts_.size() );
    std::iota( begin( random_starting_corners ), end( random_starting_corners ), 0 );
    shuffle( begin( random_starting_corners ), end( random_starting_corners ), generator_ );
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      const Maze::Point& corner = corner_starts_[random_starting_corners[i]];
      threads[i] = std::thread( [this, corner, i, thread_mask] { dfs_thread_hunt( corner, i, thread_mask ); } );
    }
  } else {
    std::cerr << "Uncategorized game slipped through initializations." << std::endl;
    std::abort();
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
}

void Thread_solvers::animate_with_dfs_threads()
{
  if ( escape_path_index_ != -1 ) {
    clear_paths();
  }

  std::vector<std::thread> threads( cardinal_directions_.size() );
  if ( game_ == Maze_game::hunt ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread( [this, i, thread_mask] { dfs_thread_hunt_animated( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::gather ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread( [this, i, thread_mask] { dfs_thread_gather_animated( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::corners ) {
    // Randomly shuffle thread start corners so colors mix differently each time.
    std::vector<int> random_starting_corners( corner_starts_.size() );
    std::iota( begin( random_starting_corners ), end( random_starting_corners ), 0 );
    shuffle( begin( random_starting_corners ), end( random_starting_corners ), generator_ );
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      const Maze::Point& corner = corner_starts_[random_starting_corners[i]];
      threads[i]
        = std::thread( [this, corner, i, thread_mask] { dfs_thread_hunt_animated( corner, i, thread_mask ); } );
    }
  } else {
    std::cerr << "Uncategorized game slipped through initializations." << std::endl;
    std::abort();
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
}

void Thread_solvers::dfs_thread_hunt( Maze::Point start, int thread_index, Thread_paint paint )
{
  /* We have useful bits in a square. Each square can use a unique bit to track seen threads.
   * Each thread could maintain its own hashset, but this is much more space efficient. Use
   * the space the maze already occupies and provides.
   */
  Thread_cache seen = paint << thread_tag_offset_;
  // Each thread only needs enough space for an O(current path length) stack.
  std::vector<Maze::Point>& dfs = thread_paths_[thread_index];
  dfs.push_back( start );
  Maze::Point cur = start;
  while ( !dfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( escape_path_index_ != -1 ) {
      break;
    }

    // Don't pop() yet!
    cur = dfs.back();

    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ ) {
      if ( escape_path_index_ == -1 ) {
        escape_path_index_ = thread_index;
      }
      solver_mutex_.unlock();
      dfs.pop_back();
      break;
    }
    maze_[cur.row][cur.col] |= seen;
    solver_mutex_.unlock();

    // Bias each thread's first choice towards orginal dispatch direction. More coverage.
    int direction_index = thread_index;
    bool found_branch_to_explore = false;
    do {
      const Maze::Point& p = cardinal_directions_[direction_index];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      solver_mutex_.lock();
      bool push_next
        = !( maze_[next.row][next.col] & seen ) && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != thread_index );
    if ( !found_branch_to_explore ) {
      dfs.pop_back();
    }
  }
  solver_mutex_.lock();
  // Another benefit of true depth first search is our stack holds path to exact location.
  for ( const Maze::Point& p : dfs ) {
    maze_[p.row][p.col] |= paint;
  }
  solver_mutex_.unlock();
}

void Thread_solvers::dfs_thread_hunt_animated( Maze::Point start, int thread_index, Thread_paint paint )
{
  Thread_cache seen = paint << thread_tag_offset_;
  std::vector<Maze::Point>& dfs = thread_paths_[thread_index];
  dfs.push_back( start );
  Maze::Point cur = start;
  while ( !dfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( escape_path_index_ != -1 ) {
      return;
    }

    // Don't pop() yet!
    cur = dfs.back();

    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ ) {
      if ( escape_path_index_ == -1 ) {
        escape_path_index_ = thread_index;
      }
      solver_mutex_.unlock();
      dfs.pop_back();
      return;
    }
    maze_[cur.row][cur.col] |= seen;
    maze_[cur.row][cur.col] |= paint;
    flush_cursor_path_coordinate( cur.row, cur.col );
    solver_mutex_.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );

    // Bias each thread's first choice towards orginal dispatch direction. More coverage.
    int direction_index = thread_index;
    bool found_branch_to_explore = false;
    do {
      const Maze::Point& p = cardinal_directions_[direction_index];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      solver_mutex_.lock();
      bool push_next
        = !( maze_[next.row][next.col] & seen ) && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != thread_index );
    if ( !found_branch_to_explore ) {
      solver_mutex_.lock();
      maze_[cur.row][cur.col] &= ~paint;
      flush_cursor_path_coordinate( cur.row, cur.col );
      solver_mutex_.unlock();
      std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );
      dfs.pop_back();
    }
  }
}

void Thread_solvers::dfs_thread_gather( Maze::Point start, int thread_index, Thread_paint paint )
{
  Thread_cache seen = paint << thread_tag_offset_;
  std::vector<Maze::Point>& dfs = thread_paths_[thread_index];
  dfs.push_back( start );
  Maze::Point cur = start;
  while ( !dfs.empty() ) {
    cur = dfs.back();

    solver_mutex_.lock();
    // We are the first thread to this finish! Claim it!
    if ( maze_[cur.row][cur.col] & finish_bit_ && !( maze_[cur.row][cur.col] & cache_mask_ ) ) {
      maze_[cur.row][cur.col] |= seen;
      dfs.pop_back();
      for ( const Maze::Point& p : dfs ) {
        maze_[p.row][p.col] |= paint;
      }
      solver_mutex_.unlock();
      return;
    }
    maze_[cur.row][cur.col] |= seen;
    solver_mutex_.unlock();

    // Bias each thread's first choice towards orginal dispatch direction. More coverage.
    int direction_index = thread_index;
    bool found_branch_to_explore = false;
    do {
      const Maze::Point& p = cardinal_directions_[direction_index];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      solver_mutex_.lock();
      bool push_next
        = !( maze_[next.row][next.col] & seen ) && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != thread_index );
    if ( !found_branch_to_explore ) {
      dfs.pop_back();
    }
  }
}

void Thread_solvers::dfs_thread_gather_animated( Maze::Point start, int thread_index, Thread_paint paint )
{
  Thread_cache seen = paint << thread_tag_offset_;
  std::vector<Maze::Point>& dfs = thread_paths_[thread_index];
  dfs.push_back( start );
  Maze::Point cur = start;
  while ( !dfs.empty() ) {
    cur = dfs.back();

    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ && !( maze_[cur.row][cur.col] & cache_mask_ ) ) {
      maze_[cur.row][cur.col] |= seen;
      solver_mutex_.unlock();
      dfs.pop_back();
      return;
    }
    maze_[cur.row][cur.col] |= seen;
    maze_[cur.row][cur.col] |= paint;
    flush_cursor_path_coordinate( cur.row, cur.col );
    solver_mutex_.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );

    int direction_index = thread_index;
    bool found_branch_to_explore = false;
    do {
      const Maze::Point& p = cardinal_directions_[direction_index];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      solver_mutex_.lock();
      bool push_next
        = !( maze_[next.row][next.col] & seen ) && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != thread_index );
    if ( !found_branch_to_explore ) {
      solver_mutex_.lock();
      maze_[cur.row][cur.col] &= ~paint;
      flush_cursor_path_coordinate( cur.row, cur.col );
      solver_mutex_.unlock();
      std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );
      dfs.pop_back();
    }
  }
}

/* * * * * * * * * * * * * * *   Randomized Depth First Search   * * * * * * * * * * * * * * * * */

void Thread_solvers::animate_with_randomized_dfs_threads()
{
  if ( escape_path_index_ != -1 ) {
    clear_paths();
  }

  std::vector<std::thread> threads( cardinal_directions_.size() );
  if ( game_ == Maze_game::hunt ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i]
        = std::thread( [this, i, thread_mask] { randomized_dfs_thread_hunt_animated( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::gather ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread(
        [this, i, thread_mask] { randomized_dfs_thread_gather_animated( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::corners ) {
    std::vector<int> random_starting_corners( corner_starts_.size() );
    std::iota( begin( random_starting_corners ), end( random_starting_corners ), 0 );
    shuffle( begin( random_starting_corners ), end( random_starting_corners ), generator_ );
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      const Maze::Point& corner = corner_starts_[random_starting_corners[i]];
      threads[i] = std::thread(
        [this, corner, i, thread_mask] { randomized_dfs_thread_hunt_animated( corner, i, thread_mask ); } );
    }
  } else {
    std::cerr << "Uncategorized game slipped through initializations." << std::endl;
    std::abort();
  }
  for ( std::thread& t : threads ) {
    t.join();
  }
}

void Thread_solvers::solve_with_randomized_dfs_threads()
{
  if ( escape_path_index_ != -1 ) {
    clear_paths();
  }
  std::vector<std::thread> threads( cardinal_directions_.size() );
  if ( game_ == Maze_game::hunt ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread( [this, i, thread_mask] { randomized_dfs_thread_hunt( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::gather ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i]
        = std::thread( [this, i, thread_mask] { randomized_dfs_thread_gather( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::corners ) {
    std::vector<int> random_starting_corners( corner_starts_.size() );
    std::iota( begin( random_starting_corners ), end( random_starting_corners ), 0 );
    shuffle( begin( random_starting_corners ), end( random_starting_corners ), generator_ );
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      const Maze::Point& corner = corner_starts_[random_starting_corners[i]];
      threads[i]
        = std::thread( [this, corner, i, thread_mask] { randomized_dfs_thread_hunt( corner, i, thread_mask ); } );
    }
  } else {
    std::cerr << "Uncategorized game slipped through initializations." << std::endl;
    std::abort();
  }
  for ( std::thread& t : threads ) {
    t.join();
  }
}

void Thread_solvers::randomized_dfs_thread_hunt( Maze::Point start, int thread_index, Thread_paint paint )
{
  Thread_cache seen = paint << thread_tag_offset_;
  std::vector<Maze::Point>& dfs = thread_paths_[thread_index];
  dfs.push_back( start );
  Maze::Point cur = start;
  std::vector<int> random_direction_indices( generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  while ( !dfs.empty() ) {
    if ( escape_path_index_ != -1 ) {
      break;
    }

    cur = dfs.back();
    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ ) {
      if ( escape_path_index_ == -1 ) {
        escape_path_index_ = thread_index;
      }
      solver_mutex_.unlock();
      dfs.pop_back();
      break;
    }
    maze_[cur.row][cur.col] |= seen;
    solver_mutex_.unlock();

    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator_ );
    bool found_branch_to_explore = false;
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = cardinal_directions_[i];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      solver_mutex_.lock();
      bool push_next
        = !( maze_[next.row][next.col] & seen ) && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
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
  solver_mutex_.lock();
  for ( const Maze::Point& p : dfs ) {
    maze_[p.row][p.col] |= paint;
  }
  solver_mutex_.unlock();
}

void Thread_solvers::randomized_dfs_thread_hunt_animated( Maze::Point start,
                                                          int thread_index,
                                                          Thread_paint paint )
{
  Thread_cache seen = paint << thread_tag_offset_;
  std::vector<Maze::Point>& dfs = thread_paths_[thread_index];
  dfs.push_back( start );
  Maze::Point cur = start;
  std::vector<int> random_direction_indices( generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  while ( !dfs.empty() ) {
    if ( escape_path_index_ != -1 ) {
      return;
    }
    cur = dfs.back();
    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ ) {
      if ( escape_path_index_ == -1 ) {
        escape_path_index_ = thread_index;
      }
      solver_mutex_.unlock();
      dfs.pop_back();
      return;
    }
    maze_[cur.row][cur.col] |= seen;
    maze_[cur.row][cur.col] |= paint;
    flush_cursor_path_coordinate( cur.row, cur.col );
    solver_mutex_.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );

    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator_ );
    bool found_branch_to_explore = false;
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = cardinal_directions_[i];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      solver_mutex_.lock();
      bool push_next
        = !( maze_[next.row][next.col] & seen ) && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
    }
    if ( !found_branch_to_explore ) {
      solver_mutex_.lock();
      maze_[cur.row][cur.col] &= ~paint;
      flush_cursor_path_coordinate( cur.row, cur.col );
      solver_mutex_.unlock();
      std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );
      dfs.pop_back();
    }
  }
}

void Thread_solvers::randomized_dfs_thread_gather( Maze::Point start, int thread_index, Thread_paint paint )
{
  Thread_cache seen = paint << thread_tag_offset_;
  std::vector<Maze::Point>& dfs = thread_paths_[thread_index];
  dfs.push_back( start );
  Maze::Point cur = start;
  std::vector<int> random_direction_indices( generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  while ( !dfs.empty() ) {
    cur = dfs.back();
    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ && !( maze_[cur.row][cur.col] & cache_mask_ ) ) {
      maze_[cur.row][cur.col] |= seen;
      dfs.pop_back();
      for ( const Maze::Point& p : dfs ) {
        maze_[p.row][p.col] |= paint;
      }
      solver_mutex_.unlock();
      return;
    }
    maze_[cur.row][cur.col] |= seen;
    solver_mutex_.unlock();
    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator_ );
    bool found_branch_to_explore = false;
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = cardinal_directions_[i];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      solver_mutex_.lock();
      bool push_next
        = !( maze_[next.row][next.col] & seen ) && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
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

void Thread_solvers::randomized_dfs_thread_gather_animated( Maze::Point start,
                                                            int thread_index,
                                                            Thread_paint paint )
{
  Thread_cache seen = paint << thread_tag_offset_;
  std::vector<Maze::Point>& dfs = thread_paths_[thread_index];
  dfs.push_back( start );
  Maze::Point cur = start;
  std::vector<int> random_direction_indices( generate_directions_.size() );
  std::iota( begin( random_direction_indices ), end( random_direction_indices ), 0 );
  while ( !dfs.empty() ) {
    cur = dfs.back();
    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ && !( maze_[cur.row][cur.col] & cache_mask_ ) ) {
      maze_[cur.row][cur.col] |= seen;
      solver_mutex_.unlock();
      dfs.pop_back();
      return;
    }
    maze_[cur.row][cur.col] |= seen;
    maze_[cur.row][cur.col] |= paint;
    flush_cursor_path_coordinate( cur.row, cur.col );
    solver_mutex_.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );

    shuffle( begin( random_direction_indices ), end( random_direction_indices ), generator_ );
    bool found_branch_to_explore = false;
    for ( const int& i : random_direction_indices ) {
      const Maze::Point& p = cardinal_directions_[i];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      solver_mutex_.lock();
      bool push_next
        = !( maze_[next.row][next.col] & seen ) && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        found_branch_to_explore = true;
        dfs.push_back( next );
        break;
      }
    }
    if ( !found_branch_to_explore ) {
      solver_mutex_.lock();
      maze_[cur.row][cur.col] &= ~paint;
      flush_cursor_path_coordinate( cur.row, cur.col );
      solver_mutex_.unlock();
      std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );
      dfs.pop_back();
    }
  }
}

/* * * * * * * * * * * * * * *        Breadth First Search       * * * * * * * * * * * * * * * * */

void Thread_solvers::solve_with_bfs_threads()
{
  if ( escape_path_index_ != -1 ) {
    clear_paths();
  }

  std::vector<std::thread> threads( cardinal_directions_.size() );
  if ( game_ == Maze_game::hunt ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread( [this, i, thread_mask] { bfs_thread_hunt( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::gather ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread( [this, i, thread_mask] { bfs_thread_gather( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::corners ) {
    std::vector<int> random_starting_corners( corner_starts_.size() );
    std::iota( begin( random_starting_corners ), end( random_starting_corners ), 0 );
    shuffle( begin( random_starting_corners ), end( random_starting_corners ), generator_ );
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      const Maze::Point& corner = corner_starts_[random_starting_corners[i]];
      threads[i] = std::thread( [this, &corner, i, thread_mask] { bfs_thread_hunt( corner, i, thread_mask ); } );
    }
  } else {
    std::cerr << "Uncategorized game slipped through initializations." << std::endl;
    std::abort();
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
  if ( game_ == Maze_game::gather ) {
    // Too chaotic to show all paths. So we will make a color flag near each finish.
    int thread = 0;
    for ( const std::vector<Maze::Point>& path : thread_paths_ ) {
      Thread_paint single_color = thread_masks_[thread++];
      const Maze::Point& p = path.front();
      maze_[p.row][p.col] &= ~thread_mask_;
      maze_[p.row][p.col] |= single_color;
    }
  } else {
    // It is cool to see the shortest path that the winning thread took to victory
    Thread_paint winner_color = thread_masks_[escape_path_index_];
    for ( const Maze::Point& p : thread_paths_[escape_path_index_] ) {
      maze_[p.row][p.col] &= ~thread_mask_;
      maze_[p.row][p.col] |= winner_color;
    }
  }
}

void Thread_solvers::animate_with_bfs_threads()
{
  if ( escape_path_index_ != -1 ) {
    clear_paths();
  }
  std::vector<std::thread> threads( cardinal_directions_.size() );
  if ( game_ == Maze_game::hunt ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread( [this, i, thread_mask] { bfs_thread_hunt_animated( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::gather ) {
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      threads[i] = std::thread( [this, i, thread_mask] { bfs_thread_gather_animated( start_, i, thread_mask ); } );
    }
  } else if ( game_ == Maze_game::corners ) {
    std::vector<int> random_starting_corners( corner_starts_.size() );
    std::iota( begin( random_starting_corners ), end( random_starting_corners ), 0 );
    shuffle( begin( random_starting_corners ), end( random_starting_corners ), generator_ );
    for ( int i = 0; i < num_threads_; i++ ) {
      const Thread_paint& thread_mask = thread_masks_[i];
      const Maze::Point& corner = corner_starts_[random_starting_corners[i]];
      threads[i]
        = std::thread( [this, &corner, i, thread_mask] { bfs_thread_hunt_animated( corner, i, thread_mask ); } );
    }
  } else {
    std::cerr << "Uncategorized game slipped through initializations." << std::endl;
    std::abort();
  }

  for ( std::thread& t : threads ) {
    t.join();
  }
  if ( game_ == Maze_game::gather ) {
    // Too chaotic to show all paths. So we will make a color flag near each finish.
    int thread = 0;
    for ( const std::vector<Maze::Point>& path : thread_paths_ ) {
      Thread_paint single_color = thread_masks_[thread++];
      const Maze::Point& p = path.front();
      maze_[p.row][p.col] &= ~thread_mask_;
      maze_[p.row][p.col] |= single_color;
      flush_cursor_path_coordinate( p.row, p.col );
    }
  } else {
    // It is cool to see the shortest path that the winning thread took to victory
    Thread_paint winner_color = thread_masks_[escape_path_index_];
    for ( const Maze::Point& p : thread_paths_[escape_path_index_] ) {
      maze_[p.row][p.col] &= ~thread_mask_;
      maze_[p.row][p.col] |= winner_color;
      flush_cursor_path_coordinate( p.row, p.col );
      std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );
    }
  }
}

void Thread_solvers::bfs_thread_hunt( Maze::Point start, int thread_index, Thread_paint paint )
{
  // This will be how we rebuild the path because queue does not represent the current path.
  std::unordered_map<Maze::Point, Maze::Point>& seen = thread_bfs_maps[thread_index];
  seen[start] = { -1, -1 };
  My_queue<Maze::Point>& bfs = thread_queues_[thread_index];
  bfs.push( start );
  Maze::Point cur = start;
  while ( !bfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( escape_path_index_ != -1 ) {
      break;
    }

    cur = bfs.front();
    bfs.pop();

    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ ) {
      if ( escape_path_index_ == -1 ) {
        escape_path_index_ = thread_index;
      }
      solver_mutex_.unlock();
      break;
    }
    // This creates a nice fanning out of mixed color for each searching thread.
    maze_[cur.row][cur.col] |= paint;
    solver_mutex_.unlock();

    // Bias each thread towards the direction it was dispatched when we first sent it.
    int direction_index = thread_index;
    do {
      const Maze::Point& p = cardinal_directions_[direction_index];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      bool seen_next = seen.count( next );
      solver_mutex_.lock();
      bool push_next = !seen_next && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != thread_index );
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    thread_paths_[thread_index].push_back( cur );
    cur = seen.at( cur );
  }
}

void Thread_solvers::bfs_thread_hunt_animated( Maze::Point start, int thread_index, Thread_paint paint )
{
  // This will be how we rebuild the path because queue does not represent the current path.
  std::unordered_map<Maze::Point, Maze::Point>& seen = thread_bfs_maps[thread_index];
  seen[start] = { -1, -1 };
  My_queue<Maze::Point>& bfs = thread_queues_[thread_index];
  bfs.push( start );
  Maze::Point cur = start;
  while ( !bfs.empty() ) {
    // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
    if ( escape_path_index_ != -1 ) {
      break;
    }

    cur = bfs.front();
    bfs.pop();

    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ ) {
      if ( escape_path_index_ == -1 ) {
        escape_path_index_ = thread_index;
      }
      solver_mutex_.unlock();
      break;
    }
    // This creates a nice fanning out of mixed color for each searching thread.
    maze_[cur.row][cur.col] |= paint;
    flush_cursor_path_coordinate( cur.row, cur.col );
    solver_mutex_.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );

    // Bias each thread towards the direction it was dispatched when we first sent it.
    int direction_index = thread_index;
    do {
      const Maze::Point& p = cardinal_directions_[direction_index];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      bool seen_next = seen.count( next );
      solver_mutex_.lock();
      bool push_next = !seen_next && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != thread_index );
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    thread_paths_[thread_index].push_back( cur );
    cur = seen.at( cur );
  }
}

void Thread_solvers::bfs_thread_gather( Maze::Point start, int thread_index, Thread_paint paint )
{
  std::unordered_map<Maze::Point, Maze::Point>& seen = thread_bfs_maps[thread_index];
  Thread_cache seen_bit = paint << 4;
  seen[start] = { -1, -1 };
  My_queue<Maze::Point>& bfs = thread_queues_[thread_index];
  bfs.push( start );
  Maze::Point cur = start;
  while ( !bfs.empty() ) {
    cur = bfs.front();
    bfs.pop();

    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ && !( maze_[cur.row][cur.col] & cache_mask_ ) ) {
      maze_[cur.row][cur.col] |= seen_bit;
      solver_mutex_.unlock();
      break;
    }
    maze_[cur.row][cur.col] |= paint;
    maze_[cur.row][cur.col] |= seen_bit;
    solver_mutex_.unlock();

    int direction_index = thread_index;
    do {
      const Maze::Point& p = cardinal_directions_[direction_index];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      bool seen_next = seen.count( next );
      solver_mutex_.lock();
      bool push_next = !seen_next && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != thread_index );
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    thread_paths_[thread_index].push_back( cur );
    cur = seen.at( cur );
  }
  escape_path_index_ = thread_index;
}

void Thread_solvers::bfs_thread_gather_animated( Maze::Point start, int thread_index, Thread_paint paint )
{
  std::unordered_map<Maze::Point, Maze::Point>& seen = thread_bfs_maps[thread_index];
  Thread_cache seen_bit = paint << 4;
  seen[start] = { -1, -1 };
  My_queue<Maze::Point>& bfs = thread_queues_[thread_index];
  bfs.push( start );
  Maze::Point cur = start;
  while ( !bfs.empty() ) {
    cur = bfs.front();
    bfs.pop();

    solver_mutex_.lock();
    if ( maze_[cur.row][cur.col] & finish_bit_ && !( maze_[cur.row][cur.col] & cache_mask_ ) ) {
      maze_[cur.row][cur.col] |= seen_bit;
      escape_path_index_ = thread_index;
      solver_mutex_.unlock();
      break;
    }
    maze_[cur.row][cur.col] |= paint;
    maze_[cur.row][cur.col] |= seen_bit;
    flush_cursor_path_coordinate( cur.row, cur.col );
    solver_mutex_.unlock();
    std::this_thread::sleep_for( std::chrono::microseconds( solver_speed_ ) );

    int direction_index = thread_index;
    do {
      const Maze::Point& p = cardinal_directions_[direction_index];
      Maze::Point next = { cur.row + p.row, cur.col + p.col };
      bool seen_next = seen.count( next );
      solver_mutex_.lock();
      bool push_next = !seen_next && ( maze_[next.row][next.col] & Maze::path_bit_ );
      solver_mutex_.unlock();
      if ( push_next ) {
        seen[next] = cur;
        bfs.push( next );
      }
      ++direction_index %= cardinal_directions_.size();
    } while ( direction_index != thread_index );
  }
  cur = seen.at( cur );
  while ( cur.row > 0 ) {
    thread_paths_[thread_index].push_back( cur );
    cur = seen.at( cur );
  }
}

Maze::Point Thread_solvers::pick_random_point()
{
  Maze::Point choice = { row_random_( generator_ ), col_random_( generator_ ) };
  if ( !( maze_[choice.row][choice.col] & Maze::path_bit_ ) || maze_[choice.row][choice.col] & finish_bit_
       || maze_[choice.row][choice.col] & start_bit_ ) {
    choice = find_nearest_square( choice );
  }
  return choice;
}

Maze::Point Thread_solvers::find_nearest_square( Maze::Point choice )
{
  // Fanning out from a starting point should work on any medium to large maze.
  for ( const Maze::Point& p : all_directions_ ) {
    Maze::Point next = { choice.row + p.row, choice.col + p.col };
    if ( next.row > 0 && next.row < maze_.row_size() - 1 && next.col > 0 && next.col < maze_.col_size() - 1
         && ( maze_[next.row][next.col] & Maze::path_bit_ ) && !( maze_[next.row][next.col] & start_bit_ )
         && !( maze_[next.row][next.col] & finish_bit_ ) ) {
      return next;
    }
  }
  // Getting desperate here. We should only need this for very small mazes.
  for ( int row = 1; row < maze_.row_size() - 1; row++ ) {
    for ( int col = 1; col < maze_.col_size() - 1; col++ ) {
      if ( ( maze_[row][col] & Maze::path_bit_ ) && !( maze_[row][col] & start_bit_ )
           && !( maze_[row][col] & finish_bit_ ) ) {
        return { row, col };
      }
    }
  }
  std::cerr << "Could not place a point. Bad point = "
            << "{" << choice.row << "," << choice.col << "}" << std::endl;
  print_maze();
  std::abort();
}

void Thread_solvers::clear_paths()
{
  escape_path_index_ = -1;
  for ( std::vector<Maze::Point>& vec : thread_paths_ ) {
    vec.clear();
    vec.reserve( starting_path_len_ );
  }
  maze_.clear_squares();
}

void Thread_solvers::clear_and_flush_paths() const
{
  clear_screen();
  print_maze();
}

void Thread_solvers::clear_screen() const
{
  std::cout << ansi_clear_screen_;
}

void Thread_solvers::print_maze() const
{
  for ( int row = 0; row < maze_.row_size(); row++ ) {
    for ( int col = 0; col < maze_.col_size(); col++ ) {
      print_point( row, col );
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

void Thread_solvers::flush_cursor_path_coordinate( int row, int col ) const
{
  set_cursor_point( row, col );
  print_point( row, col );
  std::cout << std::flush;
}

void Thread_solvers::print_point( int row, int col ) const
{
  const Maze::Square& square = maze_[row][col];
  if ( square & finish_bit_ ) {
    std::cout << ansi_finish_;
  } else if ( square & start_bit_ ) {
    std::cout << ansi_start_;
  } else if ( square & thread_mask_ ) {
    Thread_paint thread_color = ( square & thread_mask_ ) >> thread_tag_offset_;
    std::cout << thread_colors_[thread_color];
  } else {
    maze_.print_maze_square( row, col );
  }
}

void Thread_solvers::set_cursor_point( int row, int col ) const
{
  std::string cursor_pos = "\033[" + std::to_string( row + 1 ) + ";" + std::to_string( col + 1 ) + "f";
  std::cout << cursor_pos;
}

void Thread_solvers::print_solver() const
{
  std::cout << "Maze solved with ";
  if ( solver_ == Solver_algorithm::depth_first_search ) {
    std::cout << "Depth First Search\n";
  } else if ( solver_ == Solver_algorithm::breadth_first_search ) {
    std::cout << "Breadth First Search\n";
  } else if ( solver_ == Solver_algorithm::randomized_depth_first_search ) {
    std::cout << "Randomized Depth First Search\n";
  } else {
    std::cerr << "Maze solver is unset. ERROR." << std::endl;
    std::abort();
  }
}

void Thread_solvers::print_solution_path() const
{
  if ( game_ == Maze_game::gather ) {
    for ( const Thread_paint& mask : thread_masks_ ) {
      std::cout << thread_colors_.at( mask >> thread_tag_offset_ );
    }
    std::cout << " All threads found their finish squares!\n";
  } else {
    std::cout << thread_colors_.at( thread_masks_[escape_path_index_] >> thread_tag_offset_ ) << " thread won!"
              << "\n";
  }
}

void Thread_solvers::print_overlap_key() const
{
  std::cout << "┌────────────────────────────────────────────────────────────────┐\n"
            << "│     Overlap Key: 3_THREAD | 2_THREAD | 1_THREAD | 0_THREAD     │\n"
            << "├────────────┬────────────┬────────────┬────────────┬────────────┤\n"
            << "│ " << thread_colors_[1] << " = 0      │ " << thread_colors_[2] << " = 1      │ "
            << thread_colors_[3] << " = 1|0    │ " << thread_colors_[4] << " = 2      │ " << thread_colors_[5]
            << " = 2|0    │\n"
            << "├────────────┼────────────┼────────────┼────────────┼────────────┤\n"
            << "│ " << thread_colors_[6] << " = 2|1    │ " << thread_colors_[7] << " = 2|1|0  │ "
            << thread_colors_[8] << " = 3      │ " << thread_colors_[9] << " = 3|0    │ " << thread_colors_[10]
            << " = 3|1    │\n"
            << "├────────────┼────────────┼────────────┼────────────┼────────────┤\n"
            << "│ " << thread_colors_[11] << " = 3|1|0  │ " << thread_colors_[12] << " = 3|2    │ "
            << thread_colors_[13] << " = 3|2|0  │ " << thread_colors_[14] << " = 3|2|1  │ " << thread_colors_[15]
            << " = 3|2|1|0│\n"
            << "└────────────┴────────────┴────────────┴────────────┴────────────┘\n";
}
