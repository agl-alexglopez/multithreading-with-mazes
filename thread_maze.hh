#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <random>
#include <vector>
#include <array>


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

    struct Packaged_args {
        size_t odd_rows = 31;
        size_t odd_cols = 111;
        Builder_algorithm builder = Builder_algorithm::randomized_depth_first;
        Solver_algorithm solver = Solver_algorithm::depth_first_search;
    };

    struct Point {
        int row;
        int col;
    };

    Thread_maze(const Packaged_args& args);

    void solve_maze(Solver_algorithm solver);
    void solve_maze();

    void clear_paths();
    void new_maze();
    void new_maze(Builder_algorithm builder, size_t odd_rows, size_t odd_cols);
    size_t size() const;
    std::vector<Square>& operator[](size_t index);
    const std::vector<Square>& operator[](size_t index) const;

private:

    static constexpr int num_threads_ = 4;
    static constexpr int starting_path_len_ = 4096;
    static constexpr const char *const ansi_red = "\033[31;1m";
    static constexpr const char *const ansi_grn = "\033[32;1m";
    static constexpr const char *const ansi_yel = "\033[93;1m";
    static constexpr const char *const ansi_blu = "\033[34;1m";
    static constexpr const char *const ansi_cyn = "\033[36;1m";
    static constexpr const char *const ansi_wit = "\033[255;1m";
    static constexpr const char *const ansi_nil = "\033[0m";
    static constexpr std::array<const char *const, 5> thread_colors_ = {
        ansi_red, ansi_grn, ansi_blu, ansi_yel, ansi_wit
    };
    static constexpr std::array<char,5> thread_chars_ = {'x', '^', '+', '*','@'};
    //                                                               n      e     s      w
    static constexpr std::array<Point,4> cardinal_directions_ = { {{-1,0},{0,1},{1,0},{0,-1}} };
    static constexpr std::array<Point,4> generate_directions_ = { {{-2,0},{0,2},{2,0},{0,-2}} };
    static constexpr std::array<Point,7> all_directions_ = {
        { {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1} }
    };


    Builder_algorithm builder_;
    Solver_algorithm solver_;
    std::mt19937 generator_;
    std::vector<std::vector<Square>> maze_;
    std::vector<std::vector<Point>> thread_paths_;
    Point start_;
    Point finish_;
    int escape_path_index_;
    std::mutex escape_section_;
    void generate_maze(Builder_algorithm algorithm, size_t odd_rows, size_t odd_cols);
    void generate_randomized_dfs_maze(size_t odd_rows, size_t odd_cols);
    void solve_with_dfs_threads();
    void solve_with_bfs_threads();
    bool dfs_thread_search(Point prev, Point start, Square thread_index);
    bool bfs_thread_search(Point prev, Point start, Square thread_index);
    void rebuild_path(const std::unordered_map<Point,Point>& parent_map,
                      Point cur, int safe_thread_index_cast, Square thread_index);
    bool is_valid_point(Point to_check);
    Point pick_random_point(std::uniform_int_distribution<int>& row,
                            std::uniform_int_distribution<int>& col);
    Point adjust_point(Point choice);
    void print_solution_path();
    void print_maze() const;
    void set_squares(const std::vector<std::vector<Square>>& tiles);

}; // class Thread_maze

bool operator==(const Thread_maze::Point& lhs, const Thread_maze::Point& rhs);
bool operator!=(const Thread_maze::Point& lhs, const Thread_maze::Point& rhs);

// Points should be hashable for ease of use in most containers.
namespace std {
template<>
struct hash<Thread_maze::Point> {
    inline size_t operator()(const Thread_maze::Point& p) const {
        std::hash<int>hasher;
        return hasher(p.row) ^ hasher(p.col);
    }
};
} // namespace std

