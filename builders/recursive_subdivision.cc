module;
#include <random>
#include <stack>
#include <tuple>
export module labyrinth:recursive_subdivision;
import :maze;
import :speed;
import :build_utilities;

///////////////////////////////////   Exported Interface

export namespace Recursive_subdivision {
void generate_maze(Maze::Maze &maze);
void animate_maze(Maze::Maze &maze, Speed::Speed speed);
} // namespace Recursive_subdivision

//////////////////////////////////   Implementation

namespace {

using Height = int;
using Width = int;

int
random_even_division(std::mt19937 &generator, int axis_limit)
{
    std::uniform_int_distribution<int> divider(1, (axis_limit - 2) / 2);
    return 2 * divider(generator);
}

int
random_odd_passage(std::mt19937 &generator, int axis_limit)
{
    std::uniform_int_distribution<int> divider(1, (axis_limit - 2) / 2);
    return 2 * divider(generator) + 1;
}

} // namespace

namespace Recursive_subdivision {

void
generate_maze(Maze::Maze &maze)
{
    Butil::build_wall_outline(maze);
    std::mt19937 generator(std::random_device{}());
    std::stack<std::tuple<Maze::Point, Height, Width>> chamber_stack(
        {{{0, 0}, maze.row_size(), maze.col_size()}});
    while (!chamber_stack.empty())
    {
        std::tuple<Maze::Point, Height, Width> &chamber = chamber_stack.top();
        const Maze::Point &chamber_offset = std::get<0>(chamber);
        const int chamber_height = std::get<1>(chamber);
        const int chamber_width = std::get<2>(chamber);
        if (chamber_height >= chamber_width && chamber_width > 3)
        {
            const int divide = random_even_division(generator, chamber_height);
            const int passage = random_odd_passage(generator, chamber_width);
            for (int col = 0; col < chamber_width; col++)
            {
                if (col != passage)
                {
                    maze[chamber_offset.row + divide][chamber_offset.col + col]
                        &= ~Maze::path_bit;
                    Butil::build_wall_line(maze, {chamber_offset.row + divide,
                                                  chamber_offset.col + col});
                }
            }
            // Remember to shrink height of this branch before we continue down
            // next branch.
            std::get<1>(chamber) = divide + 1;
            const Maze::Point offset
                = {chamber_offset.row + divide, chamber_offset.col};
            chamber_stack.emplace(offset, chamber_height - divide,
                                  chamber_width);
        }
        else if (chamber_width > chamber_height && chamber_height > 3)
        {
            const int divide = random_even_division(generator, chamber_width);
            const int passage = random_odd_passage(generator, chamber_height);
            for (int row = 0; row < chamber_height; row++)
            {
                if (row != passage)
                {
                    maze[chamber_offset.row + row][chamber_offset.col + divide]
                        &= ~Maze::path_bit;
                    Butil::build_wall_line(maze, {chamber_offset.row + row,
                                                  chamber_offset.col + divide});
                }
            }
            // In this case, we are shrinking the width.
            std::get<2>(chamber) = divide + 1;
            const Maze::Point offset
                = {chamber_offset.row, chamber_offset.col + divide};
            chamber_stack.emplace(offset, chamber_height,
                                  chamber_width - divide);
        }
        else
        {
            chamber_stack.pop();
        }
    }
    Butil::clear_and_flush_grid(maze);
}

void
animate_maze(Maze::Maze &maze, Speed::Speed speed)
{
    const Speed::Speed_unit animation
        = Butil::builder_speeds.at(static_cast<int>(speed));
    Butil::build_wall_outline(maze);
    Butil::clear_and_flush_grid(maze);
    std::mt19937 generator(std::random_device{}());
    std::stack<std::tuple<Maze::Point, Height, Width>> chamber_stack(
        {{{0, 0}, maze.row_size(), maze.col_size()}});
    while (!chamber_stack.empty())
    {
        std::tuple<Maze::Point, Height, Width> &chamber = chamber_stack.top();
        const Maze::Point &chamber_offset = std::get<0>(chamber);
        const int chamber_height = std::get<1>(chamber);
        const int chamber_width = std::get<2>(chamber);
        if (chamber_height >= chamber_width && chamber_width > 3)
        {
            const int divide = random_even_division(generator, chamber_height);
            const int passage = random_odd_passage(generator, chamber_width);
            for (int col = 0; col < chamber_width; col++)
            {
                if (col != passage)
                {
                    maze[chamber_offset.row + divide][chamber_offset.col + col]
                        &= ~Maze::path_bit;
                    Butil::build_wall_line_animated(
                        maze,
                        {chamber_offset.row + divide, chamber_offset.col + col},
                        animation);
                }
            }
            std::get<1>(chamber) = divide + 1;
            const Maze::Point offset
                = {chamber_offset.row + divide, chamber_offset.col};
            chamber_stack.emplace(offset, chamber_height - divide,
                                  chamber_width);
        }
        else if (chamber_width > chamber_height && chamber_height > 3)
        {
            const int divide = random_even_division(generator, chamber_width);
            const int passage = random_odd_passage(generator, chamber_height);
            for (int row = 0; row < chamber_height; row++)
            {
                if (row != passage)
                {
                    maze[chamber_offset.row + row][chamber_offset.col + divide]
                        &= ~Maze::path_bit;
                    Butil::build_wall_line_animated(
                        maze,
                        {chamber_offset.row + row, chamber_offset.col + divide},
                        animation);
                }
            }
            std::get<2>(chamber) = divide + 1;
            const Maze::Point offset
                = {chamber_offset.row, chamber_offset.col + divide};
            chamber_stack.emplace(offset, chamber_height,
                                  chamber_width - divide);
        }
        else
        {
            chamber_stack.pop();
        }
    }
}

} // namespace Recursive_subdivision
