import labyrinth;
#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace {

using Build_function
    = std::tuple<std::function<void(Maze::Maze &)>,
                 std::function<void(Maze::Maze &, Speed::Speed)>>;

using Paint_function
    = std::tuple<std::function<void(Maze::Maze &)>,
                 std::function<void(Maze::Maze &, Speed::Speed)>>;

constexpr int static_image = 0;
constexpr int animated_playback = 1;

struct Flag_arg
{
    std::string_view flag;
    std::string_view arg;
};

struct Maze_runner
{
    Maze::Maze_args args;

    int builder_view{static_image};
    Speed::Speed builder_speed{};
    Build_function builder{Recursive_backtracker::generate_maze,
                           Recursive_backtracker::animate_maze};

    int modification_getter{static_image};
    std::optional<Build_function> modder;

    int painter_view{static_image};
    Speed::Speed painter_speed{};
    Paint_function painter{Distance::paint_distance_from_center,
                           Distance::animate_distance_from_center};
    Maze_runner() : args{}
    {}
};

struct Lookup_tables
{
    std::unordered_set<std::string> argument_flags;
    std::unordered_map<std::string, Build_function> builder_table;
    std::unordered_map<std::string, Build_function> modification_table;
    std::unordered_map<std::string, Paint_function> painter_table;
    std::unordered_map<std::string, Maze::Maze_style> style_table;
    std::unordered_map<std::string, Speed::Speed> painter_animation_table;
    std::unordered_map<std::string, Speed::Speed> builder_animation_table;
};

void set_relevant_arg(const Lookup_tables &tables, Maze_runner &runner,
                      const Flag_arg &pairs);
void set_rows(Maze_runner &runner, const Flag_arg &pairs);
void set_cols(Maze_runner &runner, const Flag_arg &pairs);
void print_invalid_arg(const Flag_arg &pairs);
void print_usage();

} // namespace

int
main(int argc, char **argv)
{
    const Lookup_tables tables = {
        .argument_flags={"-r", "-c", "-b", "-p", "-h", "-g", "-d", "-m", "-pa", "-ba"},
        .builder_table={
            {"rdfs",
             {Recursive_backtracker::generate_maze,
              Recursive_backtracker::animate_maze}},
            {"wilson",
             {Wilson_path_carver::generate_maze,
              Wilson_path_carver::animate_maze}},
            {"wilson-walls",
             {Wilson_wall_adder::generate_maze,
              Wilson_wall_adder::animate_maze}},
            {"fractal",
             {Recursive_subdivision::generate_maze,
              Recursive_subdivision::animate_maze}},
            {"kruskal", {Kruskal::generate_maze, Kruskal::animate_maze}},
            {"eller", {Eller::generate_maze, Eller::animate_maze}},
            {"prim", {Prim::generate_maze, Prim::animate_maze}},
            {"grid", {Grid::generate_maze, Grid::animate_maze}},
            {"arena", {Arena::generate_maze, Arena::animate_maze}},
        },
        .modification_table={
            {"cross", {Mods::add_cross, Mods::add_cross_animated}},
            {"x", {Mods::add_x, Mods::add_x_animated}},
        },
        .painter_table={
            {"distance",
             {Distance::paint_distance_from_center,
              Distance::animate_distance_from_center}},
            {"runs", {Runs::paint_runs, Runs::animate_runs}},
        },
        .style_table={
            {"sharp", Maze::Maze_style::sharp},
            {"round", Maze::Maze_style::round},
            {"doubles", Maze::Maze_style::doubles},
            {"bold", Maze::Maze_style::bold},
            {"contrast", Maze::Maze_style::contrast},
            {"spikes", Maze::Maze_style::spikes},
        },
        .painter_animation_table={
            {"0", Speed::Speed::instant},
            {"1", Speed::Speed::speed_1},
            {"2", Speed::Speed::speed_2},
            {"3", Speed::Speed::speed_3},
            {"4", Speed::Speed::speed_4},
            {"5", Speed::Speed::speed_5},
            {"6", Speed::Speed::speed_6},
            {"7", Speed::Speed::speed_7},
        },
        .builder_animation_table={
            {"0", Speed::Speed::instant},
            {"1", Speed::Speed::speed_1},
            {"2", Speed::Speed::speed_2},
            {"3", Speed::Speed::speed_3},
            {"4", Speed::Speed::speed_4},
            {"5", Speed::Speed::speed_5},
            {"6", Speed::Speed::speed_6},
            {"7", Speed::Speed::speed_7},
        },
    };

    Maze_runner runner;
    const auto args = std::span(argv, static_cast<size_t>(argc));
    bool process_current = false;
    std::string_view prev_flag = {};
    // In the case of no arguments this is skipped and we use our sensible
    // defaults.
    for (size_t i = 1; i < args.size(); i++)
    {
        auto *arg = args[i];
        if (process_current)
        {
            set_relevant_arg(tables, runner, {.flag = prev_flag, .arg = arg});
            process_current = false;
        }
        else
        {
            const auto &found_arg = tables.argument_flags.find(arg);
            if (found_arg == tables.argument_flags.end())
            {
                std::cerr << "Invalid argument flag: " << arg << "\n";
                print_usage();
                std::exit(1);
            }
            if (*found_arg == "-h")
            {
                print_usage();
                return 0;
            }
            process_current = true;
            prev_flag = arg;
        }
    }

    Maze::Maze maze(runner.args);

    // Functions are stored in tuples so use tuple get syntax and then call them
    // immidiately.

    if (runner.builder_view == animated_playback)
    {
        std::get<animated_playback>(runner.builder)(maze, runner.builder_speed);
        if (runner.modder)
        {
            std::get<animated_playback>(runner.modder.value())(
                maze, runner.builder_speed);
        }
    }
    else
    {
        std::get<static_image>(runner.builder)(maze);
        if (runner.modder)
        {
            std::get<static_image>(runner.modder.value())(maze);
        }
    }

    // This helps ensure we have a smooth transition from build to solve with no
    // flashing from redrawing frame.
    Printer::set_cursor_position({.row = 0, .col = 0});

    if (runner.painter_view == animated_playback)
    {
        std::get<animated_playback>(runner.painter)(maze, runner.painter_speed);
    }
    else
    {
        std::get<static_image>(runner.painter)(maze);
    }
    return 0;
}

namespace {

void
set_relevant_arg(const Lookup_tables &tables, Maze_runner &runner,
                 const Flag_arg &pairs)
{
    const std::string arg_data{pairs.arg};
    if (pairs.flag == "-r")
    {
        set_rows(runner, pairs);
        return;
    }
    if (pairs.flag == "-c")
    {
        set_cols(runner, pairs);
        return;
    }
    if (pairs.flag == "-b")
    {
        const auto found = tables.builder_table.find(arg_data);
        if (found == tables.builder_table.end())
        {
            print_invalid_arg(pairs);
        }
        runner.builder = found->second;
        return;
    }
    if (pairs.flag == "-m")
    {
        const auto found = tables.modification_table.find(arg_data);
        if (found == tables.modification_table.end())
        {
            print_invalid_arg(pairs);
        }
        runner.modder = found->second;
        return;
    }
    if (pairs.flag == "-p")
    {
        const auto found = tables.painter_table.find(arg_data);
        if (found == tables.painter_table.end())
        {
            print_invalid_arg(pairs);
        }
        runner.painter = found->second;
        return;
    }
    if (pairs.flag == "-d")
    {
        const auto found = tables.style_table.find(arg_data);
        if (found == tables.style_table.end())
        {
            print_invalid_arg(pairs);
        }
        runner.args.style = found->second;
        return;
    }
    if (pairs.flag == "-pa")
    {
        const auto found = tables.painter_animation_table.find(arg_data);
        if (found == tables.painter_animation_table.end())
        {
            print_invalid_arg(pairs);
        }
        runner.painter_speed = found->second;
        runner.painter_view = animated_playback;
        return;
    }
    if (pairs.flag == "-ba")
    {
        const auto found = tables.builder_animation_table.find(arg_data);
        if (found == tables.builder_animation_table.end())
        {
            print_invalid_arg(pairs);
        }
        runner.builder_speed = found->second;
        runner.builder_view = animated_playback;
        runner.modification_getter = animated_playback;
        return;
    }
    print_invalid_arg(pairs);
}

void
set_rows(Maze_runner &runner, const Flag_arg &pairs)
{
    runner.args.odd_rows = std::stoi(std::string{pairs.arg});
    if (runner.args.odd_rows % 2 == 0)
    {
        runner.args.odd_rows++;
    }
    if (runner.args.odd_rows < 7)
    {
        print_invalid_arg(pairs);
    }
}
void
set_cols(Maze_runner &runner, const Flag_arg &pairs)
{
    runner.args.odd_cols = std::stoi(std::string{pairs.arg});
    if (runner.args.odd_cols % 2 == 0)
    {
        runner.args.odd_cols++;
    }
    if (runner.args.odd_cols < 7)
    {
        print_invalid_arg(pairs);
    }
}

void
print_invalid_arg(const Flag_arg &pairs)
{
    std::cerr << "Flag was: " << pairs.flag << "\n";
    std::cerr << "Invalid argument: " << pairs.arg << "\n";
    print_usage();
    std::exit(1);
}

void
print_usage()
{
    constexpr auto msg =
        R"(
    ┌───┬─────────┬─────┬───┬───────────┬─────┬───────┬─────────────┬─────┐
    │   │         │     │   │           │     │       │             │     │
    │ ╷ ╵ ┌───┐ ╷ └─╴ ╷ │ ╷ │ ┌─╴ ┌─┬─╴ │ ╶─┐ ╵ ┌───╴ │ ╶───┬─┐ ╶─┬─┘ ╶─┐ │
    │ │   │   │ │     │ │ │ │ │   │ │   │   │   │     │     │ │   │     │ │
    │ └───┤ ┌─┘ ├─────┘ └─┤ ╵ │ ┌─┘ ╵ ┌─┴─┐ └───┤ ╶───┴───┐ │ └─┐ ╵ ┌─┬─┘ │
    │     │ │   │      Thread Maze Usage Instructions     │ │   │   │ │   │
    ├───┐ ╵ │ -Use flags, followed by arguments, in any order:╷ └─┬─┘ │ ╷ │
    │   │   │ -r Rows flag. Set rows for the maze.    │   │ │ │   │   │ │ │
    │ ╶─┴─┐ └─┐ Any number > 7. Zoom out for larger mazes!╵ ╵ ├─┐ │ ╶─┤ └─┤
    │     │   -c Columns flag. Set columns for the maze.│     │ │ │   │   │
    │ ┌─┐ └─┐ │ Any number > 7. Zoom out for larger mazes!────┤ │ │ ╷ └─┐ │
    │ │ │   │ -b Builder flag. Set maze building algorithm.   │ │ │ │   │ │
    │ │ └─┐ ╵ │ rdfs - Randomized Depth First Search.         │ │ └─┘ ┌─┘ │
    │     │   │ kruskal - Randomized Kruskal's algorithm. │   │       │   │
    ├─────┤ ╷ ╵ prim - Randomized Prim's algorithm.─┴───┐ │ ┌─┴─────┬─┴─┐ │
    │     │ │   eller - Randomized Eller's algorithm.   │ │ │       │   │ │
    │     │ │   wilson - Loop-Erased Random Path Carver.│ │ │       │   │ │
    │ ┌─┐ ╵ ├─┬─wilson-walls - Loop-Erased Random Wall Adder. ┌───┐ ╵ ╷ │ │
    │ │ │   │ │ fractal - Randomized recursive subdivision. │ │   │   │ │ │
    │ ╵ ├───┘ ╵ grid - A random grid pattern. ├─┐ │ ┌─────┤ ╵ │ ┌─┴───┤ ╵ │
    │   │       arena - Open floor with no walls. │ │     │   │ │     │   │
    ├─╴ ├─────-m Modification flag. Add shortcuts to the maze.┘ │ ┌─┐ └─╴ │
    │   │     │ cross - Add crossroads through the center.      │ │ │     │
    │ ┌─┘ ┌─┐ │ x - Add an x of crossing paths through center.──┘ │ └─────┤
    │ │   │ │ -s painter flag. Choose the painter measurement.    │       │
    ├─┐ │ ┌─┐ └─distance - distance from the center     ├─╴ │ └─┐ ├───╴ │ │
    │ │ │ │ │   runs - bias of straight line runs       │   │   │ │     │ │
    │ │ │ ╵ └─-d Draw flag. Set the line style for the maze.┴─┐ └─┘ ┌─┬─┘ │
    │ │ │       sharp - The default straight lines. │   │     │     │ │   │
    │ │ └─┬───╴ round - Rounded corners.──╴ │ ╷ ╵ ╵ │ ╶─┴─┐ ╶─┴─────┘ │ ╶─┤
    │ │   │     doubles - Sharp double lines. │     │     │           │   │
    │ └─┐ └───┬─bold - Thicker straight lines.└─┬───┴─┬─╴ │ ┌───┬───╴ └─┐ │
    │   │     │ contrast - Full block width and height walls.   │       │ │
    │ ╷ ├─┬─╴ │ spikes - Connected lines with spikes. ╵ ┌─┘ ╵ ┌─┘ ┌─┐ ┌─┘ │
    │ │ │ │   -pa Painter Animation flag. Watch the maze solution.│ │ │   │
    │ │ ╵ │ ╶─┤ Any number 1-7. Speed increases with number.┌─┘ ┌─┤ ╵ │ ╶─┤
    │ │   │   -ba Builder Animation flag. Watch the maze build. │ │   │   │
    │ ├─╴ ├─┐ └─Any number 1-7. Speed increases with number.┘ ┌─┘ │ ┌─┴─┐ │
    │ │   │ │ -h Help flag. Make this prompt appear.  │   │   │   │ │   │ │
    │ └─┐ ╵ └─┐ No arguments.─┘ ┌───┐ └─┐ ├─╴ │ ╵ └───┤ ┌─┘ ┌─┴─╴ │ ├─╴ │ │
    │   │     -If any flags are omitted, defaults are used. │     │ │   │ │
    │   │             │       │       │           │   │     │           │ │
    └───┴─────────────┴───────┴───────┴───────────┴───┴─────┴───────────┴─┘)";
    std::cout << msg << "\n";
}

} // namespace
