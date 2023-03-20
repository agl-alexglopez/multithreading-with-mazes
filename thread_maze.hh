#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <vector>
#include <array>


class Thread_maze {

public:

    enum class Builder_algorithm {
        randomized_depth_first,
        randomized_loop_erased,
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
        path,
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

    typedef uint8_t Wall_line;
    static constexpr Wall_line default_wall_ = 0b0;
    static constexpr Wall_line north_wall_ = 0b1;
    static constexpr Wall_line east_wall_ = 0b10;
    static constexpr Wall_line south_wall_ = 0b100;
    static constexpr Wall_line west_wall_ = 0b1000;
    static constexpr Wall_line north_east_south_west_wall_ = north_wall_ | east_wall_ | south_wall_ | west_wall_;
    static constexpr Wall_line north_east_south_wall_ = north_wall_ | east_wall_ | south_wall_;
    static constexpr Wall_line north_west_south_wall_ = north_wall_ | west_wall_ | south_wall_;
    static constexpr Wall_line south_west_east_wall_ = south_wall_ | west_wall_ | east_wall_;
    static constexpr Wall_line north_west_east_wall_ = north_wall_ | west_wall_ | east_wall_;
    static constexpr Wall_line north_east_wall_ = north_wall_ | east_wall_;
    static constexpr Wall_line north_south_wall_ = north_wall_ | south_wall_;
    static constexpr Wall_line north_west_wall_ = north_wall_ | west_wall_;
    static constexpr Wall_line south_east_wall_ = south_wall_ | east_wall_;
    static constexpr Wall_line south_west_wall_ = south_wall_ | west_wall_;
    static constexpr Wall_line east_west_wall_ = east_wall_ | west_wall_;
    static constexpr int num_threads_ = 4;
    static constexpr int starting_path_len_ = 4096;
    static constexpr const char *const ansi_red_ = "\033[31;1m";
    static constexpr const char *const ansi_grn_ = "\033[32;1m";
    static constexpr const char *const ansi_yel_ = "\033[93;1m";
    static constexpr const char *const ansi_blu_ = "\033[34;1m";
    static constexpr const char *const ansi_cyn_ = "\033[36;1m";
    static constexpr const char *const ansi_wit_ = "\033[255;1m";
    static constexpr const char *const ansi_nil_ = "\033[0m";

    static const std::unordered_map<Wall_line,std::string> wall_lines_;
    static constexpr std::array<const char *const, 5> thread_colors_ = {
        ansi_red_, ansi_grn_, ansi_blu_, ansi_yel_, ansi_wit_
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
    void generate_randomized_loop_erased_maze(size_t odd_rows, size_t odd_cols);
    Point choose_arbitrary_point(const std::unordered_set<Point>& maze_paths) const;
    void solve_with_dfs_threads();
    void solve_with_bfs_threads();
    bool dfs_thread_search(Point start, Square thread_index);
    bool bfs_thread_search(Point start, Square thread_index);
    Point pick_random_point(std::uniform_int_distribution<int>& row,
                            std::uniform_int_distribution<int>& col);
    Point find_nearest_square(Point choice);
    Point find_nearest_wall(Point choice);
    void print_solution_path();
    void print_maze() const;
    void print_wall(int row, int col) const;
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

