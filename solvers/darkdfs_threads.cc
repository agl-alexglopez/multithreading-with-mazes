module;
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <unordered_set>
#include <vector>
export module labyrinth:dark_dfs;
import :maze;
import :speed;
import :printers;
import :solve_utilities;
import :my_queue;

//////////////////////////////////   Exported Interface

export namespace Dark_dfs {
void animate_hunt(Maze::Maze &maze, Speed::Speed speed);
void animate_gather(Maze::Maze &maze, Speed::Speed speed);
void animate_corners(Maze::Maze &maze, Speed::Speed speed);
} // namespace Dark_dfs

//////////////////////////////////   Implementation

namespace {

struct Thread_light
{
    uint16_t index;
    uint16_t bit;
};

void
animate_hunter(Maze::Maze &maze, Sutil::Dfs_monitor &monitor, Thread_light id)
{
    const Sutil::Thread_cache seen(id.bit << Sutil::thread_cache_shift);
    const Sutil::Thread_paint paint(id.bit << Sutil::thread_paint_shift);
    std::vector<Maze::Point> &dfs = monitor.thread_paths.at(id.index);
    dfs.push_back(monitor.starts.at(id.index));
    Maze::Point cur = monitor.starts.at(id.index);
    while (!dfs.empty())
    {
        // Lock? Garbage read stolen mid write by winning thread is still ok for
        // program logic.
        if (monitor.winning_index.load() != Sutil::no_winner)
        {
            return;
        }

        // Don't pop() yet!
        cur = dfs.back();

        if (maze[cur.row][cur.col] & Sutil::finish_bit)
        {
            if (monitor.winning_index.ces(Sutil::no_winner, id.index))
            {
                monitor.monitor.lock();
                Sutil::flush_cursor_path_coordinate(maze, cur);
                monitor.monitor.unlock();
            }
            dfs.pop_back();
            return;
        }
        maze[cur.row][cur.col] |= (paint | seen);

        monitor.monitor.lock();
        Sutil::flush_cursor_path_coordinate(maze, cur);
        monitor.monitor.unlock();
        std::this_thread::sleep_for(
            std::chrono::microseconds(monitor.speed.value_or(0)));

        // Bias each thread's first choice towards orginal dispatch direction.
        // More coverage.
        bool found_branch_to_explore = false;
        for (uint64_t count = 0, i = id.index; count < Sutil::dirs.size();
             count++, ++i %= Sutil::dirs.size())
        {
            const Maze::Point &p = Sutil::dirs.at(i);
            const Maze::Point next = {cur.row + p.row, cur.col + p.col};
            const bool push_next
                = !(maze[next.row][next.col] & seen)
                  && (maze[next.row][next.col] & Maze::path_bit);
            if (push_next)
            {
                found_branch_to_explore = true;
                dfs.push_back(next);
                break;
            }
        }

        if (!found_branch_to_explore)
        {
            maze[cur.row][cur.col] &= ~paint;
            monitor.monitor.lock();
            Sutil::flush_cursor_path_coordinate(maze, cur);
            monitor.monitor.unlock();
            std::this_thread::sleep_for(
                std::chrono::microseconds(monitor.speed.value_or(0)));
            dfs.pop_back();
        }
    }
}

void
animate_gatherer(Maze::Maze &maze, Sutil::Dfs_monitor &monitor, Thread_light id)
{
    const Sutil::Thread_cache seen(id.bit << Sutil::thread_cache_shift);
    const Sutil::Thread_paint paint(id.bit << Sutil::thread_paint_shift);
    std::vector<Maze::Point> &dfs = monitor.thread_paths.at(id.index);
    dfs.push_back(monitor.starts.at(id.index));
    Maze::Point cur = monitor.starts.at(id.index);
    while (!dfs.empty())
    {
        cur = dfs.back();

        if ((maze[cur.row][cur.col] & Sutil::finish_bit)
            && !(maze[cur.row][cur.col] & Sutil::cache_mask))
        {
            maze[cur.row][cur.col] |= seen;
            monitor.monitor.lock();
            Sutil::flush_cursor_path_coordinate(maze, cur);
            monitor.monitor.unlock();
            dfs.pop_back();
            return;
        }
        maze[cur.row][cur.col] |= (seen | paint);

        monitor.monitor.lock();
        Sutil::flush_cursor_path_coordinate(maze, cur);
        monitor.monitor.unlock();
        std::this_thread::sleep_for(
            std::chrono::microseconds(monitor.speed.value_or(0)));

        bool found_branch_to_explore = false;
        for (uint64_t count = 0, i = id.index; count < Sutil::dirs.size();
             count++, ++i %= Sutil::dirs.size())
        {
            const Maze::Point &p = Sutil::dirs.at(i);
            const Maze::Point next = {cur.row + p.row, cur.col + p.col};
            const bool push_next
                = !(maze[next.row][next.col] & seen)
                  && (maze[next.row][next.col] & Maze::path_bit);
            if (push_next)
            {
                found_branch_to_explore = true;
                dfs.push_back(next);
                break;
            }
        }
        if (!found_branch_to_explore)
        {
            maze[cur.row][cur.col] &= ~paint;
            monitor.monitor.lock();
            Sutil::flush_cursor_path_coordinate(maze, cur);
            monitor.monitor.unlock();
            std::this_thread::sleep_for(
                std::chrono::microseconds(monitor.speed.value_or(0)));
            dfs.pop_back();
        }
    }
}

} // namespace

namespace Dark_dfs {

void
animate_hunt(Maze::Maze &maze, Speed::Speed speed)
{
    Printer::set_cursor_position({maze.row_size(), 0});
    Sutil::print_overlap_key();
    Sutil::deluminate_maze(maze);
    Printer::set_cursor_position({0, 0});
    Sutil::Dfs_monitor monitor;
    monitor.speed = Sutil::solver_speeds.at(static_cast<int>(speed));
    monitor.starts = std::vector<Maze::Point>(Sutil::num_threads,
                                              Sutil::pick_random_point(maze));
    maze[monitor.starts.at(0).row][monitor.starts.at(0).col]
        |= Sutil::start_bit;
    const Maze::Point finish = Sutil::pick_random_point(maze);
    maze[finish.row][finish.col] |= Sutil::finish_bit;

    std::array<std::thread, Sutil::num_threads> threads;
    for (uint16_t i_thread = 0; i_thread < Sutil::num_threads; i_thread++)
    {
        const Thread_light this_thread{i_thread,
                                       Sutil::thread_bits.at(i_thread)};
        threads.at(i_thread) = std::thread(animate_hunter, std::ref(maze),
                                           std::ref(monitor), this_thread);
    }

    for (std::thread &t : threads)
    {
        t.join();
    }
    Printer::set_cursor_position(
        {maze.row_size() + Sutil::overlap_key_and_message_height, 0});
    Sutil::print_hunt_solution_message(monitor.winning_index.load());
    std::cout << "\n";
}

void
animate_gather(Maze::Maze &maze, Speed::Speed speed)
{
    Printer::set_cursor_position({maze.row_size(), 0});
    Sutil::print_overlap_key();
    Sutil::deluminate_maze(maze);
    Sutil::Dfs_monitor monitor;
    monitor.speed = Sutil::solver_speeds.at(static_cast<int>(speed));
    monitor.starts = std::vector<Maze::Point>(Sutil::num_threads,
                                              Sutil::pick_random_point(maze));
    maze[monitor.starts.at(0).row][monitor.starts.at(0).col]
        |= Sutil::start_bit;
    for (int finish_square = 0; finish_square < Sutil::num_gather_finishes;
         finish_square++)
    {
        const Maze::Point finish = Sutil::pick_random_point(maze);
        maze[finish.row][finish.col] |= Sutil::finish_bit;
    }

    std::vector<std::thread> threads(Sutil::num_threads);
    for (uint16_t i_thread = 0; i_thread < Sutil::num_threads; i_thread++)
    {
        const Thread_light this_thread{i_thread,
                                       Sutil::thread_bits.at(i_thread)};
        threads.at(i_thread) = std::thread(animate_gatherer, std::ref(maze),
                                           std::ref(monitor), this_thread);
    }

    for (std::thread &t : threads)
    {
        t.join();
    }
    Printer::set_cursor_position(
        {maze.row_size() + Sutil::overlap_key_and_message_height, 0});
    Sutil::print_gather_solution_message();
    std::cout << "\n";
}

void
animate_corners(Maze::Maze &maze, Speed::Speed speed)
{
    Printer::set_cursor_position({maze.row_size(), 0});
    Sutil::print_overlap_key();
    Sutil::deluminate_maze(maze);
    Sutil::Dfs_monitor monitor;
    monitor.speed = Sutil::solver_speeds.at(static_cast<int>(speed));
    monitor.starts = Sutil::set_corner_starts(maze);
    for (const Maze::Point &p : monitor.starts)
    {
        maze[p.row][p.col] |= Sutil::start_bit;
    }
    const Maze::Point finish = {maze.row_size() / 2, maze.col_size() / 2};
    for (const Maze::Point &p : Sutil::all_dirs)
    {
        const Maze::Point next = {finish.row + p.row, finish.col + p.col};
        maze[next.row][next.col] |= Maze::path_bit;
    }
    maze[finish.row][finish.col] |= Maze::path_bit;
    maze[finish.row][finish.col] |= Sutil::finish_bit;

    std::vector<std::thread> threads(Sutil::num_threads);
    // Randomly shuffle thread start corners so colors mix differently each
    // time.
    shuffle(begin(monitor.starts), end(monitor.starts),
            std::mt19937(std::random_device{}()));
    for (uint16_t i_thread = 0; i_thread < Sutil::num_threads; i_thread++)
    {
        const Thread_light this_thread
            = {i_thread, Sutil::thread_bits.at(i_thread)};
        threads[i_thread] = std::thread(animate_hunter, std::ref(maze),
                                        std::ref(monitor), this_thread);
    }
    for (std::thread &t : threads)
    {
        t.join();
    }
    Printer::set_cursor_position(
        {maze.row_size() + Sutil::overlap_key_and_message_height, 0});
    Sutil::print_hunt_solution_message(monitor.winning_index.load());
    std::cout << "\n";
}

} // namespace Dark_dfs
