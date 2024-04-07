export module labyrinth:mods;
import :maze;
import :speed;
import :build_utilities;

///////////////////////////////////   Exported Interface

export namespace Mods {
void add_x(Maze::Maze &maze);
void add_x_animated(Maze::Maze &maze, Speed::Speed speed);
void add_cross(Maze::Maze &maze);
void add_cross_animated(Maze::Maze &maze, Speed::Speed speed);
} // namespace Mods

//////////////////////////////////   Implementation

namespace Mods {

void
add_x(Maze::Maze &maze)
{
    for (int row = 1; row < maze.row_size() - 1; row++)
    {
        for (int col = 1; col < maze.col_size() - 1; col++)
        {
            Butil::add_positive_slope(maze, {row, col});
            Butil::add_negative_slope(maze, {row, col});
        }
    }
}

void
add_x_animated(Maze::Maze &maze, Speed::Speed speed)
{
    const Speed::Speed_unit animation
        = Butil::builder_speeds.at(static_cast<int>(speed));
    for (int row = 1; row < maze.row_size() - 1; row++)
    {
        for (int col = 1; col < maze.col_size() - 1; col++)
        {
            Butil::add_positive_slope_animated(maze, {row, col}, animation);
            Butil::add_negative_slope_animated(maze, {row, col}, animation);
        }
    }
}

void
add_cross(Maze::Maze &maze)
{
    for (int row = 0; row < maze.row_size(); row++)
    {
        for (int col = 0; col < maze.col_size(); col++)
        {
            if ((row == maze.row_size() / 2 && col > 1
                 && col < maze.col_size() - 2)
                || (col == maze.col_size() / 2 && row > 1
                    && row < maze.row_size() - 2))
            {
                Butil::build_path(maze, {row, col});
                if (col + 1 < maze.col_size() - 2)
                {
                    Butil::build_path(maze, {row, col + 1});
                }
            }
        }
    }
}

void
add_cross_animated(Maze::Maze &maze, Speed::Speed speed)
{
    const Speed::Speed_unit animation
        = Butil::builder_speeds.at(static_cast<int>(speed));
    for (int row = 1; row < maze.row_size() - 1; row++)
    {
        for (int col = 1; col < maze.col_size() - 1; col++)
        {
            if ((row == maze.row_size() / 2 && col > 1
                 && col < maze.col_size() - 2)
                || (col == maze.col_size() / 2 && row > 1
                    && row < maze.row_size() - 2))
            {
                Butil::build_path_animated(maze, {row, col}, animation);
                if (col + 1 < maze.col_size() - 2)
                {
                    Butil::build_path_animated(maze, {row, col + 1}, animation);
                }
            }
        }
    }
}

} // namespace Mods
