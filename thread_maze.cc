#include "thread_maze.hh"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <stack>
#include <queue>
#include <iostream>
#include <thread>
#include <vector>

const std::unordered_map<Thread_maze::Wall_line,std::string> Thread_maze::wall_lines_ = {
    {north_wall_, "┃"},
    {south_wall_, "┃"},
    {north_south_wall_, "┃"},
    {east_wall_, "━"},
    {west_wall_, "━"},
    {east_west_wall_, "━"},
    {north_east_south_west_wall_, "╋"},
    {north_east_south_wall_, "┣"},
    {north_west_south_wall_, "┫"},
    {north_west_east_wall_, "┻"},
    {south_west_east_wall_, "┳"},
    {north_east_wall_, "┗"},
    {north_west_wall_, "┛"},
    {south_east_wall_, "┏"},
    {south_west_wall_, "┓"},
};

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
        if (maze_[thread_start.row][thread_start.col] == Square::wall) {
            thread_start = start_;
        }
        threads[i] = std::thread([this, thread_start, i] {
            dfs_thread_search(thread_start, static_cast<Square>(i));
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
        if (maze_[thread_start.row][thread_start.col] == Square::wall) {
            thread_start = start_;
        }
        threads[i] = std::thread([this, thread_start, i] {
            dfs_thread_search(thread_start, static_cast<Square>(i));
        });
    }

    for (std::thread& t : threads) {
        t.join();
    }
    print_solution_path();
}

bool Thread_maze::dfs_thread_search(Point start, Thread_maze::Square thread) {
    std::unordered_set<Point> seen;
    seen.insert(start);
    std::stack<Point> dfs;
    dfs.push(start);
    bool result = false;
    Point cur = start;
    int thread_direction_index = static_cast<int>(thread);
    while (!dfs.empty()) {
        if (escape_path_index_ != -1) {
            result = false;
            break;
        }

        // Don't pop() yet!
        cur = dfs.top();

        if (maze_[cur.row][cur.col] == Square::finish) {
            escape_section_.lock();
            if (escape_path_index_ == -1) {
                escape_path_index_ = thread_direction_index;
            }
            escape_section_.unlock();
            result = true;
            dfs.pop();
            break;
        }

        Point chosen = {};
        // Bias each thread's first choice towards orginal dispatch direction. More coverage.
        int direction_index = thread_direction_index;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            if (!seen.count(next) && maze_[next.row][next.col] != Square::wall) {
                chosen = next;
                break;
            }
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_direction_index);

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
        thread_paths_[thread_direction_index].push_back(cur);
        escape_section_.lock();
        Square& square = maze_[cur.row][cur.col];
        // A thread or threads have already arrived at this location. Mark as an overlap.
        square <= Square::overlap_threads ? square = Square::overlap_threads : square = thread;
        escape_section_.unlock();
    }
    return result;
}

bool Thread_maze::bfs_thread_search(Point start, Square thread) {
    // This will be how we rebuild the path because queue does not represent the current path.
    std::unordered_map<Point,Point> seen;
    seen[start] = {-1,-1};
    std::queue<Point> dfs;
    dfs.push(start);
    bool result = false;
    Point cur = start;
    int thread_direction_index = static_cast<int>(thread);
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
                escape_path_index_ = thread_direction_index;
            }
            escape_section_.unlock();
            result = true;
            break;
        }

        // Bias each thread towards the direction it was dispatched when we first sent it.
        int direction_index = thread_direction_index;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            if (!seen.count(next) && maze_[next.row][next.col] != Square::wall) {
                seen[next] = cur;
                dfs.push(next);
            }
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_direction_index);
    }
    cur = seen.at(cur);
    while(cur.row > 0) {
        thread_paths_[thread_direction_index].push_back(cur);
        escape_section_.lock();
        Square& square = maze_[cur.row][cur.col];
        square <= Square::overlap_threads ? square = Square::overlap_threads : square = thread;
        escape_section_.unlock();
        cur = seen.at(cur);
    }
    return result;
}

bool Thread_maze::is_valid_point(Point to_check) {
    return to_check.row >= 0 && static_cast<size_t>(to_check.row) < maze_.size() - 1
        && to_check.col >= 0 && static_cast<size_t>(to_check.col) < maze_[0].size() - 1
            && maze_[to_check.row][to_check.col] != Square::wall;
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
    maze_ = std::vector<std::vector<Square>>(odd_rows, std::vector<Square>(odd_cols));
    for (size_t row = 0; row < maze_.size(); row++) {
        for (size_t col = 0; col < maze_[0].size(); col++) {
            maze_[row][col] = Square::wall;
        }
    }
    std::uniform_int_distribution<int> row(1, maze_.size() - 2);
    std::uniform_int_distribution<int> col(1, maze_[0].size() - 2);
    start_ = {row(generator_), col(generator_)};
    Point path_start = {row(generator_), col(generator_)};
    if (start_ == path_start) {
        path_start = find_nearest_wall(path_start);
    }
    std::unordered_set<Point> maze_paths = {start_};
    std::unordered_map<Point,Point> seen_path = {{path_start, path_start}};
    std::vector<Point> random_walk({path_start});
    std::vector<int> random_direction_indices(generate_directions_.size());
    iota(begin(random_direction_indices), end(random_direction_indices), 0);

    Point prev_direction = path_start;
    while (!random_walk.empty()) {
        Point cur = random_walk.back();

        if (maze_paths.count(cur)) {
            while (!random_walk.empty()) {
                cur = random_walk.back();
                maze_[cur.row][cur.col] = Square::square;
                maze_paths.insert(cur);
                random_walk.pop_back();
            }
            path_start = choose_arbitrary_point(maze_paths);
            if (!path_start.row) {
                return;
            }
            random_walk.push_back(path_start);
        }


        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        Point random_neighbor = {};
        for (const int& i : random_direction_indices) {
            const Point& direction = generate_directions_[i];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            if (next.row > 0 && static_cast<size_t>(next.row) < maze_.size() - 1
                        && next.col > 0 && static_cast<size_t>(next.col) < maze_[0].size() - 1
                            && next != prev_direction) {
                random_neighbor = next;
                break;
            }
        }

        if (!random_neighbor.row) {
            random_walk.pop_back();
            continue;
        }

        if (seen_path.count(random_neighbor)) {
            while (cur != random_neighbor) {
                Point parent = seen_path.at(cur);
                random_walk.pop_back();
                seen_path.erase(cur);
                cur = parent;
            }
            continue;
        }


        Point wall = random_neighbor;
        if (random_neighbor.row < cur.row) {
            wall.row++;
        } else if (random_neighbor.row > cur.row) {
            wall.row--;
        } else if (random_neighbor.col < cur.col) {
            wall.col++;
        } else if (random_neighbor.col > cur.col) {
            wall.col--;
        } else {
            std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
        }
        seen_path[wall] = cur;
        seen_path[random_neighbor] = wall;
        random_walk.push_back(wall);
        random_walk.push_back(random_neighbor);
        prev_direction = cur;
    }
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
        // Don't pop yet!
        Point cur = dfs.top();
        maze_[cur.row][cur.col] = Square::square;
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
        maze_[random_neighbor.row][random_neighbor.col] = Square::square;
        // Walls are just a simple value that a square can have so I need to mark as seen.
        seen.insert(random_neighbor);
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
    return find_nearest_square(choice);
}

Thread_maze::Point Thread_maze::find_nearest_square(Thread_maze::Point choice) {
    for (const Point& p : all_directions_) {
        Point next = {choice.row + p.row, choice.col + p.col};
        if (next.row > 0 && static_cast<size_t>(next.row) < maze_.size() - 1
                && next.col > 0 && static_cast<size_t>(next.col) < maze_[0].size() - 1
                            && maze_[next.row][next.col] == Square::square) {
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
                    && maze_[next.row][next.col] == Square::wall) {
             return next;
         }
    }
    return {0,0};
}

void Thread_maze::print_solution_path() {
    maze_[start_.row][start_.col] = Square::start;
    maze_[finish_.row][finish_.col] = Square::finish;
    std::cout << "\n";
    print_maze();
    if (escape_path_index_ == 0) {
        std::cout << thread_colors_[0] << thread_chars_[0] << " 0_thread won!" << ansi_nil_ << "\n";
    } else if (escape_path_index_ == 1) {
        std::cout << thread_colors_[1] <<thread_chars_[1] << " 1_thread won!" << ansi_nil_ << "\n";
    } else if (escape_path_index_ == 2) {
        std::cout << thread_colors_[2] << thread_chars_[2] << " 2_thread won!" << ansi_nil_ << "\n";
    } else if (escape_path_index_ == 3) {
        std::cout << thread_colors_[3] << thread_chars_[3] << " 3_thread won!" << ansi_nil_ << "\n";
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
    std::cout << thread_colors_[0] << thread_chars_[0] << " 0_THREAD, " << ansi_nil_
              << thread_colors_[1] << thread_chars_[1] << " 1_THREAD, " << ansi_nil_
              << thread_colors_[2] << thread_chars_[2] << " 2_THREAD, " << ansi_nil_
              << thread_colors_[3] << thread_chars_[3] << " 3_THREAD, " << ansi_nil_
              << thread_colors_[4] << thread_chars_[4] << " OVERLAP_THREADS" << ansi_nil_ << "\n";
    for (size_t row = 0; row < maze_.size(); row++) {
        for (size_t col = 0; col < maze_[0].size(); col++) {
            const Square& square = maze_[row][col];
            if (square <= Square::overlap_threads) {
                int index = static_cast<int>(square);
                std::cout << thread_colors_[index] << thread_chars_[index] << ansi_nil_;
            } else if (square == Square::start) {
                std::cout << ansi_cyn_ << "S" << ansi_nil_;
            } else if (square == Square::wall) {
                print_wall(row, col);
            } else if (square == Square::square) {
                std::cout << " ";
            } else if (square == Square::finish) {
                std::cout << ansi_cyn_ << "F" << ansi_nil_;
            }else {
                std::cerr << "Printed maze and a square was not categorized." << std::endl;
                abort();
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

void Thread_maze::print_wall(int row, int col) const {
    Wall_line wall = 0;
    // By hardcoding every possible wall configuration beforehand, we now simply build wall.
    if (row - 1 >= 0 && maze_[row - 1][col] == Square::wall) {
        wall |= north_wall_;
    }
    if (row + 1 < maze_.size() && maze_[row + 1][col] == Square::wall) {
        wall |= south_wall_;
    }
    if (col - 1 >= 0 && maze_[row][col - 1] == Square::wall) {
        wall |= west_wall_;
    }
    if (col + 1 < maze_[0].size() && maze_[row][col + 1] == Square::wall) {
        wall |= east_wall_;
    }
    auto found_piece = wall_lines_.find(wall);
    if (found_piece == wall_lines_.end()) {
        std::cerr << "Error building the wall. Review construction of wall pieces." << std::endl;
        std::abort();
    }
    std::cout << found_piece->second;
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

