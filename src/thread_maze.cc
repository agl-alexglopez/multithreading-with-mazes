#include "thread_maze.hh"
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <stack>
#include <queue>
#include <iostream>
#include <thread>
#include <vector>


Thread_maze::Thread_maze(const Thread_maze::Packaged_args& args)
    : builder_(args.builder),
      modification_(args.modification),
      solver_(args.solver),
      game_(args.game),
      style_(args.style),
      maze_(args.odd_rows, std::vector<Square>(args.odd_cols, 0)),
      maze_row_size_(maze_.size()),
      maze_col_size_(maze_[0].size()),
      generator_(std::random_device{}()),
      row_random_(1, maze_row_size_ - 2),
      col_random_(1, maze_col_size_ - 2),
      thread_paths_(num_threads_),
      start_({0,0}),
      finish_({0,0}),
      escape_path_index_(-1) {

    for (int row = 0; row < maze_row_size_; row++) {
        for (int col = 0; col < maze_col_size_; col++) {
            build_wall(row, col);
            add_modification(row, col);
        }
    }
    // If threads need to rely on heap for thread safe resizing, we slow parallelism.
    for (std::vector<Point>& vec : thread_paths_) {
        vec.reserve(starting_path_len_);
    }
    generate_maze(args.builder, args.game);
}

void Thread_maze::add_modification(int row, int col) {
    if (modification_ == Maze_modification::add_cross) {
        if ((row == maze_row_size_ / 2 && col > 1 && col < maze_col_size_ - 2)
                || (col == maze_col_size_ / 2 && row > 1 && row < maze_row_size_ - 2)) {
            build_path(row, col);
            if (col + 1 < maze_col_size_ - 2) {
                build_path(row, col + 1);
            }
        }
    } else if (modification_ == Maze_modification::add_x) {
        float row_size = maze_row_size_ - 2.0;
        float col_size = maze_col_size_ - 2.0;
        float cur_row = row;
        // y = mx + b. We will get the negative slope. This line goes top left to bottom right.
        float slope = (2.0 - row_size) / (2.0 - col_size);
        float b = 2.0 - (2.0 * slope);
        int on_line = (cur_row - b) / slope;
        if (col == on_line && col < maze_col_size_ - 2 && col > 1) {
            // An X is hard to notice and might miss breaking wall lines so make it wider.
            build_path(row, col);
            if (col + 1 < maze_col_size_ - 2) {
                build_path(row, col + 1);
            }
            if (col - 1 > 1) {
                build_path(row, col - 1);
            }
            if (col + 2 < maze_col_size_ - 2) {
                build_path(row, col + 2);
            }
            if (col - 2 > 1) {
                build_path(row, col - 2);
            }
        }
        // This line is drawn from top right to bottom left.
        slope = (2.0 - row_size) / (col_size - 2.0);
        b = row_size - (2.0 * slope);
        on_line = (cur_row - b) / slope;
        if (col == on_line && col > 1 && col < maze_col_size_ - 2 && row < maze_row_size_ - 2) {
            build_path(row, col);
            if (col + 1 < maze_col_size_ - 2) {
                build_path(row, col + 1);
            }
            if (col - 1 > 1) {
                build_path(row, col - 1);
            }
            if (col + 2 < maze_col_size_ - 2) {
                build_path(row, col + 2);
            }
            if (col - 2 > 1) {
                build_path(row, col - 2);
            }
        }
    }
}

void Thread_maze::generate_maze(Builder_algorithm algorithm, Maze_game game) {
    if (algorithm == Builder_algorithm::randomized_depth_first) {
        generate_randomized_dfs_maze();
    } else if (algorithm == Builder_algorithm::randomized_loop_erased) {
        generate_randomized_loop_erased_maze();
    } else if (algorithm == Builder_algorithm::arena) {
        generate_arena();
    } else if (algorithm == Builder_algorithm::randomized_grid) {
        generate_randomized_grid();
    } else {
        std::cerr << "Builder algorithm not set? Check for new builder algorithm." << std::endl;
        std::abort();
    }
    place_start_finish();
}

void Thread_maze::generate_randomized_loop_erased_maze() {
    /* Important to remember that this maze builds by jumping two squares at a time. Therefore for
     * Wilson's algorithm to work two points must both be even or both be odd to find each other.
     */
    start_ = {maze_row_size_ / 2, maze_col_size_ / 2};
    if (start_.row % 2 != 0) {
        start_.row++;
    }
    if (start_.col % 2 != 0) {
        start_.col++;
    }
    build_path(start_.row, start_.col);
    maze_[start_.row][start_.col] |= builder_bit_;
    Point maze_start = {2, 2};
    std::stack<Point> walk_stack({maze_start});
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    Point previous_square = {};
    while (!walk_stack.empty()) {
        Point cur = walk_stack.top();
        // The builder bit is only given to squares that are established as part of maze paths.
        if (maze_[cur.row][cur.col] & builder_bit_) {
            connect_walk_to_maze(walk_stack);
            cur = choose_arbitrary_point(Wilson_point::even);

            // Important return. If we can't pick a point we are done.
            if (!cur.row) {
                return;
            }

            walk_stack.push(cur);
            previous_square = {};
        } else if (maze_[cur.row][cur.col] & start_bit_) {
            erase_loop(walk_stack);
            // Manage this detail so we don't accidentally walk backwards and do this again.
            previous_square = {};
            walk_stack.pop();
            if (!walk_stack.empty()) {
                previous_square = walk_stack.top();
            }
            walk_stack.push(cur);
        }
        // Mark our progress on our current random walk. If we see this again we have looped.
        maze_[cur.row][cur.col] |= start_bit_;

        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        /* C++ does not allow named loop breaks and this says what I want directly rather than
         * adding a break and odd if branch to see if we should push or pop from the stack. You
         * may see this nested lambda style whenever I want an O(branch depth) recursive search.
         */
        [&] {
            for (const int& i : random_direction_indices) {
                const Point& p = generate_directions_[i];
                Point next = {cur.row + p.row, cur.col + p.col};
                // Do not check for seen squares, only the direction we just came from.
                if (next.row > 0 && next.row < maze_row_size_ - 1
                            && next.col > 0 && next.col < maze_col_size_ - 1
                                && next != previous_square) {
                    previous_square = cur;
                    walk_stack.push(next);
                    return;
                }
            }
            walk_stack.pop();
        }();
    }
}

void Thread_maze::connect_walk_to_maze(std::stack<Point>& walk_stack) {
    Point cur = walk_stack.top();
    walk_stack.pop();
    while (!walk_stack.empty()) {
        Point prev = walk_stack.top();
        // It is now desirable to run into this path as we complete future random walks.
        maze_[prev.row][prev.col] &= ~start_bit_;
        join_squares(cur, prev);
        cur = prev;
        walk_stack.pop();
    }
}

void Thread_maze::erase_loop(std::stack<Point>& walk_stack) {
    Point cur = walk_stack.top();
    walk_stack.pop();
    Point back = walk_stack.top();
    // We will forget we ever saw this loop as it may be part of a good walk later.
    while (back != cur) {
        maze_[back.row][back.col] &= ~start_bit_;
        walk_stack.pop();
        back = walk_stack.top();
    }
}

// Maze is built by jumping two squares at a time. Be careful to always be odd or even.
Thread_maze::Point Thread_maze::choose_arbitrary_point(Wilson_point start) const {
    int init = start == Wilson_point::even ? 2 : 1;
    for (int row = init; row < maze_row_size_ - 1; row += 2) {
        for (int col = init; col < maze_col_size_ - 1; col += 2) {
            Point cur = {row, col};
            if (!(maze_[cur.row][cur.col] & builder_bit_)) {
                return cur;
            }
        }
    }
    return {0,0};
}

void Thread_maze::generate_randomized_dfs_maze() {
    start_ = {row_random_(generator_), col_random_(generator_)};
    std::stack<Point> dfs({start_});
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    while (!dfs.empty()) {
        // Don't pop yet!
        Point cur = dfs.top();
        build_path(cur.row, cur.col);
        maze_[cur.row][cur.col] |= builder_bit_;
        // The unvisited neighbor is always random because array is re-shuffled each time.
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);

        /* A true depth first search needs to only push the current branch on the stack. The same
         * is possible without a lambda but we then need extra logic to check if we push or pop.
         */
        [&] {
            for (const int& i : random_direction_indices) {
                const Point& direction = generate_directions_[i];
                Point next = {cur.row + direction.row, cur.col + direction.col};
                if (next.row > 0 && next.row < maze_row_size_ - 1
                            && next.col > 0 && next.col < maze_col_size_ - 1
                                && !(maze_[next.row][next.col] & builder_bit_)) {
                    join_squares(cur, next);
                    dfs.push(next);
                    return;
                }
            }
            dfs.pop();
        }();
    }
}

void Thread_maze::generate_arena() {
    for (int row = 2; row < maze_row_size_ - 2; row++) {
        for (int col = 2; col < maze_col_size_ - 2; col++) {
            build_path(row, col);
        }
    }
}

void Thread_maze::generate_randomized_grid() {
    /* This value is the key to this algorithm. It is simply a randomized depth first search but we
     * force the algorithm to keep running in the random direction it chooses for run_limit amount.
     * Shorter run_limits converge on normal depth first search, longer create longer straights.
     */
    start_ = {row_random_(generator_), col_random_(generator_)};
    std::stack<Point> dfs({start_});
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    while (!dfs.empty()) {
        // Don't pop yet!
        Point cur = dfs.top();
        build_path(cur.row, cur.col);
        maze_[cur.row][cur.col] |= builder_bit_;
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        [&] {
            // Unvisited neighbor is always random because array is shuffled every time.
            for (const int& i : random_direction_indices) {
                // Choose another square that is two spaces a way.
                const Point& direction = generate_directions_[i];
                Point next = {cur.row + direction.row, cur.col + direction.col};
                if (next.row > 0 && next.row < maze_row_size_ - 1
                            && next.col > 0 && next.col < maze_col_size_ - 1
                                && !(maze_[next.row][next.col] & builder_bit_)) {
                    complete_run(dfs, cur, direction);
                    return;
                }
            }
            dfs.pop();
        }();
    }
}

void Thread_maze::complete_run(std::stack<Point>& dfs, Point cur, const Point& direction) {
    const int run_limit = 4;
    Point next = {cur.row + direction.row, cur.col + direction.col};
    Point cur_step = {};
    if (direction.row == -2) {
        cur_step = {-1,0};
    } else if (direction.col == 2) {
        cur_step = {0,1};
    } else if (direction.row == 2) {
        cur_step = {1,0};
    } else if (direction.col == -2) {
        cur_step = {0,-1};
    }
    // Create the "grid" by running in one direction until wall or limit.
    int cur_run = 0;
    while (next.row < maze_row_size_ - 1  && next.col < maze_col_size_ - 1
                && next.row > 0 && next.col > 0 && cur_run < run_limit) {
        cur.row += cur_step.row;
        cur.col += cur_step.col;
        build_path(cur.row, cur.col);
        maze_[cur.row][cur.col] |= builder_bit_;
        cur = next;

        build_path(next.row, next.col);
        maze_[next.row][next.col] |= builder_bit_;
        dfs.push(next);
        next.row += direction.row;
        next.col += direction.col;
        cur_run++;
    }
}

void Thread_maze::place_start_finish() {
    start_ = pick_random_point();
    maze_[start_.row][start_.col] |= start_bit_;
    int num_finishes = game_ == Maze_game::gather ? 4 : 1;
    for (int placement = 0; placement < num_finishes; placement++) {
        finish_ = pick_random_point();
        maze_[finish_.row][finish_.col] |= finish_bit_;
    }
}

void Thread_maze::join_squares(const Point& cur, const Point& next) {
    Point wall = cur;
    if (next.row < cur.row) {
        wall.row--;
    } else if (next.row > cur.row) {
        wall.row++;
    } else if (next.col < cur.col) {
        wall.col--;
    } else if (next.col > cur.col) {
        wall.col++;
    } else {
        std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
    }
    build_path(wall.row, wall.col);
    maze_[wall.row][wall.col] |= builder_bit_;
    build_path(next.row, next.col);
    maze_[next.row][next.col] |= builder_bit_;
}

void Thread_maze::build_wall(int row, int col) {
    Wall_line wall = 0b0;
    if (row - 1 >= 0) {
        wall |= north_wall_;
    }
    if (row + 1 < maze_row_size_) {
        wall |= south_wall_;
    }
    if (col - 1 >= 0) {
        wall |= west_wall_;
    }
    if (col + 1 < maze_col_size_) {
        wall |= east_wall_;
    }
    maze_[row][col] |= wall;
}

void Thread_maze::build_path(int row, int col) {
    if (row - 1 >= 0) {
        maze_[row - 1][col] &= ~south_wall_;
    }
    if (row + 1 < maze_row_size_) {
        maze_[row + 1][col] &= ~north_wall_;
    }
    if (col - 1 >= 0) {
        maze_[row][col - 1] &= ~east_wall_;
    }
    if (col + 1 < maze_col_size_) {
        maze_[row][col + 1] &= ~west_wall_;
    }
    maze_[row][col] |= path_bit_;
}

Thread_maze::Point Thread_maze::pick_random_point() {
    const int trouble_limit = 19;
    for (int attempt = 0; attempt < trouble_limit; attempt++) {
        Point choice = {row_random_(generator_), col_random_(generator_)};
        if ((maze_[choice.row][choice.col] & path_bit_)
                && !(maze_[choice.row][choice.col] & finish_bit_)
                    && !(maze_[choice.row][choice.col] & start_bit_)) {
            return choice;
        }
    }
    std::cerr << "Could not place point. Maze error or epicly bad luck. Run again." << std::endl;
    std::abort();
}

Thread_maze::Point Thread_maze::find_nearest_square(Thread_maze::Point choice) {
    for (const Point& p : all_directions_) {
        Point next = {choice.row + p.row, choice.col + p.col};
        if (next.row > 0 && next.row < maze_row_size_ - 1
                && next.col > 0 && next.col < maze_col_size_ - 1
                            && (maze_[next.row][next.col] & path_bit_)) {
            return next;
        }
    }
    std::cerr << "Could not place a point. Bad point = "
              << "{" << choice.row << "," << choice.col << "}" << std::endl;
    std::abort();
    print_maze();
}

void Thread_maze::solve_maze(Thread_maze::Solver_algorithm solver) {
    if (solver == Solver_algorithm::depth_first_search) {
        solve_with_dfs_threads();
    } else if (solver == Solver_algorithm::randomized_depth_first_search) {
        solve_with_randomized_dfs_threads();
    } else if (solver == Solver_algorithm::breadth_first_search) {
        solve_with_bfs_threads();
    } else {
        std::cerr << "Invalid solver?" << std::endl;
        abort();
    }
}

void Thread_maze::solve_maze() {
    solve_maze(solver_);
}

void Thread_maze::solve_with_dfs_threads() {
    if (escape_path_index_ != -1) {
        clear_paths();
    }
    std::vector<std::thread> threads(cardinal_directions_.size());
    for (int i = 0; i < num_threads_; i++) {
        const Thread_paint& thread_mask = thread_masks_[i];
        if (game_ == Maze_game::hunt) {
            threads[i] = std::thread([this, i, thread_mask] {
                dfs_thread_hunt(start_, i, thread_mask);
            });
        } else {
            threads[i] = std::thread([this, i, thread_mask] {
                dfs_thread_gather(start_, i, thread_mask);
            });
        }
    }
    for (std::thread& t : threads) {
        t.join();
    }
    print_solution_path();
}

void Thread_maze::solve_with_randomized_dfs_threads() {
    if (escape_path_index_ != -1) {
        clear_paths();
    }
    std::vector<std::thread> threads(cardinal_directions_.size());
    for (int i = 0; i < num_threads_; i++) {
        const Thread_paint& thread_mask = thread_masks_[i];
        if (game_ == Maze_game::hunt) {
            threads[i] = std::thread([this, i, thread_mask] {
                randomized_dfs_thread_hunt(start_, i, thread_mask);
            });
        } else {
            threads[i] = std::thread([this, i, thread_mask] {
                randomized_dfs_thread_gather(start_, i, thread_mask);
            });
        }
    }
    for (std::thread& t : threads) {
        t.join();
    }
    print_solution_path();
}

void Thread_maze::solve_with_bfs_threads() {
    if (escape_path_index_ != -1) {
        clear_paths();
    }

    std::vector<std::thread> threads(cardinal_directions_.size());
    for (int i = 0; i < num_threads_; i++) {
        const Thread_paint& thread_mask = thread_masks_[i];
        if (game_ == Maze_game::hunt) {
            threads[i] = std::thread([this, i, thread_mask] {
                bfs_thread_hunt(start_, i, thread_mask);
            });
        } else {
            threads[i] = std::thread([this, i, thread_mask] {
                bfs_thread_gather(start_, i, thread_mask);
            });
        }
    }

    for (std::thread& t : threads) {
        t.join();
    }
    print_solution_path();
}

bool Thread_maze::dfs_thread_hunt(Point start, int thread_index, Thread_paint paint) {
    /* We have useful bits in a square. Each square can use a unique bit to track seen threads.
     * Each thread could maintain its own hashset, but this is much more space efficient. Use
     * the space the maze already occupies and provides.
     */
    Thread_cache seen = paint << thread_tag_offset_;
    // Each thread only needs enough space for an O(current path length) stack.
    std::stack<Point> dfs({start});
    bool result = false;
    Point cur = start;
    while (!dfs.empty()) {
        if (escape_path_index_ != -1) {
            result = false;
            break;
        }

        // Don't pop() yet!
        cur = dfs.top();

        if (maze_[cur.row][cur.col] & finish_bit_) {
            maze_mutex_.lock();
            bool tie_break = escape_path_index_ == -1;
            if (tie_break) {
                escape_path_index_ = thread_index;
            }
            maze_mutex_.unlock();
            result = tie_break;
            dfs.pop();
            break;
        }
        maze_mutex_.lock();
        maze_[cur.row][cur.col] |= seen;
        maze_mutex_.unlock();

        [&] {
            // Bias each thread's first choice towards orginal dispatch direction. More coverage.
            int direction_index = thread_index;
            do {
                const Point& p = cardinal_directions_[direction_index];
                Point next = {cur.row + p.row, cur.col + p.col};
                maze_mutex_.lock();
                if (!(maze_[next.row][next.col] & seen)
                        && (maze_[next.row][next.col] & path_bit_)) {
                    maze_mutex_.unlock();
                    // Emulate a true recursive dfs. Only push the current branch onto our stack.
                    dfs.push(next);
                    return;
                }
                maze_mutex_.unlock();
                ++direction_index %= cardinal_directions_.size();
            } while (direction_index != thread_index);
            dfs.pop();
        }();
    }
    // Another benefit of true depth first search is our stack holds path to exact location.
    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
        maze_mutex_.lock();
        maze_[cur.row][cur.col] |= paint;
        maze_mutex_.unlock();
    }
    return result;
}

bool Thread_maze::randomized_dfs_thread_hunt(Point start, int thread_index, Thread_paint paint) {
    Thread_cache seen = paint << thread_tag_offset_;
    std::stack<Point> dfs({start});
    bool result = false;
    Point cur = start;
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    while (!dfs.empty()) {
        if (escape_path_index_ != -1) {
            result = false;
            break;
        }

        // Don't pop() yet!
        cur = dfs.top();

        if (maze_[cur.row][cur.col] & finish_bit_) {
            maze_mutex_.lock();
            bool tie_break = escape_path_index_ == -1;
            if (tie_break) {
                escape_path_index_ = thread_index;
            }
            maze_mutex_.unlock();
            result = tie_break;
            dfs.pop();
            return;
        }
        maze_mutex_.lock();
        maze_[cur.row][cur.col] |= seen;
        maze_mutex_.unlock();

        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        [&] {
            for (const int& i : random_direction_indices) {
                const Point& p = cardinal_directions_[i];
                Point next = {cur.row + p.row, cur.col + p.col};
                maze_mutex_.lock();
                if (!(maze_[next.row][next.col] & seen)
                        && (maze_[next.row][next.col] & path_bit_)) {
                    maze_mutex_.unlock();
                    dfs.push(next);
                    break;
                }
                maze_mutex_.unlock();
            }
            dfs.pop();
        }();
    }
    // Another benefit of true depth first search is our stack holds path to exact location.
    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
        maze_mutex_.lock();
        maze_[cur.row][cur.col] |= paint;
        maze_mutex_.unlock();
    }
    return result;
}

bool Thread_maze::dfs_thread_gather(Point start, int thread_index, Thread_paint paint) {
    Thread_cache seen = paint << thread_tag_offset_;
    std::stack<Point> dfs({start});
    bool result = false;
    Point cur = start;
    while (!dfs.empty()) {
        cur = dfs.top();

        maze_mutex_.lock();
        // We are the first thread to this finish! Claim it!
        if (maze_[cur.row][cur.col] & finish_bit_
                && !(maze_[cur.row][cur.col] & cache_mask_)){
            maze_[cur.row][cur.col] |= seen;
            maze_mutex_.unlock();
            dfs.pop();
            break;
        }
        // Shoot, another thread beat us here. Mark and move on to another finish.
        maze_[cur.row][cur.col] |= seen;
        maze_mutex_.unlock();
        [&] {
            // Bias each thread's first choice towards orginal dispatch direction. More coverage.
            int direction_index = thread_index;
            do {
                const Point& p = cardinal_directions_[direction_index];
                Point next = {cur.row + p.row, cur.col + p.col};
                maze_mutex_.lock();
                if (!(maze_[next.row][next.col] & seen)
                        && (maze_[next.row][next.col] & path_bit_)) {
                    maze_mutex_.unlock();
                    dfs.push(next);
                    return;
                }
                maze_mutex_.unlock();
                ++direction_index %= cardinal_directions_.size();
            } while (direction_index != thread_index);
            dfs.pop();
        }();
    }
    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
        maze_mutex_.lock();
        maze_[cur.row][cur.col] |= paint;
        maze_mutex_.unlock();
    }
    return result;
}

bool Thread_maze::randomized_dfs_thread_gather(Point start, int thread_index, Thread_paint paint) {

    Thread_cache seen = paint << thread_tag_offset_;
    std::stack<Point> dfs({start});
    bool result = false;
    Point cur = start;
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    while (!dfs.empty()) {
        cur = dfs.top();

        maze_mutex_.lock();
        // We are the first thread to this finish! Claim it!
        if (maze_[cur.row][cur.col] & finish_bit_
                && !(maze_[cur.row][cur.col] & cache_mask_)){
            maze_[cur.row][cur.col] |= seen;
            maze_mutex_.unlock();
            result = true;
            dfs.pop();
            break;
        }
        // Shoot, another thread beat us here. Mark and move on to another finish.
        maze_[cur.row][cur.col] |= seen;
        maze_mutex_.unlock();
        // Bias each thread's first choice towards orginal dispatch direction. More coverage.
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        [&] {
            for (const int& i : random_direction_indices) {
                const Point& p = cardinal_directions_[i];
                Point next = {cur.row + p.row, cur.col + p.col};
                maze_mutex_.lock();
                if (!(maze_[next.row][next.col] & seen)
                        && (maze_[next.row][next.col] & path_bit_)) {
                    maze_mutex_.unlock();
                    dfs.push(next);
                    return;
                }
                maze_mutex_.unlock();
            };
            dfs.pop();
        }();
    }
    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
        maze_mutex_.lock();
        maze_[cur.row][cur.col] |= paint;
        maze_mutex_.unlock();
    }
    return result;
}

bool Thread_maze::bfs_thread_hunt(Point start, int thread_index, Thread_paint paint) {
    // This will be how we rebuild the path because queue does not represent the current path.
    std::unordered_map<Point,Point> seen;
    seen[start] = {-1,-1};
    std::queue<Point> bfs({start});
    bool result = false;
    Point cur = start;
    while (!bfs.empty()) {
        // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
        if (escape_path_index_ != -1) {
            result = false;
            break;
        }

        cur = bfs.front();
        bfs.pop();

        if (maze_[cur.row][cur.col] & finish_bit_) {
            maze_mutex_.lock();
            bool tie_break = escape_path_index_ == -1;
            if (tie_break) {
                escape_path_index_ = thread_index;
            }
            maze_mutex_.unlock();
            result = tie_break;
            break;
        }
        // This creates a nice fanning out of mixed color for each searching thread.
        maze_mutex_.lock();
        maze_[cur.row][cur.col] |= paint;
        maze_mutex_.unlock();

        // Bias each thread towards the direction it was dispatched when we first sent it.
        int direction_index = thread_index;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            if (!seen.count(next) && (maze_[next.row][next.col] & path_bit_)) {
                seen[next] = cur;
                bfs.push(next);
            }
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_index);
    }
    cur = seen.at(cur);
    while(cur.row > 0) {
        thread_paths_[thread_index].push_back(cur);
        cur = seen.at(cur);
    }
    return result;
}

bool Thread_maze::bfs_thread_gather(Point start, int thread_index, Thread_paint paint) {
    std::unordered_map<Point,Point> seen;
    Thread_cache seen_bit = paint << 4;
    seen[start] = {-1,-1};
    std::queue<Point> bfs({start});
    bool result = false;
    Point cur = start;
    while (!bfs.empty()) {
        cur = bfs.front();
        bfs.pop();

        maze_mutex_.lock();
        if (maze_[cur.row][cur.col] & finish_bit_) {
            if (!(maze_[cur.row][cur.col] & cache_mask_)) {
                maze_[cur.row][cur.col] |= seen_bit;
                maze_mutex_.unlock();
                break;
            }
        }
        maze_[cur.row][cur.col] |= paint;
        maze_[cur.row][cur.col] |= seen_bit;
        maze_mutex_.unlock();

        int direction_index = thread_index;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            if (!seen.count(next) && (maze_[next.row][next.col] & path_bit_)) {
                seen[next] = cur;
                bfs.push(next);
            }
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_index);
    }
    cur = seen.at(cur);
    while(cur.row > 0) {
        thread_paths_[thread_index].push_back(cur);
        cur = seen.at(cur);
    }
    return result;
}

void Thread_maze::print_solution_path() {
    std::cout << "\n";
    print_maze();
    if (game_ == Maze_game::hunt) {
        std::cout << thread_colors_.at(thread_masks_[escape_path_index_] >> thread_tag_offset_)
                  << "█" << " thread won!" << ansi_nil_ << "\n";
    } else if (game_ == Maze_game::gather) {
        for (const Thread_paint& mask : thread_masks_) {
            std::cout << thread_colors_.at(mask >> thread_tag_offset_) << "█" << ansi_nil_;
        }
        std::cout << " All threads found their finish squares!\n";
    }
    std::cout << "Maze generated with ";
    if (builder_ == Builder_algorithm::randomized_depth_first) {
        std::cout << "Randomized Depth First Search\n";
    } else if (builder_ == Builder_algorithm::randomized_loop_erased) {
        std::cout << "Loop-Erased Random Walks\n";
    } else if (builder_ == Builder_algorithm::randomized_grid) {
        std::cout << "Randomized Grid Runs\n";
    } else if (builder_ == Builder_algorithm::arena) {
        std::cout << "Arena\n";
    } else {
        std::cerr << "Maze builder is unset. ERROR." << std::endl;
        std::abort();
    }

    std::cout << "Maze solved with ";
    if (solver_ == Solver_algorithm::depth_first_search) {
        std::cout << "Depth First Search\n";
    } else if (solver_ == Solver_algorithm::breadth_first_search) {
        std::cout << "Breadth First Search\n";
    } else if (solver_ == Solver_algorithm::randomized_depth_first_search) {
        std::cout << "Randomized Depth First Search\n";
    } else {
        std::cerr << "Maze solver is unset. ERROR." << std::endl;
        std::abort();
    }
    std::cout << std::endl;
}

void Thread_maze::print_maze() const {
    print_overlap_key();
    const std::array<const char *const,16>& lines = wall_styles_[static_cast<size_t>(style_)];
    for (int row = 0; row < maze_row_size_; row++) {
        for (int col = 0; col < maze_col_size_; col++) {
            const Square& square = maze_[row][col];
            if (square & finish_bit_) {
                std::cout << ansi_bold_ << ansi_cyn_ << "F" << ansi_nil_;
            } else if (square & start_bit_) {
                std::cout << ansi_bold_ << ansi_cyn_ << "S" << ansi_nil_;
            } else if (square & thread_mask_) {
                Thread_paint thread_color = (square & thread_mask_) >> thread_tag_offset_;
                std::cout << thread_colors_[thread_color] << "█" << ansi_nil_;
            } else if (!(square & path_bit_)) {
                std::cout << lines[square & wall_mask_];
            } else if (square & path_bit_) {
                std::cout << " ";
            } else {
                std::cerr << "Printed maze and a square was not categorized." << std::endl;
                abort();
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

void Thread_maze::print_overlap_key() const {
    const std::string d = "█████████████";
    const char *const n = ansi_nil_;
    std:: cout << "┌─────────────────────────────────────────────────────────────────────┐\n"
               << "│  Overlapping Color Key: 3_THREAD | 2_THREAD | 1_THREAD | 0_THREAD   │\n"
               << "├─────────────┬─────────────┬─────────────┬─────────────┬─────────────┤\n"
               << "│     0       │     1       │    1|0      │     2       │     2|0     │\n"
               << "├─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
               << "│"<<thread_colors_[1]<<d<<n<<"│"<<thread_colors_[2]<<d<<n<<"│"<<thread_colors_[3]<<d<<n<<"│"<<thread_colors_[4]<<d<<n<<"│"<<thread_colors_[5]<<d<<n<<"│\n"
               << "├─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
               << "│    2|1      │   2|1|0     │     3       │    3|0      │     3|1     │\n"
               << "├─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
               << "│"<<thread_colors_[6]<<d<<n<<"│"<<thread_colors_[7]<<d<<n<<"│"<<thread_colors_[8]<<d<<n<<"│"<<thread_colors_[9]<<d<<n<<"│"<<thread_colors_[10]<<d<<n<<"│\n"
               << "├─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
               << "│    3|1|0    │    3|2      │   3|2|0     │   3|2|1     │   3|2|1|0   │\n"
               << "├─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
               << "│"<<thread_colors_[11]<<d<<n<<"│"<<thread_colors_[12]<<d<<n<<"│"<<thread_colors_[13]<<d<<n<<"│"<<thread_colors_[14]<<d<<n<<"│"<<thread_colors_[15]<<d<<n<<"│\n"
               << "└─────────────┴─────────────┴─────────────┴─────────────┴─────────────┘\n"
               << std::endl;
}

void Thread_maze::clear_paths() {
    escape_path_index_ = -1;
    for (std::vector<Point>& vec : thread_paths_) {
        vec.clear();
        vec.reserve(starting_path_len_);
    }
    for (std::vector<Square>& row : maze_) {
        for (Square& square : row) {
            square &= ~clear_cache_;
        }
    }
}

void Thread_maze::new_maze() {
    generator_.seed(std::random_device{}());
    escape_path_index_ = -1;
    for (std::vector<Point>& vec : thread_paths_) {
        vec.clear();
        vec.reserve(starting_path_len_);
    }
    generate_maze(builder_, game_);
}

void Thread_maze::new_maze(Builder_algorithm builder,
                           Maze_game game,
                           size_t odd_rows,
                           size_t odd_cols) {
    generator_.seed(std::random_device{}());
    builder_ = builder;
    escape_path_index_ = -1;
    for (std::vector<Point>& vec : thread_paths_) {
        vec.clear();
        vec.reserve(starting_path_len_);
    }
    generate_maze(builder, game);
}

bool operator==(const Thread_maze::Point& lhs, const Thread_maze::Point& rhs) {
    return lhs.row == rhs.row && lhs.col == rhs.col;
}

bool operator!=(const Thread_maze::Point& lhs, const Thread_maze::Point& rhs) {
    return !(lhs == rhs);
}

size_t Thread_maze::size() const {
    return maze_row_size_;
}

std::vector<Thread_maze::Square>& Thread_maze::operator[](size_t index) {
    return maze_[index];
}

const std::vector<Thread_maze::Square>& Thread_maze::operator[](size_t index) const {
    return maze_[index];
}

