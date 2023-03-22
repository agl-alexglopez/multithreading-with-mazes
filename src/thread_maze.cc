#include "thread_maze.hh"
#include <algorithm>
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
      solver_(args.solver),
      generator_(std::random_device{}()),
      thread_paths_(num_threads_),
      start_({0,0}),
      finish_({0,0}),
      escape_path_index_(-1) {
    generate_maze(args.builder, args.odd_rows, args.odd_cols);
    // If threads need to rely on heap for thread safe resizing, we slow parallelism.
    for (std::vector<Point>& vec : thread_paths_) {
        vec.reserve(starting_path_len_);
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
        const Point& direction = cardinal_directions_[i];
        Point thread_start = {start_.row + direction.row, start_.col + direction.col};
        if (!(maze_[thread_start.row][thread_start.col] & path_bit_)) {
            thread_start = start_;
        }
        const Thread_tag& thread_mask = thread_masks_[i];
        threads[i] = std::thread([this, thread_start, i, thread_mask] {
            dfs_thread_search(thread_start, i, thread_mask);
        });

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
        const Point& direction = cardinal_directions_[i];
        Point thread_start = {start_.row + direction.row, start_.col + direction.col};
        if (!(maze_[thread_start.row][thread_start.col] & path_bit_)) {
            thread_start = start_;
        }
        const Thread_tag& thread_mask = thread_masks_[i];
        threads[i] = std::thread([this, thread_start, i, thread_mask] {
            bfs_thread_search(thread_start, i, thread_mask);
        });
    }

    for (std::thread& t : threads) {
        t.join();
    }
    print_solution_path();
}

bool Thread_maze::dfs_thread_search(Point start, size_t thread_index, Thread_tag thread_bit) {
    std::unordered_set<Point> seen;
    seen.insert(start);
    std::stack<Point> dfs;
    dfs.push(start);
    bool result = false;
    Point cur = start;
    while (!dfs.empty()) {
        if (escape_path_index_ != -1) {
            result = false;
            break;
        }

        // Don't pop() yet!
        cur = dfs.top();

        if (cur == finish_) {
            escape_section_.lock();
            if (escape_path_index_ == -1) {
                escape_path_index_ = thread_index;
            }
            escape_section_.unlock();
            result = true;
            dfs.pop();
            break;
        }

        Point chosen = {};
        // Bias each thread's first choice towards orginal dispatch direction. More coverage.
        int direction_index = thread_index;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            if (!seen.count(next) && (maze_[next.row][next.col] & path_bit_)) {
                chosen = next;
                break;
            }
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_index);

        // To emulate a true recursive dfs, we need to only push the current branch onto our stack.
        if (chosen.row) {
            dfs.push(chosen);
            seen.insert(chosen);
        } else {
            dfs.pop();
        }
    }
    // Another benefit of true depth first search is our stack always holds our exact path.
    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
        escape_section_.lock();
        maze_[cur.row][cur.col] |= thread_bit;
        // A thread or threads have already arrived at this location. Mark as an overlap.
        escape_section_.unlock();
    }
    return result;
}

bool Thread_maze::bfs_thread_search(Point start, size_t thread_index, Thread_tag thread_bit) {
    // This will be how we rebuild the path because queue does not represent the current path.
    std::unordered_map<Point,Point> seen;
    seen[start] = {-1,-1};
    std::queue<Point> bfs({start});
    bool result = false;
    Point cur = start;
    while (!bfs.empty()) {
        if (escape_path_index_ != -1) {
            result = false;
            break;
        }

        cur = bfs.front();
        bfs.pop();

        if (cur == finish_) {
            escape_section_.lock();
            if (escape_path_index_ == -1) {
                escape_path_index_ = thread_index;
            }
            escape_section_.unlock();
            result = true;
            break;
        }

        // Bias each thread towards the direction it was dispatched when we first sent it.
        int direction_index = thread_index;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            if (!seen.count(next) && (maze_[next.row][next.col] & path_bit_)) {
                seen[next] = cur;
                bfs.push(next);
                // This creates a nice spread of mixed color for each searching thread.
                escape_section_.lock();
                maze_[next.row][next.col] |= thread_bit;
                escape_section_.unlock();
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

void Thread_maze::generate_maze(Builder_algorithm algorithm, size_t odd_rows, size_t odd_cols) {
    if (odd_rows % 2 == 0) {
        odd_rows++;
    }
    if (odd_cols % 2 == 0) {
        odd_cols++;
    }
    if (odd_rows < 3 || odd_cols < 7) {
        std::cerr << "Smallest maze possible is 3x7" << std::endl;
        std::abort();
    }
    if (algorithm == Builder_algorithm::randomized_depth_first) {
        generate_randomized_dfs_maze(odd_rows, odd_cols);
    } else if (algorithm == Builder_algorithm::randomized_loop_erased) {
        generate_randomized_loop_erased_maze(odd_rows, odd_cols);
    } else {
        std::cerr << "Invalid builder arg, somehow?" << std::endl;
        std::abort();
    }
}

void Thread_maze::generate_randomized_loop_erased_maze(size_t odd_rows, size_t odd_cols) {
    (void) odd_cols;
    (void) odd_rows;
}

Thread_maze::Point
Thread_maze::choose_arbitrary_point(const std::unordered_set<Thread_maze::Point>& maze_paths) const {
    for (int row = 1; row < maze_.size() - 1; row++) {
        for (int col = 1; col < maze_[0].size() - 1; col++) {
            Point cur = {row, col};
            if (!maze_paths.count(cur)) {
                return cur;
            }
        }
    }
    return {0,0};
}

void Thread_maze::generate_randomized_dfs_maze(size_t odd_rows, size_t odd_cols) {
    maze_ = std::vector<std::vector<Square>>(odd_rows, std::vector<Square>(odd_cols));
    for (size_t row = 0; row < maze_.size(); row++) {
        for (size_t col = 0; col < maze_[0].size(); col++) {
            build_wall(row, col);
        }
    }
    std::uniform_int_distribution<int> row(1, maze_.size() - 2);
    std::uniform_int_distribution<int> col(1, maze_[0].size() - 2);
    start_ = {row(generator_), col(generator_)};
    std::unordered_set<Point> seen;
    std::stack<Point> dfs;
    dfs.push(start_);
    seen.insert(start_);
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
            if (!seen.count(next)
                    && next.row > 0 && static_cast<size_t>(next.row) < maze_.size() - 1
                        && next.col > 0 && static_cast<size_t>(next.col) < maze_[0].size() - 1) {
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
        seen.insert(random_neighbor);
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
        seen.insert(random_neighbor);
    }
    start_ = pick_random_point(row, col);
    finish_ = pick_random_point(row, col);
    maze_[start_.row][start_.col] |= start_bit_;
    maze_[finish_.row][finish_.col] |= finish_bit_;
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
    Point choice = {row(generator_), col(generator_)};
    if (maze_[choice.row][choice.col] & path_bit_) {
        return choice;
    }
    return find_nearest_square(choice);
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
    maze_[start_.row][start_.col] |= start_bit_;
    maze_[finish_.row][finish_.col] |= finish_bit_;
    std::cout << "\n";
    print_maze();
    if (escape_path_index_ == 0) {
        std::cout << thread_colors_.at(thread_masks_[0] >> thread_tag_offset_) << "█" << " 0_thread won!" << ansi_nil_ << "\n";
    } else if (escape_path_index_ == 1) {
        std::cout << thread_colors_.at(thread_masks_[1] >> thread_tag_offset_) << "█" << " 1_thread won!" << ansi_nil_ << "\n";
    } else if (escape_path_index_ == 2) {
        std::cout << thread_colors_.at(thread_masks_[2] >> thread_tag_offset_) << "█" << " 2_thread won!" << ansi_nil_ << "\n";
    } else if (escape_path_index_ == 3) {
        std::cout << thread_colors_.at(thread_masks_[3] >> thread_tag_offset_) << "█" << " 3_thread won!" << ansi_nil_ << "\n";
    } else {
        std::cout << "NO ESCAPE! >:)\n";
    }
    std::cout << "Maze generated with ";
    if (builder_ == Builder_algorithm::randomized_depth_first) {
        std::cout << "Randomized Depth First Search\n";
    } else if (builder_ == Builder_algorithm::randomized_loop_erased) {
        std::cout << "Loop-Erased Random Walk\n";
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
    for (size_t row = 0; row < maze_.size(); row++) {
        for (size_t col = 0; col < maze_[0].size(); col++) {
            const Square& square = maze_[row][col];
            if (square & start_bit_) {
                std::cout << ansi_cyn_ << "S" << ansi_nil_;
            } else if (square & finish_bit_) {
                std::cout << ansi_cyn_ << "F" << ansi_nil_;
            } else if (square & thread_mask_) {
                Thread_tag thread_color = (square & thread_mask_) >> thread_tag_offset_;
                std::cout << thread_colors_[thread_color] << "█" << ansi_nil_;
            } else if (!(square & path_bit_)) {
                std::cout << wall_lines_[square & wall_mask_];
            } else if (square & path_bit_) {
                std::cout << " ";
            }else {
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
               << "┠─────────────┬─────────────┬─────────────┬─────────────┬─────────────┤\n"
               << "│     0       │     1       │    1|0      │     2       │     2|0     │\n"
               << "┠─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
               << "│"<<thread_colors_[1]<<d<<n<<"│"<<thread_colors_[2]<<d<<n<<"│"<<thread_colors_[3]<<d<<n<<"│"<<thread_colors_[4]<<d<<n<<"│"<<thread_colors_[5]<<d<<n<<"│\n"
               << "┠─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
               << "│    2|1      │   2|1|0     │     3       │    3|0      │     3|1     │\n"
               << "┠─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
               << "│"<<thread_colors_[6]<<d<<n<<"│"<<thread_colors_[7]<<d<<n<<"│"<<thread_colors_[8]<<d<<n<<"│"<<thread_colors_[9]<<d<<n<<"│"<<thread_colors_[10]<<d<<n<<"│\n"
               << "┠─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
               << "│    3|1|0    │    3|2      │   3|2|0     │   3|2|1     │   3|2|1|0   │\n"
               << "┠─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤\n"
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
            square &= ~thread_mask_;
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
    generate_maze(builder_, maze_.size(), maze_[0].size());
}

void Thread_maze::new_maze(Thread_maze::Builder_algorithm builder,
                           size_t odd_rows,
                           size_t odd_cols) {
    generator_.seed(std::random_device{}());
    builder_ = builder;
    escape_path_index_ = -1;
    for (std::vector<Point>& vec : thread_paths_) {
        vec.clear();
        vec.reserve(starting_path_len_);
    }
    generate_maze(builder, odd_rows, odd_cols);
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

