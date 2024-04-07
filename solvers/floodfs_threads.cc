module;
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <vector>
export module labyrinth:floodfs;
import :maze;
import :speed;
import :printers;
import :solve_utilities;
import :my_queue;

//////////////////////////////////   Exported Interface

export namespace Floodfs {
void hunt(Maze::Maze &maze);
void animate_hunt(Maze::Maze &maze, Speed::Speed speed);
void gather(Maze::Maze &maze);
void animate_gather(Maze::Maze &maze, Speed::Speed speed);
void corners(Maze::Maze &maze);
void animate_corners(Maze::Maze &maze, Speed::Speed speed);
} // namespace Floodfs

//////////////////////////////////   Implementation

namespace {

void
hunter(Maze::Maze &maze, Sutil::Dfs_monitor &monitor, Sutil::Thread_id id)
{
    // We have useful bits in a square. Each square can use a unique bit to
    // track seen threads. Each thread could maintain its own hashset, but this
    // is much more space efficient. Use the space the maze already occupies and
    // provides.
    const Sutil::Thread_cache seen(id.bit << Sutil::thread_cache_shift);
    const Sutil::Thread_paint paint_bit(id.bit << Sutil::thread_paint_shift);
    // Each thread only needs enough space for an O(current path length) stack.
    std::vector<Maze::Point> &dfs = monitor.thread_paths[id.index];
    dfs.push_back(monitor.starts.at(id.index));
    Maze::Point cur = monitor.starts.at(id.index);
    while (!dfs.empty())
    {
        // Lock? Garbage read stolen mid write by winning thread is still ok for
        // program logic.
        if (monitor.winning_index.load() != Sutil::no_winner)
        {
            break;
        }

        // Don't pop() yet!
        cur = dfs.back();

        if (maze[cur.row][cur.col] & Sutil::finish_bit)
        {
            static_cast<void>(
                monitor.winning_index.ces(Sutil::no_winner, id.index));
            dfs.pop_back();
            break;
        }
        maze[cur.row][cur.col] |= (seen | paint_bit);

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
            dfs.pop_back();
        }
    }
}

void
animate_hunter(Maze::Maze &maze, Sutil::Dfs_monitor &monitor,
               Sutil::Thread_id id)
{
    const Sutil::Thread_cache seen(id.bit << Sutil::thread_cache_shift);
    const Sutil::Thread_paint paint_bit(id.bit << Sutil::thread_paint_shift);
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
            static_cast<void>(
                monitor.winning_index.ces(Sutil::no_winner, id.index));
            dfs.pop_back();
            return;
        }
        maze[cur.row][cur.col] |= (seen | paint_bit);
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
            dfs.pop_back();
        }
    }
}

void
gatherer(Maze::Maze &maze, Sutil::Dfs_monitor &monitor, Sutil::Thread_id id)
{
    const Sutil::Thread_cache seen(id.bit << Sutil::thread_cache_shift);
    const Sutil::Thread_paint paint_bit(id.bit << Sutil::thread_paint_shift);
    std::vector<Maze::Point> &dfs = monitor.thread_paths[id.index];
    dfs.push_back(monitor.starts.at(id.index));
    Maze::Point cur = monitor.starts.at(id.index);
    while (!dfs.empty())
    {
        cur = dfs.back();

        // We are the first thread to this finish! Claim it!
        if (maze[cur.row][cur.col] & Sutil::finish_bit
            && !(maze[cur.row][cur.col] & Sutil::cache_mask))
        {
            maze[cur.row][cur.col] |= seen;
            dfs.pop_back();
            for (const Maze::Point &p : dfs)
            {
                maze[p.row][p.col] |= paint_bit;
            }
            return;
        }
        maze[cur.row][cur.col] |= (seen | paint_bit);

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
            dfs.pop_back();
        }
    }
}

void
animate_gatherer(Maze::Maze &maze, Sutil::Dfs_monitor &monitor,
                 Sutil::Thread_id id)
{
    const Sutil::Thread_cache seen(id.bit << Sutil::thread_cache_shift);
    const Sutil::Thread_paint paint_bit(id.bit << Sutil::thread_paint_shift);
    std::vector<Maze::Point> &dfs = monitor.thread_paths.at(id.index);
    dfs.push_back(monitor.starts.at(id.index));
    Maze::Point cur = monitor.starts.at(id.index);
    while (!dfs.empty())
    {
        cur = dfs.back();

        if (maze[cur.row][cur.col] & Sutil::finish_bit
            && !(maze[cur.row][cur.col] & Sutil::cache_mask))
        {
            maze[cur.row][cur.col] |= seen;
            dfs.pop_back();
            return;
        }

        maze[cur.row][cur.col] |= (seen | paint_bit);
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
            dfs.pop_back();
        }
    }
}

} // namespace

///////////  Multithreaded Dispatcher Functions from Header Interface

namespace Floodfs {

void
hunt(Maze::Maze &maze)
{
    Sutil::Dfs_monitor monitor;
    monitor.starts = std::vector<Maze::Point>(Sutil::num_threads,
                                              Sutil::pick_random_point(maze));
    maze[monitor.starts.at(0).row][monitor.starts.at(0).col]
        |= Sutil::start_bit;
    const Maze::Point finish = Sutil::pick_random_point(maze);
    maze[finish.row][finish.col] |= Sutil::finish_bit;
    std::vector<std::thread> threads(Sutil::num_threads);
    for (uint16_t i_thread = 0; i_thread < Sutil::num_threads; i_thread++)
    {
        const Sutil::Thread_id this_thread{i_thread,
                                           Sutil::thread_bits.at(i_thread)};
        threads[i_thread] = std::thread(hunter, std::ref(maze),
                                        std::ref(monitor), this_thread);
    }

    for (std::thread &t : threads)
    {
        t.join();
    }

    if (monitor.winning_index.load() != Sutil::no_winner)
    {
        const Sutil::Thread_paint winner_color(
            Sutil::thread_bits.at(monitor.winning_index.load())
            << Sutil::thread_paint_shift);
        monitor.thread_paths.at(monitor.winning_index.load()).pop_back();
        const Maze::Point &before_finish
            = monitor.thread_paths.at(monitor.winning_index.load()).back();
        maze[before_finish.row][before_finish.col] |= winner_color;
    }

    Sutil::print_maze(maze);
    Sutil::print_overlap_key();
    Sutil::print_hunt_solution_message(monitor.winning_index.load());
    std::cout << "\n";
}

void
gather(Maze::Maze &maze)
{
    Sutil::Dfs_monitor monitor;
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
        const Sutil::Thread_id this_thread{i_thread,
                                           Sutil::thread_bits.at(i_thread)};
        threads[i_thread] = std::thread(gatherer, std::ref(maze),
                                        std::ref(monitor), this_thread);
    }

    for (std::thread &t : threads)
    {
        t.join();
    }
    uint16_t i_thread = 0;
    for (const std::vector<Maze::Point> &path : monitor.thread_paths)
    {
        const Sutil::Thread_paint color(Sutil::thread_bits.at(i_thread)
                                        << Sutil::thread_paint_shift);
        const Maze::Point &p = path.back();
        maze[p.row][p.col] &= ~Sutil::thread_paint_mask;
        maze[p.row][p.col] |= color;
        ++i_thread;
    }
    Sutil::print_maze(maze);
    Sutil::print_overlap_key();
    Sutil::print_gather_solution_message();
    std::cout << "\n";
}

void
corners(Maze::Maze &maze)
{
    Sutil::Dfs_monitor monitor;
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
        const Sutil::Thread_id this_thread
            = {i_thread, Sutil::thread_bits.at(i_thread)};
        threads[i_thread] = std::thread(hunter, std::ref(maze),
                                        std::ref(monitor), this_thread);
    }
    for (std::thread &t : threads)
    {
        t.join();
    }

    if (monitor.winning_index.load() != Sutil::no_winner)
    {
        const Sutil::Thread_paint winner_color(
            Sutil::thread_bits.at(monitor.winning_index.load())
            << Sutil::thread_paint_shift);
        const Maze::Point &before_finish
            = monitor.thread_paths.at(monitor.winning_index.load()).back();
        maze[before_finish.row][before_finish.col] |= winner_color;
    }

    Sutil::print_maze(maze);
    Sutil::print_overlap_key();
    Sutil::print_hunt_solution_message(monitor.winning_index.load());
    std::cout << "\n";
}

void
animate_hunt(Maze::Maze &maze, Speed::Speed speed)
{
    Printer::set_cursor_position({maze.row_size(), 0});
    Sutil::print_overlap_key();
    Sutil::Dfs_monitor monitor;
    monitor.speed = Sutil::solver_speeds.at(static_cast<int>(speed));
    monitor.starts = std::vector<Maze::Point>(Sutil::num_threads,
                                              Sutil::pick_random_point(maze));
    maze[monitor.starts.at(0).row][monitor.starts.at(0).col]
        |= Sutil::start_bit;
    const Maze::Point finish = Sutil::pick_random_point(maze);
    maze[finish.row][finish.col] |= Sutil::finish_bit;
    Sutil::flush_cursor_path_coordinate(maze, finish);
    std::this_thread::sleep_for(
        std::chrono::microseconds(monitor.speed.value_or(0)));

    std::vector<std::thread> threads(Sutil::num_threads);
    for (uint16_t i_thread = 0; i_thread < Sutil::num_threads; i_thread++)
    {
        const Sutil::Thread_id this_thread{i_thread,
                                           Sutil::thread_bits.at(i_thread)};
        threads[i_thread] = std::thread(animate_hunter, std::ref(maze),
                                        std::ref(monitor), this_thread);
    }

    for (std::thread &t : threads)
    {
        t.join();
    }

    if (monitor.winning_index.load() != Sutil::no_winner)
    {
        const Sutil::Thread_paint winner_color(
            Sutil::thread_bits.at(monitor.winning_index.load())
            << Sutil::thread_paint_shift);
        const Maze::Point &before_finish
            = monitor.thread_paths.at(monitor.winning_index.load()).back();
        maze[before_finish.row][before_finish.col] |= winner_color;
        Sutil::flush_cursor_path_coordinate(maze, before_finish);
        std::this_thread::sleep_for(
            std::chrono::microseconds(monitor.speed.value_or(0)));
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
        Sutil::flush_cursor_path_coordinate(maze, finish);
        std::this_thread::sleep_for(
            std::chrono::microseconds(monitor.speed.value_or(0)));
    }

    std::vector<std::thread> threads(Sutil::num_threads);
    for (uint16_t i_thread = 0; i_thread < Sutil::num_threads; i_thread++)
    {
        const Sutil::Thread_id this_thread{i_thread,
                                           Sutil::thread_bits.at(i_thread)};
        threads[i_thread] = std::thread(animate_gatherer, std::ref(maze),
                                        std::ref(monitor), this_thread);
    }

    for (std::thread &t : threads)
    {
        t.join();
    }

    uint16_t i_thread = 0;
    for (const std::vector<Maze::Point> &path : monitor.thread_paths)
    {
        const Sutil::Thread_paint color(Sutil::thread_bits.at(i_thread)
                                        << Sutil::thread_paint_shift);
        const Maze::Point &p = path.back();
        maze[p.row][p.col] &= ~Sutil::thread_paint_mask;
        maze[p.row][p.col] |= color;
        ++i_thread;
        Sutil::flush_cursor_path_coordinate(maze, p);
        std::this_thread::sleep_for(
            std::chrono::microseconds(monitor.speed.value_or(0)));
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
    Sutil::Dfs_monitor monitor;
    monitor.speed = Sutil::solver_speeds.at(static_cast<int>(speed));
    monitor.starts = Sutil::set_corner_starts(maze);
    for (const Maze::Point &p : monitor.starts)
    {
        maze[p.row][p.col] |= Sutil::start_bit;
        Sutil::flush_cursor_path_coordinate(maze, p);
        std::this_thread::sleep_for(
            std::chrono::microseconds(monitor.speed.value_or(0)));
    }
    const Maze::Point finish = {maze.row_size() / 2, maze.col_size() / 2};
    for (const Maze::Point &p : Sutil::all_dirs)
    {
        const Maze::Point next = {finish.row + p.row, finish.col + p.col};
        maze[next.row][next.col] |= Maze::path_bit;
        Sutil::flush_cursor_path_coordinate(maze, next);
        std::this_thread::sleep_for(
            std::chrono::microseconds(monitor.speed.value_or(0)));
    }
    maze[finish.row][finish.col] |= Maze::path_bit;
    maze[finish.row][finish.col] |= Sutil::finish_bit;
    Sutil::flush_cursor_path_coordinate(maze, finish);
    std::this_thread::sleep_for(
        std::chrono::microseconds(monitor.speed.value_or(0)));

    std::vector<std::thread> threads(Sutil::num_threads);
    // Randomly shuffle thread start corners so colors mix differently each
    // time.
    shuffle(begin(monitor.starts), end(monitor.starts),
            std::mt19937(std::random_device{}()));
    for (uint16_t i_thread = 0; i_thread < Sutil::num_threads; i_thread++)
    {
        const Sutil::Thread_id this_thread
            = {i_thread, Sutil::thread_bits.at(i_thread)};
        threads[i_thread] = std::thread(animate_hunter, std::ref(maze),
                                        std::ref(monitor), this_thread);
    }
    for (std::thread &t : threads)
    {
        t.join();
    }

    if (monitor.winning_index.load() != Sutil::no_winner)
    {
        const Sutil::Thread_paint winner_color(
            Sutil::thread_bits.at(monitor.winning_index.load())
            << Sutil::thread_paint_shift);
        const Maze::Point &before_finish
            = monitor.thread_paths.at(monitor.winning_index.load()).back();
        maze[before_finish.row][before_finish.col] |= winner_color;
        Sutil::flush_cursor_path_coordinate(maze, before_finish);
        std::this_thread::sleep_for(
            std::chrono::microseconds(monitor.speed.value_or(0)));
    }

    Printer::set_cursor_position(
        {maze.row_size() + Sutil::overlap_key_and_message_height, 0});
    Sutil::print_hunt_solution_message(monitor.winning_index.load());
    std::cout << "\n";
}

} // namespace Floodfs
