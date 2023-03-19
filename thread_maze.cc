#include "thread_maze.hh"
#include <algorithm>
#include <stack>
#include <queue>
#include <iostream>
#include <thread>
#include <unordered_set>
#include <unordered_set>


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
        this->solve_with_dfs_threads();
    } else if (solver == Solver_algorithm::breadth_first_search) {
        this->solve_with_bfs_threads();
    } else {
        std::cerr << "Invalid solver?" << std::endl;
        abort();
    }
}

void Thread_maze::solve_maze() {
    this->solve_maze(solver_);
}

void Thread_maze::solve_with_dfs_threads() {
    if (this->escape_path_index_ != -1) {
        this->clear_paths();
    }

    std::vector<std::thread> threads(cardinal_directions_.size());
    for (int i = 0; i < num_threads_; i++) {
        const Point& direction = cardinal_directions_[i];
        Point thread_start = {start_.row + direction.row, start_.col + direction.col};
        threads[i] = std::thread([this, thread_start, i] {
            dfs_thread_search(start_, thread_start, static_cast<Square>(i));
        });

    }

    for (std::thread& t : threads) {
        t.join();
    }
    print_solution_path();
}

void Thread_maze::solve_with_bfs_threads() {
    if (this->escape_path_index_ != -1) {
        this->clear_paths();
    }

    std::vector<std::thread> threads(cardinal_directions_.size());
    for (int i = 0; i < num_threads_; i++) {
        const Point& direction = cardinal_directions_[i];
        Point thread_start = {start_.row + direction.row, start_.col + direction.col};
        threads[i] = std::thread([this, thread_start, i] {
            dfs_thread_search(start_, thread_start, static_cast<Square>(i));
        });
    }

    for (std::thread& t : threads) {
        t.join();
    }
    print_solution_path();
}

bool Thread_maze::dfs_thread_search(Point prev, Point start, Thread_maze::Square thread_index) {
    std::unordered_map<Point,Point> seen;
    seen[prev] = {-1,-1};
    seen[start] = prev;
    std::stack<Point> dfs;
    dfs.push(start);
    bool result = false;
    Point cur = start;
    int safe_thread_index_cast = static_cast<int>(thread_index);
    while (!dfs.empty()) {
        if (escape_path_index_ != -1) {
            result = false;
            break;
        }

        cur = dfs.top();
        dfs.pop();

        if (maze_[cur.row][cur.col] == Square::finish) {
            escape_section_.lock();
            if (escape_path_index_ == -1) {
                escape_path_index_ = safe_thread_index_cast;
            }
            escape_section_.unlock();
            result = true;
            break;
        }

        // DFS may result in threads at more disparate locations when one finally finds the finish.
        for (const Point& p : cardinal_directions_) {
            Point next = {cur.row + p.row, cur.col + p.col};
            if (!seen.count(next) && is_valid_point(next)) {
                seen[next] = cur;
                dfs.push(next);
            }
        }
    }
    rebuild_path(seen, cur, safe_thread_index_cast, thread_index);
    return result;
}

bool Thread_maze::bfs_thread_search(Point prev, Point start, Square thread_index) {
    std::unordered_map<Point,Point> seen;
    seen[prev] = {-1,-1};
    seen[start] = prev;
    std::queue<Point> dfs;
    dfs.push(start);
    bool result = false;
    Point cur = start;
    int safe_thread_index_cast = static_cast<int>(thread_index);
    while (!dfs.empty()) {
        if (escape_path_index_ != -1) {
            result = false;
            break;
        }

        cur = dfs.front();
        dfs.pop();

        if (maze_[cur.row][cur.col] == Square::finish) {
            escape_section_.lock();
            if (escape_path_index_ == -1) {
                escape_path_index_ = safe_thread_index_cast;
            }
            escape_section_.unlock();
            result = true;
            break;
        }

        // BFS trends each thread towards the direction it was dispatched when we first sent it.
        int i = safe_thread_index_cast;
        do {
            const Point& p = cardinal_directions_[i];
            Point next = {cur.row + p.row, cur.col + p.col};
            if (!seen.count(next) && is_valid_point(next)) {
                seen[next] = cur;
                dfs.push(next);
            }
            ++i %= cardinal_directions_.size();
        } while (i != safe_thread_index_cast);
    }
    rebuild_path(seen, cur, safe_thread_index_cast, thread_index);
    return result;
}

void Thread_maze::rebuild_path(const std::unordered_map<Point,Point>& parent_map,
                               Point cur,
                               int safe_thread_index_cast,
                               Square thread_index) {
    cur = parent_map.at(cur);
    while(cur.row > 0) {
        thread_paths_[safe_thread_index_cast].push_back(cur);
        escape_section_.lock();
        // A thread or threads have already arrived at this location. Mark as an overlap.
        if (maze_[cur.row][cur.col] <= Square::overlap_threads) {
            maze_[cur.row][cur.col] = Square::overlap_threads;
        // No other thread has arrived so we uniquely mark this square.
        } else {
            maze_[cur.row][cur.col] = thread_index;
        }
        escape_section_.unlock();
        cur = parent_map.at(cur);
    }
}

bool Thread_maze::is_valid_point(Point to_check) {
    return to_check.row >= 0 && to_check.row < maze_.size() &&
           to_check.col >= 0 && to_check.col < maze_[0].size() &&
           maze_[to_check.row][to_check.col] != Square::wall;
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
    } else {
        std::cerr << "Invalid builder arg, somehow?" << std::endl;
        std::abort();
    }
}

void Thread_maze::generate_randomized_dfs_maze(size_t odd_rows, size_t odd_cols) {
    maze_ = std::vector<std::vector<Square>>(odd_rows, std::vector<Square>(odd_cols));
    for (int row = 0; row < maze_.size(); row++) {
        for (int col = 0; col < maze_[0].size(); col++) {
            maze_[row][col] = Square::wall;
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
        Point cur = dfs.top();
        dfs.pop();
        maze_[cur.row][cur.col] = Square::square;
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        for (const int& i : random_direction_indices) {
            // Choose another square that is two spaces a way.
            const Point& direction = generate_directions_[i];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            if (!seen.count(next)
                    && next.row > 0 && next.row < maze_.size() - 1
                        && next.col > 0 && next.col < maze_[0].size() - 1) {
                seen.insert(next);
                dfs.push(next);
                // Now we must break the wall between this two square jump.
                if (next.row < cur.row) {
                    next.row++;
                } else if (next.row > cur.row) {
                    next.row--;
                } else if (next.col < cur.col) {
                    next.col++;
                } else if (next.col > cur.col) {
                    next.col--;
                } else {
                    std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
                }
                maze_[next.row][next.col] = Square::square;
            }
        }
    }
    start_ = pick_random_point(row, col);
    finish_ = pick_random_point(row, col);
    if (start_ == finish_) {
        finish_ = adjust_point(finish_);
    }
    maze_[start_.row][start_.col] = Square::start;
    maze_[finish_.row][finish_.col] = Square::finish;
}

Thread_maze::Point Thread_maze::pick_random_point(std::uniform_int_distribution<int>& row,
                                                  std::uniform_int_distribution<int>& col) {
    Point choice = {row(generator_), col(generator_)};
    if (maze_[choice.row][choice.col] == Square::square) {
        return choice;
    }
    return adjust_point(choice);
}

Thread_maze::Point Thread_maze::adjust_point(Thread_maze::Point choice) {
    // s    se     e     sw     w      nw       w
    for (const Point& p : all_directions_) {
         Point next = {choice.row + p.row, choice.col + p.col};
         if (next.row > 0 && next.row < maze_.size() - 1
                 && next.col > 0 && next.col < maze_[0].size() - 1
                    && maze_[next.row][next.col] == Square::square) {
             choice = next;
             break;
         }
    }
    return choice;
}

void Thread_maze::print_solution_path() {
    maze_[start_.row][start_.col] = Square::start;
    std::cout << "\n";
    print_maze();
    if (escape_path_index_ == 0) {
        std::cout << thread_colors_[0] << thread_chars_[0] << " 0_thread won!" << ansi_nil << "\n";
    } else if (escape_path_index_ == 1) {
        std::cout << thread_colors_[1] <<thread_chars_[1] << " 1_thread won!" << ansi_nil << "\n";
    } else if (escape_path_index_ == 2) {
        std::cout << thread_colors_[2] << thread_chars_[2] << " 2_thread won!" << ansi_nil << "\n";
    } else if (escape_path_index_ == 3) {
        std::cout << thread_colors_[3] << thread_chars_[3] << " 3_thread won!" << ansi_nil << "\n";
    } else {
        std::cout << "NO ESCAPE! >:)\n";
    }
    std::cout << std::endl;
}

void Thread_maze::print_maze() const {
    std::cout << thread_colors_[0] << thread_chars_[0] << " 0_THREAD, " << ansi_nil
              << thread_colors_[1] << thread_chars_[1] << " 1_THREAD, " << ansi_nil
              << thread_colors_[2] << thread_chars_[2] << " 2_THREAD, " << ansi_nil
              << thread_colors_[3] << thread_chars_[3] << " 3_THREAD, " << ansi_nil
              << thread_colors_[4] << thread_chars_[4] << " OVERLAP_THREADS" << ansi_nil << "\n";
    for (const std::vector<Square>& row : maze_) {
        for (const Square& square : row) {
            if (square <= Square::overlap_threads) {
                int index = static_cast<int>(square);
                std::cout << thread_colors_[index] << thread_chars_[index] << ansi_nil;
            } else if (square == Square::start) {
                std::cout << ansi_cyn << "S" << ansi_nil;
            } else if (square == Square::wall) {
                std::cout << "|";
            } else if (square == Square::square) {
                std::cout << " ";
            } else if (square == Square::finish) {
                std::cout << ansi_cyn << "F" << ansi_nil;
            }else {
                std::cerr << "Dumped maze and a square was not categorized." << std::endl;
                abort();
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

void Thread_maze::clear_paths() {
    escape_path_index_ = -1;
    for (std::vector<Point>& vec : thread_paths_) {
        vec.clear();
        vec.reserve(starting_path_len_);
    }
    for (std::vector<Square>& row : maze_) {
        for (Square& square : row) {
            if (square <= Square::overlap_threads) {
                square = Square::square;
            }
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

