#include "thread_maze.hh"
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <cstdint>
#include <cstdlib>
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
      generator_(std::random_device{}()),
      maze_(args.odd_rows, std::vector<Square>(args.odd_cols, 0)),
      thread_paths_(num_threads_),
      start_({0,0}),
      finish_({0,0}),
      escape_path_index_(-1) {

    for (size_t row = 0; row < maze_.size(); row++) {
        for (size_t col = 0; col < maze_[0].size(); col++) {
            build_wall(row, col);
            add_modification(row, col);
        }
    }
    // If threads need to rely on heap for thread safe resizing, we slow parallelism.
    for (std::vector<Point>& vec : thread_paths_) {
        vec.reserve(starting_path_len_);
    }
    generate_maze(args.builder, args.game, args.odd_rows, args.odd_cols);
}

void Thread_maze::add_modification(size_t row, size_t col) {
    if (modification_ == Maze_modification::add_cross) {
        if ((row == maze_.size() / 2 && col > 1 && col < maze_[0].size() - 2)
                || (col == maze_[0].size() / 2 && row > 1 && row < maze_.size() - 2)) {
            build_path(row, col);
            if (col + 1 < maze_[0].size() - 2) {
                build_path(row, col + 1);
            }
        }
    } else if (modification_ == Maze_modification::add_x) {
        float row_size = maze_.size() - 2.0;
        float col_size = maze_[0].size() - 2.0;
        float cur_row = row;
        // y = mx + b. We will get the negative slope. This line goes top left to bottom right.
        float slope = (2.0 - row_size) / (2.0 - col_size);
        float b = 2.0 - (2.0 * slope);
        int nearest_neg_point = (cur_row - b) / slope;
        if (col == nearest_neg_point && col < maze_[0].size() - 2 && col > 1) {
            // An X is hard to notice and might miss breaking wall lines so make it wider.
            build_path(row, col);
            if (col + 1 < maze_[0].size() - 2) {
                build_path(row, col + 1);
            }
            if (col - 1 > 1) {
                build_path(row, col - 1);
            }
            if (col + 2 < maze_[0].size() - 2) {
                build_path(row, col + 2);
            }
            if (col - 2 > 1) {
                build_path(row, col - 2);
            }
        }
        // This line is drawn from top right to bottom left.
        slope = (2.0 - row_size) / (col_size - 2.0);
        b = row_size - (2.0 * slope);
        int nearest_pos_point = (cur_row - b) / slope;
        if (col == nearest_pos_point
                && row < maze_.size() - 2
                    && col < maze_[0].size() - 2
                        && col > 1) {
            build_path(row, col);
            if (col + 1 < maze_[0].size() - 2) {
                build_path(row, col + 1);
            }
            if (col - 1 > 1) {
                build_path(row, col - 1);
            }
            if (col + 2 < maze_[0].size() - 2) {
                build_path(row, col + 2);
            }
            if (col - 2 > 1) {
                build_path(row, col - 2);
            }
        }
    } else if (builder_ == Builder_algorithm::arena) {
        if (row > 1 && col > 1 && row < maze_.size() - 2 && col < maze_[0].size() - 2) {
            build_path(row, col);
        }
    }
}

void Thread_maze::solve_maze(Thread_maze::Solver_algorithm solver) {
    if (solver == Solver_algorithm::depth_first_search) {
        solve_with_dfs_threads();
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

bool Thread_maze::dfs_thread_hunt(Point start, size_t thread_index, Thread_paint thread_bit) {
    /* We have useful bits in a square. Each square can use a unique bit to track seen threads.
     * Each thread could maintain its own hashset, but this is much more space efficient. Use
     * the space the maze already occupies and provides.
     */
    Thread_cache seen = thread_bit << thread_tag_offset_;
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

        Point chosen = {};
        // Bias each thread's first choice towards orginal dispatch direction. More coverage.
        int direction_index = thread_index;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen) && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                chosen = next;
                break;
            }
            maze_mutex_.unlock();
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_index);
        // Emulate a true recursive dfs. Only push the current branch onto our stack.
        chosen.row ? dfs.push(chosen) : dfs.pop();
    }
    // Another benefit of true depth first search is our stack holds path to exact location.
    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
        maze_mutex_.lock();
        maze_[cur.row][cur.col] |= thread_bit;
        // A thread or threads have already arrived at this location. Mark as an overlap.
        maze_mutex_.unlock();
    }
    return result;
}

bool Thread_maze::dfs_thread_gather(Point start, size_t thread_index, Thread_paint thread_bit) {
    Thread_cache seen = thread_bit << thread_tag_offset_;
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

        Point chosen = {};
        // Bias each thread's first choice towards orginal dispatch direction. More coverage.
        int direction_index = thread_index;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen) && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                chosen = next;
                break;
            }
            maze_mutex_.unlock();
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_index);
        chosen.row ? dfs.push(chosen) : dfs.pop();
    }
    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
        maze_mutex_.lock();
        maze_[cur.row][cur.col] |= thread_bit;
        maze_mutex_.unlock();
    }
    return result;
}

bool Thread_maze::bfs_thread_hunt(Point start, size_t thread_index, Thread_paint thread_bit) {
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
        maze_[cur.row][cur.col] |= thread_bit;
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

bool Thread_maze::bfs_thread_gather(Point start, size_t thread_index, Thread_paint thread_bit) {
    std::unordered_map<Point,Point> seen;
    Thread_cache seen_bit = thread_bit << 4;
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
        maze_[cur.row][cur.col] |= thread_bit;
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

void Thread_maze::generate_maze(Builder_algorithm algorithm, Maze_game game,
                                size_t odd_rows, size_t odd_cols) {
    if (algorithm == Builder_algorithm::randomized_depth_first) {
        generate_randomized_dfs_maze(game, odd_rows, odd_cols);
    } else if (algorithm == Builder_algorithm::randomized_loop_erased) {
        generate_randomized_loop_erased_maze(game, odd_rows, odd_cols);
    } else if (algorithm == Builder_algorithm::arena) {
        std::uniform_int_distribution<int> row_gen(1, maze_.size() - 2);
        std::uniform_int_distribution<int> col_gen(1, maze_[0].size() - 2);
        start_ = pick_random_point(row_gen, col_gen);
        finish_ = pick_random_point(row_gen, col_gen);
        maze_[start_.row][start_.col] |= start_bit_;
        maze_[finish_.row][finish_.col] |= finish_bit_;
        if (game == Maze_game::gather) {
            finish_ = pick_random_point(row_gen, col_gen);
            maze_[finish_.row][finish_.col] |= finish_bit_;
            finish_ = pick_random_point(row_gen, col_gen);
            maze_[finish_.row][finish_.col] |= finish_bit_;
            finish_ = pick_random_point(row_gen, col_gen);
            maze_[finish_.row][finish_.col] |= finish_bit_;
        }
    } else {
        std::cerr << "Invalid builder arg, somehow?" << std::endl;
        std::abort();
    }
}

void Thread_maze::generate_randomized_loop_erased_maze(Maze_game game,
                                                       size_t odd_rows, size_t odd_cols) {
    int total_squares = (odd_rows - 2) * (odd_cols - 2);
    std::uniform_int_distribution<int> row_gen(1, maze_.size() - 2);
    std::uniform_int_distribution<int> col_gen(1, maze_[0].size() - 2);
    start_ = {row_gen(generator_), col_gen(generator_)};
    build_path(start_.row, start_.col);
    Point maze_start = {row_gen(generator_), col_gen(generator_)};
    std::stack<Point> dfs({maze_start});
    std::vector<int> random_direction_indices(generate_directions_.size());
    iota(begin(random_direction_indices), end(random_direction_indices), 0);
    Point prev = {};
    while (total_squares > 0 && !dfs.empty()) {
        Point cur = dfs.top();
        if (maze_[cur.row][cur.col] & builder_bit_) {
            while (!dfs.empty()) {
                cur = dfs.top();
                maze_[cur.row][cur.col] |= builder_bit_;
                build_path(cur.row, cur.col);
                total_squares--;
                dfs.pop();
            }
            cur = choose_arbitrary_point();
            dfs.push(cur);
            prev = {};
        } else if (maze_[cur.row][cur.col] & zero_seen_) {
            dfs.pop();
            Point back = dfs.top();
            while (back != cur) {
                maze_[back.row][back.col] &= ~zero_seen_;
                dfs.pop();
            }
            prev = {};
        }
        maze_[cur.row][cur.col] |= zero_seen_;

        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        Point chose = {};
        for (const int& i : random_direction_indices) {
            const Point& p = cardinal_directions_[i];
            Point next = {cur.row + p.row, cur.col + p.col};
            if (next.row > 0 && static_cast<size_t>(next.row) < maze_.size() - 1
                        && next.col > 0 && static_cast<size_t>(next.col) < maze_[0].size() - 1
                            && p.row + prev.row != 0 && p.col + prev.col != 0) {
                chose = next;
                break;
            }
        }
        prev = cur;
        chose.row ? dfs.push(chose) : dfs.pop();
    }
    for (std::vector<Square>& row : maze_) {
        for (Square& square : row) {
            square &= ~clear_cache_;
        }
    }
    start_ = pick_random_point(row_gen, col_gen);
    finish_ = pick_random_point(row_gen, col_gen);
    maze_[start_.row][start_.col] |= start_bit_;
    maze_[finish_.row][finish_.col] |= finish_bit_;
    if (game == Maze_game::gather) {
        finish_ = pick_random_point(row_gen, col_gen);
        maze_[finish_.row][finish_.col] |= finish_bit_;
        finish_ = pick_random_point(row_gen, col_gen);
        maze_[finish_.row][finish_.col] |= finish_bit_;
        finish_ = pick_random_point(row_gen, col_gen);
        maze_[finish_.row][finish_.col] |= finish_bit_;
    }
}

Thread_maze::Point
Thread_maze::choose_arbitrary_point() const {
    for (int row = 1; row < maze_.size() - 1; row++) {
        for (int col = 1; col < maze_[0].size() - 1; col++) {
            Point cur = {row, col};
            if (!(maze_[cur.row][cur.col] & builder_bit_)
                    && maze_[cur.row][cur.col] & path_bit_) {
                return cur;
            }
        }
    }
    return {0,0};
}

void Thread_maze::generate_randomized_dfs_maze(Maze_game game,
                                               size_t odd_rows, size_t odd_cols) {
    std::uniform_int_distribution<int> row_gen(1, maze_.size() - 2);
    std::uniform_int_distribution<int> col_gen(1, maze_[0].size() - 2);
    start_ = {row_gen(generator_), col_gen(generator_)};
    std::stack<Point> dfs;
    dfs.push(start_);
    std::vector<int> random_direction_indices(generate_directions_.size());
    iota(begin(random_direction_indices), end(random_direction_indices), 0);
    while (!dfs.empty()) {
        // Don't pop yet!
        Point cur = dfs.top();
        build_path(cur.row, cur.col);
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);

        // The unvisited neighbor is guaranteed to be random because array is shuffled every time.
        Point random_neighbor = {};
        for (const int& i : random_direction_indices) {
            // Choose another square that is two spaces a way.
            const Point& direction = generate_directions_[i];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            if (next.row > 0 && static_cast<size_t>(next.row) < maze_.size() - 1
                        && next.col > 0 && static_cast<size_t>(next.col) < maze_[0].size() - 1
                            && !(maze_[next.row][next.col] & builder_bit_)) {
                random_neighbor = next;
                break;
            }
        }

        // We have seen all the neighbors for this current square. Safe to pop()
        if (!random_neighbor.row) {
            dfs.pop();
            continue;
        }

        // We have a randomly chosen neighbor so let's connect and continue.
        maze_[random_neighbor.row][random_neighbor.col] |= builder_bit_;
        dfs.push(random_neighbor);
        build_path(random_neighbor.row, random_neighbor.col);
        // Now we must break the wall between this two square jump.
        if (random_neighbor.row < cur.row) {
            random_neighbor.row++;
        } else if (random_neighbor.row > cur.row) {
            random_neighbor.row--;
        } else if (random_neighbor.col < cur.col) {
            random_neighbor.col++;
        } else if (random_neighbor.col > cur.col) {
            random_neighbor.col--;
        } else {
            std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
        }
        build_path(random_neighbor.row, random_neighbor.col);
        // Walls are just a simple value that a square can have so I need to mark as seen.
        maze_[random_neighbor.row][random_neighbor.col] |= builder_bit_;
    }
    start_ = pick_random_point(row_gen, col_gen);
    finish_ = pick_random_point(row_gen, col_gen);
    maze_[start_.row][start_.col] |= start_bit_;
    maze_[finish_.row][finish_.col] |= finish_bit_;
    if (game == Maze_game::gather) {
        finish_ = pick_random_point(row_gen, col_gen);
        maze_[finish_.row][finish_.col] |= finish_bit_;
        finish_ = pick_random_point(row_gen, col_gen);
        maze_[finish_.row][finish_.col] |= finish_bit_;
        finish_ = pick_random_point(row_gen, col_gen);
        maze_[finish_.row][finish_.col] |= finish_bit_;
    }
}

void Thread_maze::build_wall(int row, int col) {
    Wall_line wall = 0b0;
    if (row - 1 >= 0) {
        wall |= north_wall_;
    }
    if (row + 1 < maze_.size()) {
        wall |= south_wall_;
    }
    if (col - 1 >= 0) {
        wall |= west_wall_;
    }
    if (col + 1 < maze_[0].size()) {
        wall |= east_wall_;
    }
    maze_[row][col] |= wall;
}

void Thread_maze::build_path(int row, int col) {
    if (row - 1 >= 0) {
        maze_[row - 1][col] &= ~south_wall_;
    }
    if (row + 1 < maze_.size()) {
        maze_[row + 1][col] &= ~north_wall_;
    }
    if (col - 1 >= 0) {
        maze_[row][col - 1] &= ~east_wall_;
    }
    if (col + 1 < maze_[0].size()) {
        maze_[row][col + 1] &= ~west_wall_;
    }
    maze_[row][col] |= path_bit_;
}

Thread_maze::Point Thread_maze::pick_random_point(std::uniform_int_distribution<int>& row,
                                                  std::uniform_int_distribution<int>& col) {
    const int trouble_limit = 19;
    for (int attempt = 0; attempt < trouble_limit; attempt++) {
        Point choice = {row(generator_), col(generator_)};
        if ((maze_[choice.row][choice.col] & path_bit_)
                && !(maze_[choice.row][choice.col] & finish_bit_)
                    && !(maze_[choice.row][choice.col] & start_bit_)) {
            return choice;
        }
    }
    std::cerr << "Could not place point. Maze error or insanely bad rng. Run again." << std::endl;
    std::abort();
}

Thread_maze::Point Thread_maze::find_nearest_square(Thread_maze::Point choice) {
    for (const Point& p : all_directions_) {
        Point next = {choice.row + p.row, choice.col + p.col};
        if (next.row > 0 && static_cast<size_t>(next.row) < maze_.size() - 1
                && next.col > 0 && static_cast<size_t>(next.col) < maze_[0].size() - 1
                            && (maze_[next.row][next.col] & path_bit_)) {
            return next;
        }
    }
    std::cerr << "Could not place a point. Bad point = "
              << "{" << choice.row << "," << choice.col << "}" << std::endl;
    std::abort();
    print_maze();
}

Thread_maze::Point Thread_maze::find_nearest_wall(Thread_maze::Point choice) {
    // s    se     e     sw     w      nw       w
    for (const Point& p : all_directions_) {
         Point next = {choice.row + p.row, choice.col + p.col};
         if (next.row > 0 && static_cast<size_t>(next.row) < maze_.size() - 1
                 && next.col > 0 && static_cast<size_t>(next.col) < maze_[0].size() - 1
                    && !(maze_[next.row][next.col] & path_bit_)) {
             return next;
         }
    }
    return {0,0};
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
        std::cout << "Loop-Erased Random Walk\n";
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
    } else {
        std::cerr << "Maze solver is unset. ERROR." << std::endl;
        std::abort();
    }
    std::cout << std::endl;
}

void Thread_maze::print_maze() const {
    print_overlap_key();
    const std::array<const char *const,16>& lines = wall_styles_[static_cast<size_t>(style_)];
    for (size_t row = 0; row < maze_.size(); row++) {
        for (size_t col = 0; col < maze_[0].size(); col++) {
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
    generate_maze(builder_, game_, maze_.size(), maze_[0].size());
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
    generate_maze(builder, game, odd_rows, odd_cols);
}

bool operator==(const Thread_maze::Point& lhs, const Thread_maze::Point& rhs) {
    return lhs.row == rhs.row && lhs.col == rhs.col;
}

bool operator!=(const Thread_maze::Point& lhs, const Thread_maze::Point& rhs) {
    return !(lhs == rhs);
}

size_t Thread_maze::size() const {
    return maze_.size();
}

std::vector<Thread_maze::Square>& Thread_maze::operator[](size_t index) {
    return maze_[index];
}

const std::vector<Thread_maze::Square>& Thread_maze::operator[](size_t index) const {
    return maze_[index];
}

