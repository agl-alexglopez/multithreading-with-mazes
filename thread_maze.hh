#include <cstdio>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <string_view>
#include <stack>
#include <queue>
#include <functional>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>


class Thread_maze {

public:

    enum class Builder_algorithm {
        randomized_depth_first,
    };

    enum class Solver_algorithm {
        depth_first_search,
        breadth_first_search
    };

    enum class Square {
        zero_thread=0,
        one_thread,
        two_thread,
        three_thread,
        overlap_threads,
        square,
        wall,
        start,
        finish,
    };

    struct Point {
        int row;
        int col;
    };

    Thread_maze(size_t odd_rows, size_t odd_cols);

    void clear_paths();
    void new_maze();
    void new_maze(size_t odd_rows, size_t odd_cols);
    void solve_with_dfs_threads();
    void solve_with_bfs_threads();
    size_t size() const;
    std::vector<Square>& operator[](size_t index);
    const std::vector<Square>& operator[](size_t index) const;

private:

    static const int num_threads_;
    static const int starting_path_len_;
    static const std::vector<std::string_view> thread_colors_;
    static const std::vector<char> thread_chars_;
    static const std::vector<Point> cardinal_directions_;


    std::mt19937 generator_;
    std::vector<std::vector<Square>> maze_;
    std::vector<std::vector<Point>> thread_paths_;
    Point start_;
    Point finish_;
    int escape_path_index_;
    std::mutex escape_section_;
    void generate_randomized_dfs_maze(size_t odd_rows, size_t odd_cols);
    void dfs_stack_generator();
    bool dfs_thread_search(Point prev, Point start, Square thread_index);
    bool bfs_thread_search(Thread_maze& maze, Point prev, Point start, Square thread_index);
    void rebuild_path(const std::unordered_map<Point,Point>& parent_map,
                      Point cur, int safe_thread_index_cast, Square thread_index);
    bool is_valid_point(Point to_check);
    Point pick_random_point();
    Point pick_random_point(std::uniform_int_distribution<int>& row,
                            std::uniform_int_distribution<int>& col);
    void print_solution_path();
    void print_maze() const;
    void set_squares(const std::vector<std::vector<Square>>& tiles);

}; // class Thread_maze

bool operator==(const Thread_maze::Point& lhs, const Thread_maze::Point& rhs);

namespace std {
template<>
struct hash<Thread_maze::Point> {
    inline size_t operator()(const Thread_maze::Point& p) const {
        std::hash<int>hasher;
        return hasher(p.row) ^ hasher(p.col);
    }
};
} // namespace std

