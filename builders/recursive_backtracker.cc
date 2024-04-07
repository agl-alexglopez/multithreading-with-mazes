module;
#include <algorithm>
#include <chrono>
#include <iterator>
#include <numeric>
#include <random>
#include <thread>
#include <vector>
export module labyrinth:recursive_backtracker;
import :maze;
import :speed;
import :build_utilities;

///////////////////////////////////   Exported Interface

export namespace Recursive_backtracker {
void generate_maze(Maze::Maze &maze);
void animate_maze(Maze::Maze &maze, Speed::Speed speed);
} // namespace Recursive_backtracker

//////////////////////////////////   Implementation

namespace Recursive_backtracker {

constexpr Speed::Speed_unit backtrack_delay = 8;

void
generate_maze(Maze::Maze &maze)
{
    Butil::fill_maze_with_walls(maze);
    // Note that backtracking occurs by encoding directions into path bits. No
    // stack needed.
    std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> row_random(1, maze.row_size() - 2);
    std::uniform_int_distribution<int> col_random(1, maze.col_size() - 2);

    const Maze::Point start = {2 * (row_random(generator) / 2) + 1,
                               2 * (col_random(generator) / 2) + 1};
    std::vector<int> random_direction_indices(Maze::build_dirs.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices),
              0);
    Maze::Point cur = start;
    bool branches_remain = true;
    while (branches_remain)
    {
        // The unvisited neighbor is always random because array is re-shuffled
        // each time.
        shuffle(begin(random_direction_indices), end(random_direction_indices),
                generator);
        branches_remain = false;
        for (const int &i : random_direction_indices)
        {
            const Maze::Point &direction = Maze::build_dirs.at(i);
            const Maze::Point next
                = {cur.row + direction.row, cur.col + direction.col};
            if (Butil::can_build_new_square(maze, next))
            {
                branches_remain = true;
                Butil::carve_path_markings(maze, cur, next);
                cur = next;
                break;
            }
        }
        if (!branches_remain && cur != start)
        {
            const Maze::Backtrack_marker dir{static_cast<uint16_t>(
                (maze[cur.row][cur.col] & Maze::markers_mask).load()
                >> Maze::marker_shift)};
            const Maze::Point &backtracking = Maze::backtracking_marks.at(dir);
            const Maze::Point next
                = {cur.row + backtracking.row, cur.col + backtracking.col};
            // We are using fields the threads will use later. Clear bits as we
            // backtrack.
            maze[cur.row][cur.col] &= ~Maze::markers_mask;
            cur = next;
            branches_remain = true;
        }
    }
    Butil::clear_and_flush_grid(maze);
}

void
animate_maze(Maze::Maze &maze, Speed::Speed speed)
{
    const Speed::Speed_unit animation
        = Butil::builder_speeds.at(static_cast<int>(speed));
    Butil::fill_maze_with_walls_animated(maze);
    Butil::clear_and_flush_grid(maze);
    std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> row_random(1, maze.row_size() - 2);
    std::uniform_int_distribution<int> col_random(1, maze.col_size() - 2);
    const Maze::Point start = {2 * (row_random(generator) / 2) + 1,
                               2 * (col_random(generator) / 2) + 1};
    std::vector<int> random_direction_indices(Maze::build_dirs.size());
    std::iota(begin(random_direction_indices), end(random_direction_indices),
              0);
    Maze::Point cur = start;
    bool branches_remain = true;
    while (branches_remain)
    {
        shuffle(begin(random_direction_indices), end(random_direction_indices),
                generator);
        branches_remain = false;
        for (const int &i : random_direction_indices)
        {
            const Maze::Point &direction = Maze::build_dirs.at(i);
            const Maze::Point next
                = {cur.row + direction.row, cur.col + direction.col};
            if (Butil::can_build_new_square(maze, next))
            {
                Butil::carve_path_markings_animated(maze, cur, next, animation);
                branches_remain = true;
                cur = next;
                break;
            }
        }
        if (!branches_remain && cur != start)
        {
            const Maze::Backtrack_marker dir{static_cast<uint16_t>(
                (maze[cur.row][cur.col] & Maze::markers_mask).load()
                >> Maze::marker_shift)};
            const Maze::Point &backtracking = Maze::backtracking_marks.at(dir);
            const Maze::Point &backtracking_half
                = Maze::backtracking_half_marks.at(dir);
            const Maze::Point half = {cur.row + backtracking_half.row,
                                      cur.col + backtracking_half.col};
            const Maze::Point next
                = {cur.row + backtracking.row, cur.col + backtracking.col};
            // We are using fields the threads will use later. Clear bits as we
            // backtrack.
            maze[half.row][half.col]
                &= static_cast<Maze::Backtrack_marker>(~Maze::markers_mask);
            maze[cur.row][cur.col]
                &= static_cast<Maze::Backtrack_marker>(~Maze::markers_mask);
            Butil::flush_cursor_maze_coordinate(maze, half);
            std::this_thread::sleep_for(
                std::chrono::microseconds(animation * backtrack_delay));
            Butil::flush_cursor_maze_coordinate(maze, cur);
            std::this_thread::sleep_for(
                std::chrono::microseconds(animation * backtrack_delay));
            cur = next;
            branches_remain = true;
        }
    }
}

} // namespace Recursive_backtracker
