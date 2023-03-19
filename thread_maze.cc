#include "thread_maze.hh"
#include <algorithm>
#include <functional>
#include <unordered_set>

static constexpr const char *const color_red = "\033[31;1m";
static constexpr const char *const color_grn = "\033[32;1m";
static constexpr const char *const color_yel = "\033[93;1m";
static constexpr const char *const color_blu = "\033[34;1m";
static constexpr const char *const color_cyn = "\033[36;1m";
static constexpr const char *const color_wit = "\033[255;1m";
static constexpr const char *const color_nil = "\033[0m";

const int Thread_maze::num_threads_ = 4;
const int Thread_maze::starting_path_len_ = 4096;
const std::vector<std::string_view> Thread_maze::thread_colors_ = {
    color_red, color_grn, color_blu, color_yel, color_wit
};
const std::vector<char> Thread_maze::thread_chars_ = {'x', '^', '+', '*','#'};
//                                                                           n      e     s      w
const std::vector<Thread_maze::Point> Thread_maze::cardinal_directions_ = {{-1,0},{0,1},{1,0},{0,-1}};


Thread_maze::Thread_maze(size_t odd_rows, size_t odd_cols)
    : generator_(std::random_device{}()),
      thread_paths_(num_threads_),
      escape_path_index_(-1){
      generate_randomized_dfs_maze(odd_rows, odd_cols);
    // If threads need to rely on heap for thread safe resizing, we slow parallelism.
    for (std::vector<Point>& vec : thread_paths_) {
        vec.reserve(starting_path_len_);
    }
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
    if (escape_path_index_ != -1) {
        print_solution_path();
    } else {
        std::cout << "NO ESCAPE! >:)" << std::endl;
    }
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
    if (escape_path_index_ != -1) {
        print_solution_path();
    } else {
        std::cout << "NO ESCAPE! >:)" << std::endl;
    }
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

bool Thread_maze::bfs_thread_search(Thread_maze& maze, Point prev, Point start, Square thread_index) {
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

        if (maze[cur.row][cur.col] == Square::finish) {
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

void Thread_maze::generate_randomized_dfs_maze(size_t odd_rows, size_t odd_cols) {
    if (odd_rows % 2 == 0 || odd_cols % 2 == 0) {
        std::cerr << "Mazes must have wall perimeters and therefore be odd." << std::endl;
        std::abort();
    }
    if (odd_rows < 3 || odd_cols < 3) {
        std::cerr << "Smallest maze possible is 3x3" << std::endl;
        std::abort();
    }
    maze_ = std::vector<std::vector<Square>>(odd_rows, std::vector<Square>(odd_cols));
    for (int row = 0; row < maze_.size(); row++) {
        for (int col = 0; col < maze_[0].size(); col++) {
            maze_[row][col] = Square::wall;
        }
    }
    dfs_stack_generator();
}

void Thread_maze::dfs_stack_generator() {
    std::uniform_int_distribution<int> row(1, maze_.size() - 2);
    std::uniform_int_distribution<int> col(1, maze_[0].size() - 2);
    start_ = {row(generator_), col(generator_)};
    std::unordered_set<Point> seen;
    std::stack<Point> dfs;
    dfs.push(start_);
    seen.insert(start_);
    const std::vector<Point> builder_directions = {{-2,0},{0,-2},{0,2},{2,0}};
    std::vector<int> direction_indices(builder_directions.size());
    iota(begin(direction_indices), end(direction_indices), 0);
    while (!dfs.empty()) {
        Point cur = dfs.top();
        dfs.pop();
        maze_[cur.row][cur.col] = Square::square;

        shuffle(begin(direction_indices), end(direction_indices), generator_);
        for (const int& i : direction_indices) {
            const Point& direction = builder_directions[i];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            if (!seen.count(next)
                    && next.row > 0 && next.row < maze_.size() - 1
                        && next.col > 0 && next.col < maze_[0].size() - 1) {
                seen.insert(next);
                dfs.push(next);
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
    maze_[start_.row][start_.col] = Square::start;
    maze_[finish_.row][finish_.col] = Square::finish;

}

Thread_maze::Point Thread_maze::pick_random_point(std::uniform_int_distribution<int>& row,
                                     std::uniform_int_distribution<int>& col) {
    Point choice = {row(generator_), col(generator_)};
    if (maze_[choice.row][choice.col] == Square::square) {
        return choice;
    }
    //                                          s    se     e     sw     w      nw       w
    const std::vector<Point> allDirections = {{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1}};
    for (const Point& p : allDirections) {
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
        std::cout << thread_colors_[0] << thread_chars_[0] << " 0_thread won!" << color_nil << "\n";
    } else if (escape_path_index_ == 1) {
        std::cout << thread_colors_[1] <<thread_chars_[1] << " 1_thread won!" << color_nil << "\n";
    } else if (escape_path_index_ == 2) {
        std::cout << thread_colors_[2] << thread_chars_[2] << " 2_thread won!" << color_nil << "\n";
    } else if (escape_path_index_ == 3) {
        std::cout << thread_colors_[3] << thread_chars_[3] << " 3_thread won!" << color_nil << "\n";
    }
    std::cout << std::endl;
}

void Thread_maze::print_maze() const {
    std::cout << thread_colors_[0] << thread_chars_[0] << " 0_THREAD, " << color_nil
              << thread_colors_[1] << thread_chars_[1] << " 1_THREAD, " << color_nil
              << thread_colors_[2] << thread_chars_[2] << " 2_THREAD, " << color_nil
              << thread_colors_[3] << thread_chars_[3] << " 3_THREAD, " << color_nil
              << thread_colors_[4] << thread_chars_[4] << " OVERLAP_THREADS" << color_nil << "\n";
    for (const std::vector<Square>& row : maze_) {
        for (const Square& square : row) {
            if (square <= Square::overlap_threads) {
                int index = static_cast<int>(square);
                std::cout << thread_colors_[index] << thread_chars_[index] << color_nil;
            } else if (square == Square::start) {
                std::cout << color_cyn << "S" << color_nil;
            } else if (square == Square::wall) {
                std::cout << "|";
            } else if (square == Square::square) {
                std::cout << " ";
            } else if (square == Square::finish) {
                std::cout << color_cyn << "F" << color_nil;
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
    generate_randomized_dfs_maze(maze_.size(), maze_[0].size());
}

void Thread_maze::new_maze(size_t odd_rows, size_t odd_cols) {
    generator_.seed(std::random_device{}());
    escape_path_index_ = -1;
    for (std::vector<Point>& vec : thread_paths_) {
        vec.clear();
        vec.reserve(starting_path_len_);
    }
    generate_randomized_dfs_maze(odd_rows, odd_cols);
}

bool operator==(const Thread_maze::Point& lhs, const Thread_maze::Point& rhs) {
    return lhs.row == rhs.row && lhs.col == rhs.col;
}
