#include "thread_maze.hh"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <random>
#include <string>
#include <tuple>
#include <stack>
#include <queue>
#include <iostream>
#include <thread>
#include <vector>


Thread_maze::Thread_maze(const Thread_maze::Maze_args& args)
    : builder_(args.builder),
      modification_(args.modification),
      solver_(args.solver),
      game_(args.game),
      style_(args.style),
      solver_speed_(solver_speeds_[static_cast<size_t>(args.solver_speed)]),
      builder_speed_(builder_speeds_[static_cast<size_t>(args.builder_speed)]),
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


void Thread_maze::generate_maze(Builder_algorithm algorithm, Maze_game game) {
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
    } else {
        std::cerr << "Builder algorithm not set? Check for new builder algorithm." << std::endl;
        std::abort();
    }
    if (!solver_speed_) {
        clear_screen();
        place_start_finish();
    } else {
        place_start_finish_animated();
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
        // Be careful of the order here. Next then wall or wall alters the direction bits from next.
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
    start_ = {maze_row_size_ / 2, maze_col_size_ / 2};
    if (start_.row % 2 == 0) {
        start_.row++;
    }
    if (start_.col % 2 == 0) {
        start_.col++;
    }
    build_path(start_.row, start_.col);
    maze_[start_.row][start_.col] |= builder_bit_;
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
    start_ = {maze_row_size_ / 2, maze_col_size_ / 2};
    if (start_.row % 2 == 0) {
        start_.row++;
    }
    if (start_.col % 2 == 0) {
        start_.col++;
    }
    maze_[start_.row][start_.col] |= builder_bit_;
    carve_path_walls_animated(start_.row, start_.col);
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
            // Our walk needs to be depth first, so break after selection.
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
    start_ = {row_random_(generator_), col_random_(generator_)};
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    Point cur = start_;
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
        if (!branches_remain && cur != start_) {
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
    start_ = {row_random_(generator_), col_random_(generator_)};
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    Point cur = start_;
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
        if (!branches_remain && cur != start_) {
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
            maze_[cur.row][cur.col] |= builder_bit_;
            join_squares(cur, next);
            cur = next;

            maze_[next.row][next.col] |= builder_bit_;
            dfs.push(next);
            next.row += direction.row;
            next.col += direction.col;
            cur_run++;
        }
    };

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
            maze_[cur.row][cur.col] |= builder_bit_;
            join_squares_animated(cur, next);
            cur = next;
            maze_[next.row][next.col] |= builder_bit_;
            dfs.push(next);
            next.row += direction.row;
            next.col += direction.col;
            cur_run++;
        }
    };

    clear_and_flush_grid();
    start_ = {row_random_(generator_), col_random_(generator_)};
    std::stack<Point> dfs({start_});
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    while (!dfs.empty()) {
        // Don't pop yet!
        Point cur = dfs.top();
        maze_[cur.row][cur.col] |= zero_thread_;
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
            maze_[cur.row][cur.col] &= ~zero_thread_;
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


void Thread_maze::place_start_finish() {
    // Dimensions of a maze vary based on random build generation. We need this atrocity.
    auto set_corner = [&] (int row_init, int col_init,
                           int row_end, int col_end,
                           bool increment_row, bool increment_col,
                           int corner_array_index){
        for (int row = row_init;
                (increment_row ? row < row_end : row > row_end);
                    increment_row ? row++ : row--) {
            for (int col = col_init;
                    (increment_col ? col < col_end : col > col_end);
                        increment_col ? col++ : col--) {
                if (maze_[row][col] & path_bit_) {
                    maze_[row][col] |= start_bit_;
                    corner_starts_[corner_array_index] = {row, col};
                    return;
                }
            }
        }
    };

    if (game_ == Maze_game::corners) {
        set_corner(1, 1, maze_row_size_ - 2, maze_col_size_ - 2, true, true, 0);
        set_corner(1, maze_col_size_ - 2, maze_row_size_ - 2, 0, true, false, 1);
        set_corner(maze_row_size_ - 2, 1, 0, maze_col_size_ - 2, false, true, 2);
        set_corner(maze_row_size_ - 2, maze_col_size_ - 2, 0, 0, false, false, 3);
        int middle_row = maze_row_size_ / 2;
        int middle_col = maze_col_size_ / 2;
        finish_ = {middle_row, middle_col};
        maze_[middle_row][middle_col] |= path_bit_;
        maze_[middle_row + 1][middle_col] |= path_bit_;
        maze_[middle_row][middle_col + 1] |= path_bit_;
        maze_[middle_row - 1][middle_col] |= path_bit_;
        maze_[middle_row][middle_col - 1] |= path_bit_;
        maze_[middle_row][middle_col] |= finish_bit_;
    } else {
        start_ = pick_random_point();
        maze_[start_.row][start_.col] |= start_bit_;
        int num_finishes = game_ == Maze_game::gather ? 4 : 1;
        for (int placement = 0; placement < num_finishes; placement++) {
            finish_ = pick_random_point();
            maze_[finish_.row][finish_.col] |= finish_bit_;
        }
    }
}

void Thread_maze::place_start_finish_animated() {
    // Dimensions of a maze vary based on random build generation. We need this atrocity.
    auto set_corner = [&] (int row_init, int col_init,
                           int row_end, int col_end,
                           bool increment_row, bool increment_col,
                           int corner_array_index){
        for (int row = row_init;
                (increment_row ? row < row_end : row > row_end);
                    increment_row ? row++ : row--) {
            for (int col = col_init;
                    (increment_col ? col < col_end : col > col_end);
                        increment_col ? col++ : col--) {
                if (maze_[row][col] & path_bit_) {
                    maze_[row][col] |= start_bit_;
                    corner_starts_[corner_array_index] = {row, col};
                    flush_cursor_maze_coordinate(row, col);
                    std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
                    return;
                }
            }
        }
    };

    if (game_ == Maze_game::corners) {
        set_corner(1, 1, maze_row_size_ - 2, maze_col_size_ - 2, true, true, 0);
        set_corner(1, maze_col_size_ - 2, maze_row_size_ - 2, 0, true, false, 1);
        set_corner(maze_row_size_ - 2, 1, 0, maze_col_size_ - 2, false, true, 2);
        set_corner(maze_row_size_ - 2, maze_col_size_ - 2, 0, 0, false, false, 3);
        int middle_row = maze_row_size_ / 2;
        int middle_col = maze_col_size_ / 2;
        finish_ = {middle_row, middle_col};
        for (const Point& p : all_directions_) {
            Point next = {finish_.row + p.row, finish_.col + p.col};
            maze_[next.row][next.col] |= path_bit_;
            flush_cursor_maze_coordinate(next.row, next.col);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
        }
        maze_[middle_row][middle_col] |= path_bit_;
        maze_[middle_row][middle_col] |= finish_bit_;
        flush_cursor_maze_coordinate(middle_row, middle_col);
        std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
    } else {
        start_ = pick_random_point();
        maze_[start_.row][start_.col] |= start_bit_;
        int num_finishes = game_ == Maze_game::gather ? 4 : 1;
        for (int placement = 0; placement < num_finishes; placement++) {
            finish_ = pick_random_point();
            maze_[finish_.row][finish_.col] |= finish_bit_;
            flush_cursor_maze_coordinate(finish_.row, finish_.col);
            std::this_thread::sleep_for(std::chrono::microseconds(builder_speed_));
        }
    }
}

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

Thread_maze::Point Thread_maze::pick_random_point() {
    Point choice = {row_random_(generator_), col_random_(generator_)};
    if (!(maze_[choice.row][choice.col] & path_bit_)
            || maze_[choice.row][choice.col] & finish_bit_
                || maze_[choice.row][choice.col] & start_bit_) {
        choice = find_nearest_square(choice);
    }
    return choice;
}

Thread_maze::Point Thread_maze::find_nearest_square(Thread_maze::Point choice) {
    // Fanning out from a starting point should work on any medium to large maze.
    for (const Point& p : all_directions_) {
        Point next = {choice.row + p.row, choice.col + p.col};
        if (next.row > 0 && next.row < maze_row_size_ - 1
                && next.col > 0 && next.col < maze_col_size_ - 1
                    && (maze_[next.row][next.col] & path_bit_)
                        && !(maze_[next.row][next.col] & start_bit_)
                            && !(maze_[next.row][next.col] & finish_bit_)) {
            return next;
        }
    }
    // Getting desperate here. We should only need this for very small mazes.
    for (int row = 1; row < maze_row_size_ - 1; row++) {
        for (int col = 1; col < maze_col_size_ - 1; col++) {
            if ((maze_[row][col] & path_bit_)
                    && !(maze_[row][col] & start_bit_)
                        && !(maze_[row][col] & finish_bit_)) {
                return {row, col};
            }
        }
    }
    std::cerr << "Could not place a point. Bad point = "
              << "{" << choice.row << "," << choice.col << "}" << std::endl;
    print_maze();
    std::abort();
}


/* * * * * * * * * * * * * * * * *      Maze Solvers Caller      * * * * * * * * * * * * * * * * */


void Thread_maze::solve_maze(Thread_maze::Solver_algorithm solver) {
    if (solver == Solver_algorithm::depth_first_search) {
        if (solver_speed_) {
            animate_with_dfs_threads();
        } else {
            solve_with_dfs_threads();
        }
    } else if (solver == Solver_algorithm::randomized_depth_first_search) {
        if (solver_speed_) {
            animate_with_randomized_dfs_threads();
        } else {
            solve_with_randomized_dfs_threads();
        }
    } else if (solver == Solver_algorithm::breadth_first_search) {
        if (solver_speed_) {
            animate_with_bfs_threads();
        } else {
            solve_with_bfs_threads();
        }
    } else {
        std::cerr << "Invalid solver?" << std::endl;
        abort();
    }
}

void Thread_maze::solve_maze() {
    solve_maze(solver_);
}


/* * * * * * * * * * * * * * * * *      Depth First Search       * * * * * * * * * * * * * * * * */


void Thread_maze::solve_with_dfs_threads() {
    if (escape_path_index_ != -1) {
        clear_paths();
    }
    std::vector<std::thread> threads(cardinal_directions_.size());
    if (game_ == Maze_game::hunt) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                dfs_thread_hunt(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::gather) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                dfs_thread_gather(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::corners) {
        // Randomly shuffle thread start corners so colors mix differently each time.
        std::vector<int> random_starting_corners(corner_starts_.size());
        std::iota(begin(random_starting_corners), end(random_starting_corners), 0);
        shuffle(begin(random_starting_corners), end(random_starting_corners), generator_);
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            const Point& corner = corner_starts_[random_starting_corners[i]];
            threads[i] = std::thread([this, corner, i, thread_mask] {
                dfs_thread_hunt(corner, i, thread_mask);
            });
        }
    } else {
        std::cerr << "Uncategorized game slipped through initializations." << std::endl;
        std::abort();
    }

    for (std::thread& t : threads) {
        t.join();
    }
    print_maze();
    print_overlap_key();
    print_builder();
    print_solver();
    print_solution_path();
}

void Thread_maze::animate_with_dfs_threads() {
    if (escape_path_index_ != -1) {
        clear_paths();
    }

    if (!builder_speed_) {
        clear_and_flush_grid();
    }
    set_cursor_position(maze_row_size_, 0);
    print_overlap_key();
    print_builder();
    print_solver();
    std::cout << std::flush;

    std::vector<std::thread> threads(cardinal_directions_.size());
    if (game_ == Maze_game::hunt) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                dfs_thread_hunt_animated(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::gather) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                dfs_thread_gather_animated(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::corners) {
        // Randomly shuffle thread start corners so colors mix differently each time.
        std::vector<int> random_starting_corners(corner_starts_.size());
        std::iota(begin(random_starting_corners), end(random_starting_corners), 0);
        shuffle(begin(random_starting_corners), end(random_starting_corners), generator_);
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            const Point& corner = corner_starts_[random_starting_corners[i]];
            threads[i] = std::thread([this, corner, i, thread_mask] {
                dfs_thread_hunt_animated(corner, i, thread_mask);
            });
        }
    } else {
        std::cerr << "Uncategorized game slipped through initializations." << std::endl;
        std::abort();
    }

    for (std::thread& t : threads) {
        t.join();
    }
    set_cursor_position(maze_row_size_ + overlap_key_and_message_height, 0);
    print_solution_path();
    std::cout << std::endl;
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
        // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
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

        // Bias each thread's first choice towards orginal dispatch direction. More coverage.
        int direction_index = thread_index;
        bool found_branch_to_explore = false;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen)
                    && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                // Emulate a true recursive dfs. Only push the current branch onto our stack.
                found_branch_to_explore = true;
                dfs.push(next);
                break;
            }
            maze_mutex_.unlock();
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_index);
        if (!found_branch_to_explore) {
            dfs.pop();
        }
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

bool Thread_maze::dfs_thread_hunt_animated(Point start, int thread_index, Thread_paint paint) {
    Thread_cache seen = paint << thread_tag_offset_;
    std::stack<Point> dfs({start});
    bool result = false;
    Point cur = start;
    while (!dfs.empty()) {
        // Lock? Garbage read stolen mid write by winning thread is still ok for program logic.
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
        maze_[cur.row][cur.col] |= paint;
        flush_cursor_maze_coordinate(cur.row, cur.col);
        maze_mutex_.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(solver_speed_));

        // Bias each thread's first choice towards orginal dispatch direction. More coverage.
        int direction_index = thread_index;
        bool found_branch_to_explore = false;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen)
                    && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                // Emulate a true recursive dfs. Only push the current branch onto our stack.
                found_branch_to_explore = true;
                dfs.push(next);
                break;
            }
            maze_mutex_.unlock();
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_index);
        if (!found_branch_to_explore) {
            maze_mutex_.lock();
            maze_[cur.row][cur.col] &= ~paint;
            flush_cursor_maze_coordinate(cur.row, cur.col);
            maze_mutex_.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(solver_speed_));
            dfs.pop();
        }
    }
    // Another benefit of true depth first search is our stack holds path to exact location.
    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
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
        // Bias each thread's first choice towards orginal dispatch direction. More coverage.
        int direction_index = thread_index;
        bool found_branch_to_explore = false;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen)
                    && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                found_branch_to_explore = true;
                dfs.push(next);
                break;
            }
            maze_mutex_.unlock();
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_index);
        if (!found_branch_to_explore) {
            dfs.pop();
        }
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

bool Thread_maze::dfs_thread_gather_animated(Point start, int thread_index, Thread_paint paint) {
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
        maze_[cur.row][cur.col] |= paint;
        flush_cursor_maze_coordinate(cur.row, cur.col);
        maze_mutex_.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(solver_speed_));

        // Bias each thread's first choice towards orginal dispatch direction. More coverage.
        int direction_index = thread_index;
        bool found_branch_to_explore = false;
        do {
            const Point& p = cardinal_directions_[direction_index];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen)
                    && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                found_branch_to_explore = true;
                dfs.push(next);
                break;
            }
            maze_mutex_.unlock();
            ++direction_index %= cardinal_directions_.size();
        } while (direction_index != thread_index);
        if (!found_branch_to_explore) {
            maze_mutex_.lock();
            maze_[cur.row][cur.col] &= ~paint;
            flush_cursor_maze_coordinate(cur.row, cur.col);
            maze_mutex_.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(solver_speed_));
            dfs.pop();
        }
    }
    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
    }
    return result;
}


/* * * * * * * * * * * * * * *   Randomized Depth First Search   * * * * * * * * * * * * * * * * */


void Thread_maze::animate_with_randomized_dfs_threads() {
    if (escape_path_index_ != -1) {
        clear_paths();
    }
    if (!builder_speed_) {
        clear_and_flush_grid();
    }
    set_cursor_position(maze_row_size_, 0);
    print_overlap_key();
    print_builder();
    print_solver();
    std::cout << std::flush;

    std::vector<std::thread> threads(cardinal_directions_.size());
    if (game_ == Maze_game::hunt) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                randomized_dfs_thread_hunt_animated(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::gather) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                randomized_dfs_thread_gather_animated(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::corners) {
        std::vector<int> random_starting_corners(corner_starts_.size());
        std::iota(begin(random_starting_corners), end(random_starting_corners), 0);
        shuffle(begin(random_starting_corners), end(random_starting_corners), generator_);
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            const Point& corner = corner_starts_[random_starting_corners[i]];
            threads[i] = std::thread([this, corner, i, thread_mask] {
                randomized_dfs_thread_hunt_animated(corner, i, thread_mask);
            });
        }
    } else {
        std::cerr << "Uncategorized game slipped through initializations." << std::endl;
        std::abort();
    }
    for (std::thread& t : threads) {
        t.join();
    }
    set_cursor_position(maze_row_size_ + overlap_key_and_message_height, 0);
    print_solution_path();
    std::cout << std::endl;
}

void Thread_maze::solve_with_randomized_dfs_threads() {
    if (escape_path_index_ != -1) {
        clear_paths();
    }
    std::vector<std::thread> threads(cardinal_directions_.size());
    if (game_ == Maze_game::hunt) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                randomized_dfs_thread_hunt(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::gather) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                randomized_dfs_thread_gather(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::corners) {
        std::vector<int> random_starting_corners(corner_starts_.size());
        std::iota(begin(random_starting_corners), end(random_starting_corners), 0);
        shuffle(begin(random_starting_corners), end(random_starting_corners), generator_);
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            const Point& corner = corner_starts_[random_starting_corners[i]];
            threads[i] = std::thread([this, corner, i, thread_mask] {
                randomized_dfs_thread_hunt(corner, i, thread_mask);
            });
        }
    } else {
        std::cerr << "Uncategorized game slipped through initializations." << std::endl;
        std::abort();
    }
    for (std::thread& t : threads) {
        t.join();
    }
    print_maze();
    print_overlap_key();
    print_builder();
    print_solver();
    print_solution_path();
    std::cout << std::endl;
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
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        bool found_branch_to_explore = false;
        for (const int& i : random_direction_indices) {
            const Point& p = cardinal_directions_[i];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen)
                    && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                found_branch_to_explore = true;
                dfs.push(next);
                break;
            }
            maze_mutex_.unlock();
        }
        if (!found_branch_to_explore) {
            dfs.pop();
        }
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

bool Thread_maze::randomized_dfs_thread_hunt_animated(Point start, int thread_index, Thread_paint paint) {
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
        maze_[cur.row][cur.col] |= paint;
        flush_cursor_maze_coordinate(cur.row, cur.col);
        maze_mutex_.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(solver_speed_));

        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        bool found_branch_to_explore = false;
        for (const int& i : random_direction_indices) {
            const Point& p = cardinal_directions_[i];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen)
                    && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                found_branch_to_explore = true;
                dfs.push(next);
                break;
            }
            maze_mutex_.unlock();
        }
        if (!found_branch_to_explore) {
            maze_mutex_.lock();
            maze_[cur.row][cur.col] &= ~paint;
            flush_cursor_maze_coordinate(cur.row, cur.col);
            maze_mutex_.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(solver_speed_));
            dfs.pop();
        }
    }

    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
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
        if (maze_[cur.row][cur.col] & finish_bit_
                && !(maze_[cur.row][cur.col] & cache_mask_)){
            maze_[cur.row][cur.col] |= seen;
            maze_mutex_.unlock();
            result = true;
            dfs.pop();
            break;
        }
        maze_[cur.row][cur.col] |= seen;
        maze_mutex_.unlock();
        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        bool found_branch_to_explore = false;
        for (const int& i : random_direction_indices) {
            const Point& p = cardinal_directions_[i];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen)
                    && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                found_branch_to_explore = true;
                dfs.push(next);
                break;
            }
            maze_mutex_.unlock();
        };
        if (!found_branch_to_explore) {
            dfs.pop();
        }
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

bool Thread_maze::randomized_dfs_thread_gather_animated(Point start, int thread_index, Thread_paint paint) {
    Thread_cache seen = paint << thread_tag_offset_;
    std::stack<Point> dfs({start});
    bool result = false;
    Point cur = start;
    std::vector<int> random_direction_indices(generate_directions_.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices), 0);
    while (!dfs.empty()) {
        cur = dfs.top();
        maze_mutex_.lock();
        if (maze_[cur.row][cur.col] & finish_bit_
                && !(maze_[cur.row][cur.col] & cache_mask_)){
            maze_[cur.row][cur.col] |= seen;
            maze_mutex_.unlock();
            result = true;
            dfs.pop();
            break;
        }
        maze_[cur.row][cur.col] |= seen;
        maze_[cur.row][cur.col] |= paint;
        flush_cursor_maze_coordinate(cur.row, cur.col);
        maze_mutex_.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(solver_speed_));

        shuffle(begin(random_direction_indices), end(random_direction_indices), generator_);
        bool found_branch_to_explore = false;
        for (const int& i : random_direction_indices) {
            const Point& p = cardinal_directions_[i];
            Point next = {cur.row + p.row, cur.col + p.col};
            maze_mutex_.lock();
            if (!(maze_[next.row][next.col] & seen)
                    && (maze_[next.row][next.col] & path_bit_)) {
                maze_mutex_.unlock();
                found_branch_to_explore = true;
                dfs.push(next);
                break;
            }
            maze_mutex_.unlock();
        };
        if (!found_branch_to_explore) {
            maze_mutex_.lock();
            maze_[cur.row][cur.col] &= ~paint;
            flush_cursor_maze_coordinate(cur.row, cur.col);
            maze_mutex_.unlock();
            dfs.pop();
        }
    }

    while (!dfs.empty()) {
        cur = dfs.top();
        dfs.pop();
        thread_paths_[thread_index].push_back(cur);
    }
    return result;
}


/* * * * * * * * * * * * * * *        Breadth First Search       * * * * * * * * * * * * * * * * */


void Thread_maze::solve_with_bfs_threads() {
    if (escape_path_index_ != -1) {
        clear_paths();
    }

    std::vector<std::thread> threads(cardinal_directions_.size());
    if (game_ == Maze_game::hunt) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                bfs_thread_hunt(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::gather) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                bfs_thread_gather(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::corners) {
        std::vector<int> random_starting_corners(corner_starts_.size());
        std::iota(begin(random_starting_corners), end(random_starting_corners), 0);
        shuffle(begin(random_starting_corners), end(random_starting_corners), generator_);
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            const Point& corner = corner_starts_[random_starting_corners[i]];
            threads[i] = std::thread([this, &corner, i, thread_mask] {
                bfs_thread_hunt(corner, i, thread_mask);
            });
        }
    } else {
        std::cerr << "Uncategorized game slipped through initializations." << std::endl;
        std::abort();
    }

    for (std::thread& t : threads) {
        t.join();
    }
    if (game_ == Maze_game::gather) {
        // Too chaotic to show all paths. So we will make a color flag near each finish.
        int thread = 0;
        for (const std::vector<Point>& path : thread_paths_) {
            Thread_paint single_color = thread_masks_[thread++];
            const Point& p = path.front();
            maze_[p.row][p.col] &= ~thread_mask_;
            maze_[p.row][p.col] |= single_color;
        }
    } else {
        // It is cool to see the shortest path that the winning thread took to victory
        Thread_paint winner_color = thread_masks_[escape_path_index_];
        for (const Point& p : thread_paths_[escape_path_index_]) {
            maze_[p.row][p.col] &= ~thread_mask_;
            maze_[p.row][p.col] |= winner_color;
        }
    }
    print_maze();
    print_overlap_key();
    print_builder();
    print_solver();
    print_solution_path();
}

void Thread_maze::animate_with_bfs_threads() {
    if (escape_path_index_ != -1) {
        clear_paths();
    }

    if (!builder_speed_) {
        clear_and_flush_grid();
    }
    set_cursor_position(maze_row_size_, 0);
    print_overlap_key();
    print_builder();
    print_solver();
    std::cout << std::flush;

    std::vector<std::thread> threads(cardinal_directions_.size());
    if (game_ == Maze_game::hunt) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                bfs_thread_hunt_animated(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::gather) {
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            threads[i] = std::thread([this, i, thread_mask] {
                bfs_thread_gather_animated(start_, i, thread_mask);
            });
        }
    } else if (game_ == Maze_game::corners) {
        std::vector<int> random_starting_corners(corner_starts_.size());
        std::iota(begin(random_starting_corners), end(random_starting_corners), 0);
        shuffle(begin(random_starting_corners), end(random_starting_corners), generator_);
        for (int i = 0; i < num_threads_; i++) {
            const Thread_paint& thread_mask = thread_masks_[i];
            const Point& corner = corner_starts_[random_starting_corners[i]];
            threads[i] = std::thread([this, &corner, i, thread_mask] {
                bfs_thread_hunt_animated(corner, i, thread_mask);
            });
        }
    } else {
        std::cerr << "Uncategorized game slipped through initializations." << std::endl;
        std::abort();
    }

    for (std::thread& t : threads) {
        t.join();
    }
    if (game_ == Maze_game::gather) {
        // Too chaotic to show all paths. So we will make a color flag near each finish.
        int thread = 0;
        for (const std::vector<Point>& path : thread_paths_) {
            Thread_paint single_color = thread_masks_[thread++];
            const Point& p = path.front();
            maze_[p.row][p.col] &= ~thread_mask_;
            maze_[p.row][p.col] |= single_color;
            flush_cursor_maze_coordinate(p.row, p.col);
        }
    } else {
        // It is cool to see the shortest path that the winning thread took to victory
        Thread_paint winner_color = thread_masks_[escape_path_index_];
        for (const Point& p : thread_paths_[escape_path_index_]) {
            maze_[p.row][p.col] &= ~thread_mask_;
            maze_[p.row][p.col] |= winner_color;
            flush_cursor_maze_coordinate(p.row, p.col);
        }
    }
    set_cursor_position(maze_row_size_ + overlap_key_and_message_height, 0);
    print_solution_path();
    std::cout << std::endl;
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

bool Thread_maze::bfs_thread_hunt_animated(Point start, int thread_index, Thread_paint paint) {
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
        flush_cursor_maze_coordinate(cur.row, cur.col);
        maze_mutex_.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(solver_speed_));

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
    escape_path_index_ = thread_index;
    return result;
}

bool Thread_maze::bfs_thread_gather_animated(Point start, int thread_index, Thread_paint paint) {
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
        flush_cursor_maze_coordinate(cur.row, cur.col);
        maze_mutex_.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(solver_speed_));

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
    escape_path_index_ = thread_index;
    return result;
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
    } else if (builder_ == Builder_algorithm::randomized_grid) {
        std::cout << "Randomized Grid Runs\n";
    } else if (builder_ == Builder_algorithm::arena) {
        std::cout << "Arena\n";
    } else {
        std::cerr << "Maze builder is unset. ERROR." << std::endl;
        std::abort();
    }
}

void Thread_maze::print_solver() const {
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
}

void Thread_maze::print_solution_path() {
    if (game_ == Maze_game::gather) {
        for (const Thread_paint& mask : thread_masks_) {
            std::cout << thread_colors_.at(mask >> thread_tag_offset_) << "█" << ansi_nil_;
        }
        std::cout << " All threads found their finish squares!\n";
    } else {
        std::cout << thread_colors_.at(thread_masks_[escape_path_index_] >> thread_tag_offset_)
                  << "█" << " thread won!" << ansi_nil_ << "\n";
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
    std::cout << ansi_clear_screen;
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

void Thread_maze::print_square(int row, int col) const {
    const Square& square = maze_[row][col];
    if (square & finish_bit_) {
        std::cout << ansi_bold_ << ansi_cyn_ << "F" << ansi_nil_;
    } else if (square & start_bit_) {
        std::cout << ansi_bold_ << ansi_cyn_ << "S" << ansi_nil_;
    } else if (square & thread_mask_) {
        Thread_paint thread_color = (square & thread_mask_) >> thread_tag_offset_;
        std::cout << thread_colors_[thread_color] << "█" << ansi_nil_;
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
               << "└─────────────┴─────────────┴─────────────┴─────────────┴─────────────┘\n";
}


/* * * * * * * * * * *     Miscellaneous Maze Functionality and Overloads      * * * * * * * * * */


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

