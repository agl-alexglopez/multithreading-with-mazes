module;
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
export module labyrinth:runs;
import :maze;
import :speed;
import :rgb;
import :my_queue;
import :printers;

/////////////////////////////////////   Exported Interface
////////////////////////////////////////

export namespace Runs {
void paint_runs(Maze::Maze &maze);
void animate_runs(Maze::Maze &maze, Speed::Speed speed);
} // namespace Runs

/////////////////////////////////////     Implementation
////////////////////////////////////////

namespace {

struct Run_map {
    uint32_t max;
    std::unordered_map<Maze::Point, uint32_t> runs;
    Run_map(Maze::Point p, uint32_t run) : max(run), runs({{p, run}}) {
    }
};

struct Run_point {
    uint32_t len;
    Maze::Point prev;
    Maze::Point cur;
};

void
painter(Maze::Maze &maze, Run_map const &map) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> uid(0, 2);
    int const rand_color_choice = uid(rng);
    for (int row = 0; row < maze.row_size(); row++) {
        for (int col = 0; col < maze.col_size(); col++) {
            Maze::Point const cur = {row, col};
            auto const path_point = map.runs.find(cur);
            if (path_point != map.runs.end()) {
                auto const intensity
                    = static_cast<double>(map.max - path_point->second)
                      / static_cast<double>(map.max);
                auto const dark = static_cast<uint8_t>(255.0 * intensity);
                auto const bright = static_cast<uint8_t>(128)
                                    + static_cast<uint8_t>(127.0 * intensity);
                Rgb::Rgb color{dark, dark, dark};
                color.at(rand_color_choice) = bright;
                Rgb::print_rgb(color, cur);
            } else {
                Rgb::print_wall(maze, cur);
            }
        }
    }
    std::cout << "\n";
}

void
painter_animated(Maze::Maze &maze, Run_map const &map,
                 Rgb::Bfs_monitor &monitor, Rgb::Thread_guide guide) {
    My_queue<Maze::Point> &bfs = monitor.paths[guide.bias];
    std::unordered_set<Maze::Point> &seen = monitor.seen[guide.bias];
    bfs.push(guide.p);
    while (!bfs.empty()) {
        Maze::Point const cur = bfs.front();
        bfs.pop();

        if (monitor.count.load() >= map.runs.size()) {
            return;
        }

        uint16_t const square = maze[cur.row][cur.col].load();
        uint16_t const painted = (square | Rgb::paint);
        uint16_t const not_painted
            = square & static_cast<uint16_t>(~Rgb::paint);
        if (maze[cur.row][cur.col].ces(not_painted, painted)) {
            uint64_t const dist = map.runs.at(cur);
            auto const intensity = static_cast<double>(map.max - dist)
                                   / static_cast<double>(map.max);
            auto const dark = static_cast<uint8_t>(255.0 * intensity);
            auto const bright = static_cast<uint8_t>(128)
                                + static_cast<uint8_t>(127.0 * intensity);
            Rgb::Rgb color{dark, dark, dark};
            color.at(guide.color_i) = bright;

            monitor.monitor.lock();
            Rgb::animate_rgb(color, cur);
            monitor.monitor.unlock();

            ++monitor.count;
            std::this_thread::sleep_for(
                std::chrono::microseconds(guide.animation));
        }

        for (uint64_t count = 0, i = guide.bias; count < Maze::dirs.size();
             ++count, ++i %= Maze::dirs.size()) {
            Maze::Point const &p = Maze::dirs.at(i);
            Maze::Point const next = {cur.row + p.row, cur.col + p.col};

            bool const is_path
                = (maze[next.row][next.col] & Maze::path_bit).load();

            if (is_path && !seen.contains(next)) {
                bfs.push(next);
                seen.insert(next);
            }
        }
    }
}

} // namespace

namespace Runs {

void
paint_runs(Maze::Maze &maze) {
    int const row_mid = maze.row_size() / 2;
    int const col_mid = maze.col_size() / 2;
    Maze::Point const start
        = {row_mid + 1 - (row_mid % 2), col_mid + 1 - (col_mid % 2)};
    Run_map map(start, 0);
    My_queue<Run_point> bfs;
    bfs.push({0, start, start});
    maze[start.row][start.col] |= Rgb::measure;
    while (!bfs.empty()) {
        Run_point const cur = bfs.front();
        bfs.pop();
        map.max = std::max(map.max, cur.len);
        for (Maze::Point const &p : Maze::dirs) {
            Maze::Point const next = {cur.cur.row + p.row, cur.cur.col + p.col};
            if (!(maze[next.row][next.col] & Maze::path_bit)
                || (maze[next.row][next.col] & Rgb::measure)) {
                continue;
            }
            uint32_t const len = std::abs(next.row - cur.prev.row)
                                         == std::abs(next.col - cur.prev.col)
                                     ? 1
                                     : cur.len + 1;
            maze[next.row][next.col] |= Rgb::measure;
            map.runs.insert({next, len});
            bfs.push({len, cur.cur, next});
        }
    }
    painter(maze, map);
    std::cout << "\n";
}

void
animate_runs(Maze::Maze &maze, Speed::Speed speed) {
    int const row_mid = maze.row_size() / 2;
    int const col_mid = maze.col_size() / 2;
    Maze::Point const start
        = {row_mid + 1 - (row_mid % 2), col_mid + 1 - (col_mid % 2)};
    Run_map map(start, 0);
    My_queue<Run_point> bfs;
    bfs.push({0, start, start});
    maze[start.row][start.col] |= Rgb::measure;
    while (!bfs.empty()) {
        Run_point const cur = bfs.front();
        bfs.pop();
        map.max = std::max(map.max, cur.len);
        for (Maze::Point const &p : Maze::dirs) {
            Maze::Point const next = {cur.cur.row + p.row, cur.cur.col + p.col};
            if (!(maze[next.row][next.col] & Maze::path_bit)
                || (maze[next.row][next.col] & Rgb::measure)) {
                continue;
            }
            uint32_t const len = std::abs(next.row - cur.prev.row)
                                         == std::abs(next.col - cur.prev.col)
                                     ? 1
                                     : cur.len + 1;
            maze[next.row][next.col] |= Rgb::measure;
            map.runs.insert({next, len});
            bfs.push({len, cur.cur, next});
        }
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> uid(0, 2);
    uint64_t const rand_color_choice = uid(rng);
    std::array<std::thread, Rgb::num_painters> handles;
    Speed::Speed_unit const animation
        = Rgb::animation_speeds.at(static_cast<uint64_t>(speed));
    Rgb::Bfs_monitor monitor;
    for (uint64_t i = 0; i < handles.size(); i++) {
        Rgb::Thread_guide const this_thread
            = {i, rand_color_choice, animation, start};
        handles.at(i)
            = std::thread(painter_animated, std::ref(maze), std::ref(map),
                          std::ref(monitor), this_thread);
    }

    for (std::thread &t : handles) {
        t.join();
    }
    Printer::set_cursor_position({maze.row_size(), maze.col_size()});
    std::cout << "\n";
}

} // namespace Runs
