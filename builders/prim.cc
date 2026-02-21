module;
#include <climits>
#include <functional>
#include <optional>
#include <queue>
#include <random>
#include <unordered_map>
#include <vector>
export module labyrinth:prim;
import :maze;
import :speed;
import :build_utilities;

///////////////////////////////////   Exported Interface

export namespace Prim {
void generate_maze(Maze::Maze &maze);
void animate_maze(Maze::Maze &maze, Speed::Speed speed);
} // namespace Prim

//////////////////////////////////   Implementation

namespace {

struct Priority_cell {
    Maze::Point cell;
    int priority;
    bool
    operator==(Priority_cell const &rhs) const {
        return this->priority == rhs.priority && this->cell == rhs.cell;
    }
    bool
    operator!=(Priority_cell const &rhs) const {
        return !(*this == rhs);
    }
    bool
    operator<(Priority_cell const &rhs) const {
        return this->priority < rhs.priority;
    }
    bool
    operator>(Priority_cell const &rhs) const {
        return this->priority > rhs.priority;
    }
    bool
    operator<=(Priority_cell const &rhs) const {
        return !(*this > rhs);
    }
    bool
    operator>=(Priority_cell const &rhs) const {
        return !(*this < rhs);
    }
};

Maze::Point
pick_random_odd_point(Maze::Maze &maze) {
    std::uniform_int_distribution<int> rand_row(1, (maze.row_size() - 2) / 2);
    std::uniform_int_distribution<int> rand_col(1, (maze.col_size() - 2) / 2);
    std::mt19937 generator(std::random_device{}());
    return {2 * rand_row(generator) + 1, 2 * rand_col(generator) + 1};
}

} // namespace

namespace Prim {

void
generate_maze(Maze::Maze &maze) {
    Butil::fill_maze_with_walls(maze);
    std::unordered_map<Maze::Point, int> cell_cost{};
    std::uniform_int_distribution<int> random_cost(0, 100);
    std::mt19937 generator(std::random_device{}());
    Maze::Point const odd_point = pick_random_odd_point(maze);
    std::priority_queue<Priority_cell, std::vector<Priority_cell>,
                        std::greater<>>
        cells;
    cells.push({odd_point, cell_cost[odd_point]});
    while (!cells.empty()) {
        Maze::Point const &cur = cells.top().cell;
        maze[cur.row][cur.col] |= Maze::builder_bit;
        std::optional<Maze::Point> min_neighbor = {};
        int min_weight = INT_MAX;
        for (Maze::Point const &p : Maze::build_dirs) {
            Maze::Point const next = {cur.row + p.row, cur.col + p.col};
            if (!Butil::can_build_new_square(maze, next)) {
                continue;
            }
            // We can generate random costs as we go efficiently thanks to
            // try_emplace not constructing if present.
            auto const cost
                = cell_cost.try_emplace(next, random_cost(generator));
            int const weight = cost.first->second;
            if (weight < min_weight) {
                min_weight = weight;
                min_neighbor = next;
            }
        }
        if (min_neighbor) {
            Butil::join_squares(maze, cur, min_neighbor.value());
            cells.push({min_neighbor.value(), min_weight});
        } else {
            cells.pop();
        }
    }
    Butil::clear_and_flush_grid(maze);
}

void
animate_maze(Maze::Maze &maze, Speed::Speed speed) {
    Speed::Speed_unit const animation_speed
        = Butil::builder_speeds.at(static_cast<int>(speed));
    Butil::fill_maze_with_walls_animated(maze);
    Butil::clear_and_flush_grid(maze);
    std::unordered_map<Maze::Point, int> cell_cost{};
    std::uniform_int_distribution<int> random_cost(0, 100);
    std::mt19937 generator(std::random_device{}());
    Maze::Point const odd_point = pick_random_odd_point(maze);
    std::priority_queue<Priority_cell, std::vector<Priority_cell>,
                        std::greater<>>
        cells;
    cells.push({odd_point, cell_cost[odd_point]});
    while (!cells.empty()) {
        Maze::Point const &cur = cells.top().cell;
        maze[cur.row][cur.col] |= Maze::builder_bit;
        std::optional<Maze::Point> min_neighbor = {};
        int min_weight = INT_MAX;
        for (Maze::Point const &p : Maze::build_dirs) {
            Maze::Point const next = {cur.row + p.row, cur.col + p.col};
            if (!Butil::can_build_new_square(maze, next)) {
                continue;
            }
            auto const cost
                = cell_cost.try_emplace(next, random_cost(generator));
            int const weight = cost.first->second;
            if (weight < min_weight) {
                min_weight = weight;
                min_neighbor = next;
            }
        }
        if (min_neighbor) {
            Butil::join_squares_animated(maze, cur, min_neighbor.value(),
                                         animation_speed);
            cells.push({min_neighbor.value(), min_weight});
        } else {
            cells.pop();
        }
    }
}

} // namespace Prim
