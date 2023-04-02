#include "thread_maze.hh"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <ios>
#include <numeric>
#include <ostream>
#include <random>
#include <string>
#include <tuple>
#include <stack>
#include <queue>
#include <iostream>
#include <thread>
#include <vector>

namespace {

using Height = int;
using Width = int;

struct Disjoint_set {
    std::vector<int> parent_set_;
    std::vector<int> set_rank_;
    Disjoint_set(const std::vector<int>& maze_square_ids)
        : parent_set_(maze_square_ids.size()),
          set_rank_(maze_square_ids.size()){
        for (size_t elem = 0; elem < maze_square_ids.size(); elem++) {
            parent_set_[elem] = elem;
            set_rank_[elem] = 0;
        }
    }

    int find(int p) {
        std::vector<int> compress_path;
        while (parent_set_[p] != p) {
            compress_path.push_back(p);
            p = parent_set_[p];
        }
        while (!compress_path.empty()) {
            parent_set_[compress_path.back()] = p;
            compress_path.pop_back();
        }
        return p;
    }

    bool is_union(int a, int b) {
        int x = find(a);
        int y = find(b);
        if (x == y) {
            return true;
        }
        if (set_rank_[x] > set_rank_[y]) {
            parent_set_[y] = x;
        } else if (set_rank_[x] < set_rank_[y]) {
            parent_set_[x] = y;
        } else {
            parent_set_[x] = y;
            set_rank_[y]++;
        }
        return false;
    }

};


} // namespace


Thread_maze::Thread_maze(const Thread_maze::Maze_args& args)
    : builder_(args.builder),
      modification_(args.modification),
      style_(args.style),
      builder_speed_(builder_speeds_[static_cast<size_t>(args.builder_speed)]),
      maze_(args.odd_rows, std::vector<Square>(args.odd_cols, 0)),
      maze_row_size_(maze_.size()),
      maze_col_size_(maze_[0].size()),
      generator_(std::random_device{}()),
      row_random_(1, maze_row_size_ - 2),
      col_random_(1, maze_col_size_ - 2) {
    if (builder_ != Builder_algorithm::randomized_fractal
            && modification_ != Maze_modification::none
                && builder_speed_) {
        clear_screen();
        for (int row = 0; row < maze_row_size_; row++) {
            for (int col = 0; col < maze_col_size_; col++) {
                build_wall(row, col);
                if (builder_ != Builder_algorithm::randomized_fractal
                        && modification_ != Maze_modification::none) {
                    add_modification_animated(row, col);
                }
            }
        }

    } else {
        for (int row = 0; row < maze_row_size_; row++) {
            for (int col = 0; col < maze_col_size_; col++) {
                build_wall(row, col);
                if (builder_ != Builder_algorithm::randomized_fractal
                        && modification_ != Maze_modification::none) {
                    add_modification(row, col);
                }
            }
        }
    }
    generate_maze(args.builder);
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

void Thread_maze::add_modification_animated(int row, int col) {
    if (modification_ == Maze_modification::add_cross) {
        if ((row == maze_row_size_ / 2 && col > 1 && col < maze_col_size_ - 2)
                || (col == maze_col_size_ / 2 && row > 1 && row < maze_row_size_ - 2)) {
            build_path_animated(row, col);
            if (col + 1 < maze_col_size_ - 2) {
                build_path_animated(row, col + 1);
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
            build_path_animated(row, col);
            if (col + 1 < maze_col_size_ - 2) {
                build_path_animated(row, col + 1);
            }
            if (col - 1 > 1) {
                build_path_animated(row, col - 1);
            }
            if (col + 2 < maze_col_size_ - 2) {
                build_path_animated(row, col + 2);
            }
            if (col - 2 > 1) {
                build_path_animated(row, col - 2);
            }
        }
        // This line is drawn from top right to bottom left.
        slope = (2.0 - row_size) / (col_size - 2.0);
        b = row_size - (2.0 * slope);
        on_line = (cur_row - b) / slope;
        if (col == on_line && col > 1 && col < maze_col_size_ - 2 && row < maze_row_size_ - 2) {
            build_path_animated(row, col);
            if (col + 1 < maze_col_size_ - 2) {
                build_path_animated(row, col + 1);
            }
            if (col - 1 > 1) {
                build_path_animated(row, col - 1);
            }
            if (col + 2 < maze_col_size_ - 2) {
                build_path_animated(row, col + 2);
            }
            if (col - 2 > 1) {
                build_path_animated(row, col - 2);
            }
        }
    }
}


/* * * * * * * * * * * * * * * *    Maze Generation Caller    * * * * * * * * * * * * * * * * * * */


void Thread_maze::generate_maze(Builder_algorithm algorithm) {
    if (algorithm == Builder_algorithm::randomized_depth_first) {
        if (builder_speed_) {
            generate_randomized_dfs_maze_animated();
        } else {
            generate_randomized_dfs_maze();
        }
    } else if (algorithm == Builder_algorithm::randomized_loop_erased) {
        if (builder_speed_) {
            generate_randomized_loop_erased_maze_animated();
        } else {
            generate_randomized_loop_erased_maze();
        }
    } else if (algorithm == Builder_algorithm::arena) {
        if (builder_speed_) {
            generate_arena_animated();
        } else {
            generate_arena();
        }
    } else if (algorithm == Builder_algorithm::randomized_grid) {
        if (builder_speed_) {
            generate_randomized_grid_animated();
        } else {
            generate_randomized_grid();
        }
    } else if (algorithm == Builder_algorithm::randomized_fractal) {
        if (builder_speed_) {
            generate_randomized_fractal_maze_animated();
        } else {
            generate_randomized_fractal_maze();
        }
    } else if (algorithm == Builder_algorithm::randomized_kruskal) {
        if (builder_speed_) {
            generate_randomized_kruskal_maze_animated();
        } else {
            generate_randomized_kruskal_maze();
        }
    } else {
        std::cerr << "Builder algorithm not set? Check for new builder algorithm." << std::endl;
        std::abort();
    }
}


/* * * * * * * * * * * * * * *      Wilson's  Algorithm      * * * * * * * * * * * * * * * * * * */


void Thread_maze::generate_randomized_loop_erased_maze() {

    auto build_with_marks = [&] (const Point& cur, const Point& next) {
        Point wall = cur;
        carve_path_walls(cur.row, cur.col);
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
        carve_path_walls(next.row, next.col);
        carve_path_walls(wall.row, wall.col);
    };

    auto connect_walk_to_maze = [&] (const Point& walk) {
        Point cur = walk;
        while (maze_[cur.row][cur.col] & markers_mask_) {
            // It is now desirable to run into this path as we complete future random walks.
            maze_[cur.row][cur.col] &= ~start_bit_;
            Backtrack_marker mark = (maze_[cur.row][cur.col] & markers_mask_) >> marker_shift_;
            const Point& direction = backtracking_marks_[mark];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            build_with_marks(cur, next);
            // Clean up after ourselves and leave no marks behind for the maze solvers.
            maze_[cur.row][cur.col] &= ~markers_mask_;
            cur = next;
        }
        maze_[cur.row][cur.col] &= ~start_bit_;
        maze_[cur.row][cur.col] &= ~markers_mask_;
        carve_path_walls(cur.row, cur.col);
    };

    auto erase_loop = [&] (const Point& walk, const Point& loop_root) {
        // We will forget we ever saw this loop as it may be part of a good walk later.
        Point cur = walk;
        while (cur != loop_root) {
            maze_[cur.row][cur.col] &= ~start_bit_;
            Backtrack_marker mark = (maze_[cur.row][cur.col] & markers_mask_) >> marker_shift_;
            const Point& direction = backtracking_marks_[mark];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            maze_[cur.row][cur.col] &= ~markers_mask_;
            cur = next;
        }
    };

    auto mark_origin = [&] (const Point& walk, const Point& next) {
        if (next.row > walk.row) {
            maze_[next.row][next.col] |= from_north_;
        } else if (next.row < walk.row) {
            maze_[next.row][next.col] |= from_south_;
        } else if (next.col < walk.col) {
            maze_[next.row][next.col] |= from_east_;
        } else if (next.col > walk.col) {
            maze_[next.row][next.col] |= from_west_;
        }
    };

    /* Important to remember that this maze builds by jumping two squares at a time. Therefore for
     * Wilson's algorithm to work two points must both be even or odd to find each other.
     */
    Point start = {maze_row_size_ / 2, maze_col_size_ / 2};
    if (start.row % 2 == 0) {
        start.row++;
    }
    if (start.col % 2 == 0) {
        start.col++;
    }
    build_path(start.row, start.col);
    maze_[start.row][start.col] |= builder_bit_;
    Point walk = {1, 1};
    maze_[walk.row][walk.col] &= ~markers_mask_;
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);

    Point previous_square = {};
    for (;;) {
        // Mark our progress on our current random walk. If we see this again we have looped.
        maze_[walk.row][walk.col] |= start_bit_;
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        for (const int& i : random_direction_indices) {
            const Point& p = generate_directions_[i];
            Point next = {walk.row + p.row, walk.col + p.col};
            // Do not check for seen squares, only valid moves and the direction we just came from.
            if (next.row <= 0 || next.row >= maze_row_size_ - 1
                        || next.col <= 0 || next.col >= maze_col_size_ - 1
                            || next == previous_square) {
                continue;
            }

            if (maze_[next.row][next.col] & builder_bit_) {
                build_with_marks(walk, next);
                connect_walk_to_maze(walk);
                walk = choose_arbitrary_point(Wilson_point::odd);

                // This is the only way out of the wilson algorithm! Everything is connected.
                if (!walk.row) {
                    return;
                }

                maze_[walk.row][walk.col] &= ~markers_mask_;
                previous_square = {};
            } else if (maze_[next.row][next.col] & start_bit_) {
                erase_loop(walk, next);
                walk = next;
                previous_square = {};
                Backtrack_marker mark = (maze_[walk.row][walk.col] & markers_mask_) >> marker_shift_;
                const Point& direction = backtracking_marks_[mark];
                previous_square = {walk.row + direction.row, walk.col + direction.col};
            } else {
                mark_origin(walk, next);
                previous_square = walk;
                walk = next;
            }
            // Our walk needs to be depth first, so break after selection.
            break;
        }
    }
}

void Thread_maze::generate_randomized_loop_erased_maze_animated() {
    auto build_with_marks = [&] (const Point& cur, const Point& next) {
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
        // Otherwise the cursor will print current and next squares excess times.
        maze_[wall.row][wall.col] |= path_bit_;
        maze_[next.row][next.col] |= path_bit_;
        maze_[next.row][next.col] &= ~start_bit_;
        carve_path_walls_animated(cur.row, cur.col);
        carve_path_walls_animated(wall.row, wall.col);
        carve_path_walls_animated(next.row, next.col);
    };

    auto connect_walk_to_maze = [&] (const Point& walk) {
        Point cur = walk;
        while (maze_[cur.row][cur.col] & markers_mask_) {
            // It is now desirable to run into this path as we complete future random walks.
            maze_[cur.row][cur.col] &= ~start_bit_;
            Backtrack_marker mark = (maze_[cur.row][cur.col] & markers_mask_) >> marker_shift_;
            const Point& direction = backtracking_marks_[mark];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            build_with_marks(cur, next);
            // Clean up after ourselves and leave no marks behind for the maze solvers.
            maze_[cur.row][cur.col] &= ~markers_mask_;
            flush_cursor_maze_coordinate(cur.row, cur.col);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
            cur = next;
        }
        maze_[cur.row][cur.col] &= ~start_bit_;
        maze_[cur.row][cur.col] &= ~markers_mask_;
        carve_path_walls_animated(cur.row, cur.col);
        flush_cursor_maze_coordinate(cur.row, cur.col);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    };

    auto erase_loop = [&] (const Point& walk, const Point& loop_root) {
        // We will forget we ever saw this loop as it may be part of a good walk later.
        Point cur = walk;
        while (cur != loop_root) {
            maze_[cur.row][cur.col] &= ~start_bit_;
            Backtrack_marker mark = (maze_[cur.row][cur.col] & markers_mask_) >> marker_shift_;
            const Point& direction = backtracking_marks_[mark];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            maze_[cur.row][cur.col] &= ~markers_mask_;
            flush_cursor_maze_coordinate(cur.row, cur.col);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
            cur = next;
        }
    };

    auto mark_origin = [&] (const Point& walk, const Point& next) {
        if (next.row > walk.row) {
            maze_[next.row][next.col] |= from_north_;
        } else if (next.row < walk.row) {
            maze_[next.row][next.col] |= from_south_;
        } else if (next.col < walk.col) {
            maze_[next.row][next.col] |= from_east_;
        } else if (next.col > walk.col) {
            maze_[next.row][next.col] |= from_west_;
        }
        flush_cursor_maze_coordinate(next.row, next.col);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    };

    clear_and_flush_grid();
    Point start = {maze_row_size_ / 2, maze_col_size_ / 2};
    if (start.row % 2 == 0) {
        start.row++;
    }
    if (start.col % 2 == 0) {
        start.col++;
    }
    maze_[start.row][start.col] |= builder_bit_;
    carve_path_walls_animated(start.row, start.col);
    Point walk = {1, 1};
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    Point previous_square = {};
    for (;;) {
        // Mark our progress on our current random walk. If we see this again we have looped.
        maze_[walk.row][walk.col] |= start_bit_;
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        for (const int& i : random_direction_indices) {
            const Point& p = generate_directions_[i];
            Point next = {walk.row + p.row, walk.col + p.col};
            // Do not check for seen squares, only valid moves and the direction we just came from.
            if (next.row <= 0 || next.row >= maze_row_size_ - 1
                        || next.col <= 0 || next.col >= maze_col_size_ - 1
                            || next == previous_square) {
                continue;
            }

            if (maze_[next.row][next.col] & builder_bit_) {
                build_with_marks(walk, next);
                connect_walk_to_maze(walk);
                walk = choose_arbitrary_point(Wilson_point::odd);

                // This is the only way out of the wilson algorithm! Everything is connected.
                if (!walk.row) {
                    return;
                }

                maze_[walk.row][walk.col] &= ~markers_mask_;
                previous_square = {};
            } else if (maze_[next.row][next.col] & start_bit_) {
                erase_loop(walk, next);
                walk = next;
                previous_square = {};
                Backtrack_marker mark = (maze_[walk.row][walk.col] & markers_mask_) >> marker_shift_;
                const Point& direction = backtracking_marks_[mark];
                previous_square = {walk.row + direction.row, walk.col + direction.col};
            } else {
                mark_origin(walk, next);
                previous_square = walk;
                walk = next;
            }
            break;
        }
    }
}

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


/* * * * * * * * * * * * * *    Randomized Depth First Search     * * * * * * * * * * * * * * * * */


void Thread_maze::generate_randomized_dfs_maze() {
    /* Thanks the the bits that the maze provides, we only need O(1) auxillary storage to complete
     * our recursive depth first search maze building. We will mark our current branch and complete
     * any backtracking by leaving backtracking instructions in the bits of path squares.
     */
    Point start = {row_random_(generator_), col_random_(generator_)};
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    Point cur = start;
    bool branches_remain = true;
    while (branches_remain) {
        // The unvisited neighbor is always random because array is re-shuffled each time.
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        branches_remain = false;
        for (const int& i : random_direction_indices) {
            const Point& direction = generate_directions_[i];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            if (next.row > 0 && next.row < maze_row_size_ - 1
                        && next.col > 0 && next.col < maze_col_size_ - 1
                            && !(maze_[next.row][next.col] & builder_bit_)) {
                branches_remain = true;
                carve_path_markings(cur, next);
                cur = next;
                break;
            }
        }
        if (!branches_remain && cur != start) {
            Backtrack_marker dir = (maze_[cur.row][cur.col] & markers_mask_) >> marker_shift_;
            const Point& backtracking = backtracking_marks_[dir];
            Point next = {cur.row + backtracking.row, cur.col + backtracking.col};
            // We are using fields the threads will use later. Clear bits as we backtrack.
            maze_[cur.row][cur.col] &= ~markers_mask_;
            cur = next;
            branches_remain = true;
        }
    }
}

void Thread_maze::generate_randomized_dfs_maze_animated() {
    clear_and_flush_grid();
    Point start = {row_random_(generator_), col_random_(generator_)};
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    Point cur = start;
    bool branches_remain = true;
    while (branches_remain) {
        // The unvisited neighbor is always random because array is re-shuffled each time.
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        branches_remain = false;
        for (const int& i : random_direction_indices) {
            const Point& direction = generate_directions_[i];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            if (next.row > 0 && next.row < maze_row_size_ - 1
                        && next.col > 0 && next.col < maze_col_size_ - 1
                            && !(maze_[next.row][next.col] & builder_bit_)) {
                carve_path_markings_animated(cur, next);
                branches_remain = true;
                cur = next;
                break;
            }
        }
        if (!branches_remain && cur != start) {
            Backtrack_marker dir = (maze_[cur.row][cur.col] & markers_mask_) >> marker_shift_;
            const Point& backtracking = backtracking_marks_[dir];
            Point next = {cur.row + backtracking.row, cur.col + backtracking.col};
            // We are using fields the threads will use later. Clear bits as we backtrack.
            maze_[cur.row][cur.col] &= ~markers_mask_;
            flush_cursor_maze_coordinate(cur.row, cur.col);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
            cur = next;
            branches_remain = true;
        }
    }
}


/* * * * * * * * * * * * * *        Kruskal's Algorithm          * * * * * * * * * * * * * * * * */


void Thread_maze::generate_randomized_kruskal_maze() {
    std::vector<Point> walls = load_shuffled_walls();
    std::unordered_map<Point,int> set_ids = tag_cells();
    std::vector<int> indices(set_ids.size());
    std::iota(begin(indices), end(indices), 0);
    Disjoint_set sets(indices);
    for (const Point& p : walls) {
        if (p.row % 2 == 0) {
            Point above_cell = {p.row - 1, p.col};
            Point below_cell = {p.row + 1, p.col};
            if (!sets.is_union(set_ids[above_cell], set_ids[below_cell])) {
                join_squares(above_cell, below_cell);
            }
        } else {
            Point left_cell = {p.row, p.col - 1};
            Point right_cell = {p.row, p.col + 1};
            if (!sets.is_union(set_ids[left_cell], set_ids[right_cell])) {
                join_squares(left_cell, right_cell);
            }
        }
    }
    std::cout << sets.parent_set_.size() << std::endl;
}

void Thread_maze::generate_randomized_kruskal_maze_animated() {
    std::vector<Point> walls = load_shuffled_walls();
    std::unordered_map<Point,int> set_ids = tag_cells();
    std::vector<int> indices(set_ids.size());
    std::iota(begin(indices), end(indices), 0);
    Disjoint_set sets(indices);

    clear_and_flush_grid();
    for (const Point& p : walls) {
        if (p.row % 2 == 0) {
            Point above_cell = {p.row - 1, p.col};
            Point below_cell = {p.row + 1, p.col};
            if (!sets.is_union(set_ids[above_cell], set_ids[below_cell])) {
                join_squares_animated(above_cell, below_cell);
            }
        } else {
            Point left_cell = {p.row, p.col - 1};
            Point right_cell = {p.row, p.col + 1};
            if (!sets.is_union(set_ids[left_cell], set_ids[right_cell])) {
                join_squares_animated(left_cell, right_cell);
            }
        }
    }
}

std::vector<Thread_maze::Point> Thread_maze::load_shuffled_walls() {
    std::vector<Point> walls = {};
    // The walls between cells to the left and right. If row is odd look left and right.
    for (int row = 1; row < maze_row_size_ - 1; row += 2) {
        // Cells will be odd walls will be even within a col.
        for (int col = 2; col < maze_col_size_ - 1; col += 2) {
            walls.push_back({row, col});
        }
    }
    // The walls between cells above and below. If row is even look above and below.
    for (int row = 2; row < maze_row_size_ - 1; row += 2) {
        for (int col = 1; col < maze_col_size_ - 1; col += 2) {
            walls.push_back({row, col});
        }
    }
    std::shuffle(walls.begin(), walls.end(), generator_);
    return walls;
}

std::unordered_map<Thread_maze::Point, int> Thread_maze::tag_cells() {
    std::unordered_map<Point,int> set_ids = {};
    int id = 0;
    for (int row = 1; row < maze_row_size_ - 1; row += 2) {
        // Cells will be odd walls will be even within a col.
        for (int col = 1; col < maze_col_size_ - 1; col += 2) {
            set_ids[{row,col}] = id;
            id++;
        }
    }
    return set_ids;
}

/* * * * * * * * * * * * * *       Recursive Subdivision         * * * * * * * * * * * * * * * * */


// I made an iterative implementation because I've never seen one for this algorithm.
void Thread_maze::generate_randomized_fractal_maze() {

    /* So far these are only relevant functions to this algorithm. If they become useful elsewhere
     * we can extract them out as member functions.
     */

    // All the other algorithms tunnel out passages. This one draws walls instead.
    auto connect_wall = [&](int row, int col) {
        Wall_line wall = 0b0;
        if (row - 1 >= 0 && !(maze_[row - 1][col] & path_bit_)) {
            wall |= north_wall_;
            maze_[row - 1][col] |= south_wall_;
        }
        if (row + 1 < maze_row_size_ && !(maze_[row + 1][col] & path_bit_)) {
            wall |= south_wall_;
            maze_[row + 1][col] |= north_wall_;
        }
        if (col - 1 >= 0 && !(maze_[row][col - 1] & path_bit_)) {
            wall |= west_wall_;
            maze_[row][col - 1] |= east_wall_;
        }
        if (col + 1 < maze_col_size_ && !(maze_[row][col + 1] & path_bit_)) {
            wall |= east_wall_;
            maze_[row][col + 1] |= west_wall_;
        }
        maze_[row][col] |= wall;
    };

    /* All my squares are lumped together so wall logic must always be even and passage logic must
     * but odd or vice versa. Change here if needed but they must not interfere with each other.
     */

    auto choose_division = [&](int axis_limit) -> int {
        std::uniform_int_distribution<int> divider(1, axis_limit - 2);
        int divide = divider(generator_);
        if (divide % 2 != 0) {
            divide++;
        }
        if (divide >= axis_limit - 1) {
            divide -= 2;
        }
        return divide;
    };

    auto choose_passage = [&](int axis_limit) -> int {
        std::uniform_int_distribution<int> random_passage(1, axis_limit - 2);
        int passage = random_passage(generator_);
        if (passage % 2 == 0) {
            passage++;
        }
        if (passage >= axis_limit - 1) {
            passage -= 2;
        }
        return passage;
    };

    for (int row = 1; row < maze_row_size_ - 1; row++) {
        for (int col = 1; col < maze_col_size_ - 1; col++) {
            build_path(row, col);
        }
    }
    // "Recursion" is replaced by tuple for each chamber. <chamber coordinates,height,width>.
    std::stack<std::tuple<Point,Height,Width>> chamber_stack({{{0,0},maze_row_size_,maze_col_size_}});
    while (!chamber_stack.empty()) {
        std::tuple<Point,Height,Width>& chamber = chamber_stack.top();
        const Point& chamber_offset = std::get<0>(chamber);
        int chamber_height = std::get<1>(chamber);
        int chamber_width = std::get<2>(chamber);
        if (chamber_height >= chamber_width && chamber_width > 3) {
            int divide = choose_division(chamber_height);
            int passage = choose_passage(chamber_width);
            for (int col = 0; col < chamber_width; col++) {
                if (col != passage) {
                    maze_[chamber_offset.row + divide][chamber_offset.col + col] &= ~path_bit_;
                    connect_wall(chamber_offset.row + divide, chamber_offset.col + col);
                }
            }
            // Remember to shrink height of this branch before we continue down next branch.
            std::get<1>(chamber) = divide + 1;
            Point offset = {chamber_offset.row + divide, chamber_offset.col};
            chamber_stack.push(std::make_tuple(offset, chamber_height - divide, chamber_width));
        } else if (chamber_width > chamber_height && chamber_height > 3){
            int divide = choose_division(chamber_width);
            int passage = choose_passage(chamber_height);
            for (int row = 0; row < chamber_height; row++) {
                if (row != passage) {
                    maze_[chamber_offset.row + row][chamber_offset.col + divide] &= ~path_bit_;
                    connect_wall(chamber_offset.row + row, chamber_offset.col + divide);
                }
            }
            // In this case, we are shrinking the width.
            std::get<2>(chamber) = divide + 1;
            Point offset = {chamber_offset.row, chamber_offset.col + divide};
            chamber_stack.push(std::make_tuple(offset, chamber_height, chamber_width - divide));
        } else {
            chamber_stack.pop();
        }
    }
    if (modification_ != Maze_modification::none) {
        for (int row = 0; row < maze_row_size_; row++) {
            for (int col = 0; col < maze_col_size_; col++) {
                add_modification(row, col);
            }
        }
    }
}

void Thread_maze::generate_randomized_fractal_maze_animated() {
    auto connect_wall_animated = [&](int row, int col) {
        Wall_line wall = 0b0;
        if (row - 1 >= 0 && !(maze_[row - 1][col] & path_bit_)) {
            wall |= north_wall_;
            maze_[row - 1][col] |= south_wall_;
            flush_cursor_maze_coordinate(row - 1, col);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
        }
        if (row + 1 < maze_row_size_ && !(maze_[row + 1][col] & path_bit_)) {
            wall |= south_wall_;
            maze_[row + 1][col] |= north_wall_;
            flush_cursor_maze_coordinate(row + 1, col);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
        }
        if (col - 1 >= 0 && !(maze_[row][col - 1] & path_bit_)) {
            wall |= west_wall_;
            maze_[row][col - 1] |= east_wall_;
            flush_cursor_maze_coordinate(row, col - 1);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
        }
        if (col + 1 < maze_col_size_ && !(maze_[row][col + 1] & path_bit_)) {
            wall |= east_wall_;
            maze_[row][col + 1] |= west_wall_;
            flush_cursor_maze_coordinate(row, col + 1);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
        }
        maze_[row][col] |= wall;
        flush_cursor_maze_coordinate(row, col);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    };

    auto choose_division = [&](int axis_limit) -> int {
        std::uniform_int_distribution<int> divider(1, axis_limit - 2);
        int divide = divider(generator_);
        if (divide % 2 != 0) {
            divide++;
        }
        if (divide >= axis_limit - 1) {
            divide -= 2;
        }
        return divide;
    };

    auto choose_passage = [&](int axis_limit) -> int {
        std::uniform_int_distribution<int> random_passage(1, axis_limit - 2);
        int passage = random_passage(generator_);
        if (passage % 2 == 0) {
            passage++;
        }
        if (passage >= axis_limit - 1) {
            passage -= 2;
        }
        return passage;
    };

    for (int row = 1; row < maze_row_size_ - 1; row++) {
        for (int col = 1; col < maze_col_size_ - 1; col++) {
            build_path(row, col);
        }
    }

    clear_and_flush_grid();
    std::stack<std::tuple<Point,Height,Width>> chamber_stack({{{0,0},maze_row_size_,maze_col_size_}});
    while (!chamber_stack.empty()) {
        std::tuple<Point,Height,Width>& chamber = chamber_stack.top();
        const Point& chamber_offset = std::get<0>(chamber);
        int chamber_height = std::get<1>(chamber);
        int chamber_width = std::get<2>(chamber);
        if (chamber_height >= chamber_width && chamber_width > 3) {
            int divide = choose_division(chamber_height);
            int passage = choose_passage(chamber_width);
            for (int col = 0; col < chamber_width; col++) {
                if (col != passage) {
                    maze_[chamber_offset.row + divide][chamber_offset.col + col] &= ~path_bit_;
                    connect_wall_animated(chamber_offset.row + divide,
                                          chamber_offset.col + col);
                }
            }
            std::get<1>(chamber) = divide + 1;
            Point offset = {chamber_offset.row + divide, chamber_offset.col};
            chamber_stack.push(std::make_tuple(offset, chamber_height - divide, chamber_width));
        } else if (chamber_width > chamber_height && chamber_height > 3){
            int divide = choose_division(chamber_width);
            int passage = choose_passage(chamber_height);
            for (int row = 0; row < chamber_height; row++) {
                if (row != passage) {
                    maze_[chamber_offset.row + row][chamber_offset.col + divide] &= ~path_bit_;
                    connect_wall_animated(chamber_offset.row + row,
                                          chamber_offset.col + divide);
                }
            }
            std::get<2>(chamber) = divide + 1;
            Point offset = {chamber_offset.row, chamber_offset.col + divide};
            chamber_stack.push(std::make_tuple(offset, chamber_height, chamber_width - divide));
        } else {
            chamber_stack.pop();
        }
    }
    if (modification_ != Maze_modification::none) {
        for (int row = 0; row < maze_row_size_; row++) {
            for (int col = 0; col < maze_col_size_; col++) {
                add_modification_animated(row, col);
            }
        }
    }
}


/* * * * * * * * * * * * * * * *       Randomized Grid     * * * * * * * * * * * * * * * * * * * */


void Thread_maze::generate_randomized_grid() {
    // We need an explicit stack. We run over previous paths so in place backtrack won't work.
    auto complete_run = [&](std::stack<Point>& dfs, Point cur, const Point& direction) {
        /* This value is the key to this algorithm. Simply a randomized depth first search but we
         * force the algorithm to keep running in the random direction for limited amount.
         * Shorter run_limits converge on normal depth first search, longer create longer straights.
         */
        const int run_limit = 4;
        Point next = {cur.row + direction.row, cur.col + direction.col};
        // Create the "grid" by running in one direction until wall or limit.
        int cur_run = 0;
        while (next.row < maze_row_size_ - 1  && next.col < maze_col_size_ - 1
                    && next.row > 0 && next.col > 0 && cur_run < run_limit) {
            join_squares(cur, next);
            cur = next;

            dfs.push(next);
            next.row += direction.row;
            next.col += direction.col;
            cur_run++;
        }
    };

    Point start = {row_random_(generator_), col_random_(generator_)};
    std::stack<Point> dfs({start});
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    while (!dfs.empty()) {
        // Don't pop yet!
        Point cur = dfs.top();
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        bool branches_remain = false;
        // Unvisited neighbor is always random because array is shuffled every time.
        for (const int& i : random_direction_indices) {
            // Choose another square that is two spaces a way.
            const Point& direction = generate_directions_[i];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            if (next.row > 0 && next.row < maze_row_size_ - 1
                        && next.col > 0 && next.col < maze_col_size_ - 1
                            && !(maze_[next.row][next.col] & builder_bit_)) {
                complete_run(dfs, cur, direction);
                branches_remain = true;
                break;
            }
        }
        if (!branches_remain) {
            dfs.pop();
        }
    }
}

void Thread_maze::generate_randomized_grid_animated() {
    auto complete_run_animated = [&](std::stack<Point>& dfs, Point cur,
                                     const Point& direction) {
        const int run_limit = 4;
        Point next = {cur.row + direction.row, cur.col + direction.col};
        int cur_run = 0;
        while (next.row < maze_row_size_ - 1  && next.col < maze_col_size_ - 1
                    && next.row > 0 && next.col > 0 && cur_run < run_limit) {
            join_squares_animated(cur, next);
            cur = next;
            dfs.push(next);
            next.row += direction.row;
            next.col += direction.col;
            cur_run++;
        }
    };

    clear_and_flush_grid();
    Point start = {row_random_(generator_), col_random_(generator_)};
    std::stack<Point> dfs({start});
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    while (!dfs.empty()) {
        // Don't pop yet!
        Point cur = dfs.top();
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        bool branches_remain = false;
        // Unvisited neighbor is always random because array is shuffled every time.
        for (const int& i : random_direction_indices) {
            // Choose another square that is two spaces a way.
            const Point& direction = generate_directions_[i];
            Point next = {cur.row + direction.row, cur.col + direction.col};
            if (next.row > 0 && next.row < maze_row_size_ - 1
                        && next.col > 0 && next.col < maze_col_size_ - 1
                            && !(maze_[next.row][next.col] & builder_bit_)) {
                complete_run_animated(dfs, cur, direction);
                branches_remain = true;
                break;
            }
        }
        if (!branches_remain) {
            flush_cursor_maze_coordinate(cur.row, cur.col);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
            dfs.pop();
        }
    }
}


/* * * * * * * * * * * * * * * *           Arena           * * * * * * * * * * * * * * * * * * * */


void Thread_maze::generate_arena() {
    for (int row = 2; row < maze_row_size_ - 2; row++) {
        for (int col = 2; col < maze_col_size_ - 2; col++) {
            build_path(row, col);
        }
    }
}

void Thread_maze::generate_arena_animated() {
    clear_and_flush_grid();
    for (int row = 2; row < maze_row_size_ - 2; row++) {
        for (int col = 2; col < maze_col_size_ - 2; col++) {
            carve_path_walls_animated(row, col);
        }
    }
}


/* * * * * * * * * * * * * * * * *    Maze Generation Helpers    * * * * * * * * * * * * * * * * */


void Thread_maze::carve_path_walls (int row, int col) {
    maze_[row][col] |= path_bit_;
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
    maze_[row][col] |= builder_bit_;
}

// The animated version tries to save cursor movements if they are not necessary.
void Thread_maze::carve_path_walls_animated(int row, int col) {
    maze_[row][col] |= path_bit_;
    flush_cursor_maze_coordinate(row, col);
    std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    if (row - 1 >= 0 && !(maze_[row - 1][col] & path_bit_)) {
        maze_[row - 1][col] &= ~south_wall_;
        flush_cursor_maze_coordinate(row - 1, col);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    }
    if (row + 1 < maze_row_size_ && !(maze_[row + 1][col] & path_bit_)) {
        maze_[row + 1][col] &= ~north_wall_;
        flush_cursor_maze_coordinate(row + 1, col);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    }
    if (col - 1 >= 0 && !(maze_[row][col - 1] & path_bit_)) {
        maze_[row][col - 1] &= ~east_wall_;
        flush_cursor_maze_coordinate(row, col - 1);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    }
    if (col + 1 < maze_col_size_ && !(maze_[row][col + 1] & path_bit_)) {
        maze_[row][col + 1] &= ~west_wall_;
        flush_cursor_maze_coordinate(row, col + 1);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    }
    maze_[row][col] |= builder_bit_;
}

void Thread_maze::carve_path_markings(const Point& cur, const Point& next) {
    Point wall = cur;
    carve_path_walls(cur.row, cur.col);
    if (next.row < cur.row) {
        wall.row--;
        maze_[next.row][next.col] |= from_south_;
    } else if (next.row > cur.row) {
        wall.row++;
        maze_[next.row][next.col] |= from_north_;
    } else if (next.col < cur.col) {
        wall.col--;
        maze_[next.row][next.col] |= from_east_;
    } else if (next.col > cur.col) {
        wall.col++;
        maze_[next.row][next.col] |= from_west_;
    } else {
        std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
    }
    carve_path_walls(next.row, next.col);
    carve_path_walls(wall.row, wall.col);
}

void Thread_maze::carve_path_markings_animated(const Point& cur, const Point& next) {
    Point wall = cur;
    carve_path_walls_animated(cur.row, cur.col);
    if (next.row < cur.row) {
        wall.row--;
        maze_[next.row][next.col] |= from_south_;
    } else if (next.row > cur.row) {
        wall.row++;
        maze_[next.row][next.col] |= from_north_;
    } else if (next.col < cur.col) {
        wall.col--;
        maze_[next.row][next.col] |= from_east_;
    } else if (next.col > cur.col) {
        wall.col++;
        maze_[next.row][next.col] |= from_west_;
    } else {
        std::cerr << "Wall break error. Step through wall didn't work" << std::endl;
    }
    carve_path_walls_animated(next.row, next.col);
    carve_path_walls_animated(wall.row, wall.col);
}

void Thread_maze::join_squares(const Point& cur, const Point& next) {
    Point wall = cur;
    build_path(cur.row, cur.col);
    maze_[cur.row][cur.col] |= builder_bit_;
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

void Thread_maze::join_squares_animated(const Point& cur, const Point& next) {
    Point wall = cur;
    carve_path_walls_animated(cur.row, cur.col);
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
    carve_path_walls_animated(wall.row, wall.col);
    carve_path_walls_animated(next.row, next.col);
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

void Thread_maze::build_path_animated(int row, int col) {
    maze_[row][col] |= path_bit_;
    flush_cursor_maze_coordinate(row, col);
    std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    if (row - 1 >= 0 && !(maze_[row - 1][col] & path_bit_)) {
        maze_[row - 1][col] &= ~south_wall_;
        flush_cursor_maze_coordinate(row - 1, col);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    }
    if (row + 1 < maze_row_size_ && !(maze_[row + 1][col] & path_bit_)) {
        maze_[row + 1][col] &= ~north_wall_;
        flush_cursor_maze_coordinate(row + 1, col);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    }
    if (col - 1 >= 0 && !(maze_[row][col - 1] & path_bit_)) {
        maze_[row][col - 1] &= ~east_wall_;
        flush_cursor_maze_coordinate(row, col - 1);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    }
    if (col + 1 < maze_col_size_ && !(maze_[row][col + 1] & path_bit_)) {
        maze_[row][col + 1] &= ~west_wall_;
        flush_cursor_maze_coordinate(row, col + 1);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    }
}


/* * * * * * * * * * * * * * *      Cout Printing Functions      * * * * * * * * * * * * * * * * */


void Thread_maze::print_builder() const {
    std::cout << "Maze generated with ";
    if (builder_ == Builder_algorithm::randomized_depth_first) {
        std::cout << "Randomized Depth First Search\n";
    } else if (builder_ == Builder_algorithm::randomized_loop_erased) {
        std::cout << "Loop-Erased Random Walks\n";
    } else if (builder_ == Builder_algorithm::randomized_fractal) {
        std::cout << "Randomized Recursive Subdivision\n";
    } else if (builder_ == Builder_algorithm::randomized_kruskal) {
        std::cout << "Randomized Disjoint Sets\n";
    } else if (builder_ == Builder_algorithm::randomized_grid) {
        std::cout << "Randomized Grid Runs\n";
    } else if (builder_ == Builder_algorithm::arena) {
        std::cout << "Arena\n";
    } else {
        std::cerr << "Maze builder is unset. ERROR." << std::endl;
        std::abort();
    }
}

void Thread_maze::clear_and_flush_grid() const {
    clear_screen();
    for (int row = 0; row < maze_row_size_; row++) {
        for (int col = 0; col < maze_col_size_; col++) {
            print_square(row, col);
        }
        std::cout << "\n";
    }
    std::cout << std::flush;
}

void Thread_maze::clear_screen() const {
    std::cout << ansi_clear_screen_;
}

void Thread_maze::flush_cursor_maze_coordinate(int row, int col) const {
    set_cursor_position(row, col);
    print_square(row, col);
    std::cout << std::flush;
}

void Thread_maze::set_cursor_position(int row, int col) const {
    std::string cursor_pos = "\033[" + std::to_string(row + 1) + ";" + std::to_string(col + 1) + "f";
    std::cout << cursor_pos;
}

void Thread_maze::print_maze_square(int row, int col) const {
    const Square& square = maze_[row][col];
    if (!(square & Thread_maze::path_bit_)) {
        std::cout << Thread_maze::wall_styles_[static_cast<size_t>(style_)][square & wall_mask_];
    } else if (square & Thread_maze::path_bit_) {
        std::cout << " ";
    } else {
        std::cerr << "Printed maze and a square was not categorized." << std::endl;
        abort();
    }
}

void Thread_maze::print_square(int row, int col) const {
    const Square& square = maze_[row][col];
    if (square & markers_mask_) {
        Backtrack_marker mark = (maze_[row][col] & markers_mask_) >> marker_shift_;
        std::cout << backtracking_symbols_[mark];
    } else if (!(square & path_bit_)) {
        std::cout << wall_styles_[static_cast<size_t>(style_)][square & wall_mask_];
    } else if (square & path_bit_) {
        std::cout << " ";
    } else {
        std::cerr << "Printed maze and a square was not categorized." << std::endl;
        abort();
    }
}

void Thread_maze::print_maze() const {
    for (int row = 0; row < maze_row_size_; row++) {
        for (int col = 0; col < maze_col_size_; col++) {
            print_square(row, col);
        }
        std::cout << "\n";
    }
}


/* * * * * * * * * * *     Miscellaneous Maze Functionality and Overloads      * * * * * * * * * */

void Thread_maze::clear_squares() {
    for (std::vector<Square>& row : maze_) {
        for (Square& square : row) {
            square &= ~clear_unused_;
        }
    }
}

void Thread_maze::new_maze() {
    generator_.seed(std::random_device{}());
    generate_maze(builder_);
}

void Thread_maze::new_maze(Builder_algorithm builder,
                           size_t odd_rows,
                           size_t odd_cols) {
    generator_.seed(std::random_device{}());
    builder_ = builder;
    generate_maze(builder);
}

int Thread_maze::row_size() const {
    return maze_row_size_;
}

int Thread_maze::col_size() const {
    return maze_col_size_;
}

bool Thread_maze::is_animated() const {
    return builder_speed_;
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

