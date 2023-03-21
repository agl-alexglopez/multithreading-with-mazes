#include <cstdint>
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

    /* Here is the scheme we will use to store tons of data in a square.
     *
     * wall structure-------------------|
     * path bit---------------------|   |
     * 0 thread------------------|  |   |
     * 1 thread-----------------||  |   |
     * 2 thread----------------|||  |   |
     * 3 thread --------------||||  |   |
     * start bit-----------|  ||||  |   |
     * finish bit---------||  ||||  | |----|
     *                  0b00  0000  0  0000
     *
     * More uses for the upper bits could arise in the future.
     */
    using Square = uint16_t;
    using Thread_tag = uint16_t;
    using Wall_line = uint8_t;

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

    static constexpr Wall_line wall_mask_ =     0b1111;
    static constexpr Wall_line floating_wall_ = 0b0000;
    static constexpr Wall_line north_wall_ =    0b0001;
    static constexpr Wall_line east_wall_ =     0b0010;
    static constexpr Wall_line south_wall_ =    0b0100;
    static constexpr Wall_line west_wall_ =     0b1000;
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

    static constexpr Square path_bit_ =     0b000'0001'0000;
    static constexpr Square start_bit_ =    0b010'0000'0000;
    static constexpr Square finish_bit_ =   0b100'0000'0000;

    static constexpr Thread_tag thread_tag_offset_ = 5;
    static constexpr Thread_tag thread_mask_ =  0b001'1110'0000;
    static constexpr Thread_tag zero_thread_ =  0b000'0010'0000;
    static constexpr Thread_tag one_thread_ =   0b000'0100'0000;
    static constexpr Thread_tag two_thread_ =   0b000'1000'0000;
    static constexpr Thread_tag three_thread_ = 0b001'0000'0000;
    static constexpr Thread_tag zero_one_overlap_ = zero_thread_ | one_thread_;
    static constexpr Thread_tag zero_two_overlap_ = zero_thread_ | two_thread_;
    static constexpr Thread_tag zero_three_overlap_ = zero_thread_ | three_thread_;
    static constexpr Thread_tag zero_one_two_overlap_ = zero_thread_ | one_thread_ | two_thread_;
    static constexpr Thread_tag zero_one_three_overlap_ = zero_thread_ | one_thread_ | three_thread_;
    static constexpr Thread_tag zero_two_three_overlap_ = zero_thread_ | two_thread_ | three_thread_;
    static constexpr Thread_tag zero_one_two_three_overlap_ = zero_thread_ | one_thread_ | two_thread_ | three_thread_;
    static constexpr Thread_tag one_two_overlap_ = one_thread_ | two_thread_;
    static constexpr Thread_tag one_three_overlap_ = one_thread_ | three_thread_;
    static constexpr Thread_tag one_two_three_overlap_ = one_thread_ | two_thread_ | three_thread_;
    static constexpr Thread_tag two_three_overlap_ = two_thread_ | three_thread_;
    static constexpr std::array<Thread_tag,4> thread_masks_ = {
        zero_thread_, one_thread_, two_thread_, three_thread_
    };
    static const std::unordered_map<Thread_tag, const char *const> thread_colors_;
    static const std::vector<std::pair<std::string,const char *const>> thread_overlap_key_;
    static const std::unordered_map<Thread_tag, char> thread_chars_;

    static constexpr int num_threads_ = 4;
    static constexpr int starting_path_len_ = 4096;
    static constexpr const char *const ansi_red_ = "\033[38;5;1m";
    static constexpr const char *const ansi_grn_ = "\033[38;5;2m";
    static constexpr const char *const ansi_yel_ = "\033[38;5;3m";
    static constexpr const char *const ansi_blu_ = "\033[38;5;4m";
    static constexpr const char *const ansi_prp_ = "\033[38;5;183m";
    static constexpr const char *const ansi_mag_ = "\033[38;5;201m";
    static constexpr const char *const ansi_cyn_ = "\033[38;5;87m";
    static constexpr const char *const ansi_wit_ = "\033[38;5;15m";
    static constexpr const char *const ansi_dark_red_ = "\033[38;5;160m";
    static constexpr const char *const ansi_blu_mag_ = "\033[38;5;105m";
    static constexpr const char *const ansi_red_grn_blu_ = "\033[38;5;121m";
    static constexpr const char *const ansi_grn_prp_ = "\033[38;5;59m";
    static constexpr const char *const ansi_grn_blu_prp_ = "\033[38;5;60m";
    static constexpr const char *const ansi_red_grn_prp_ = "\033[38;5;96m";
    static constexpr const char *const ansi_red_blu_prp_ = "\033[38;5;53m";
    static constexpr const char *const ansi_dark_blu_mag_ = "\033[38;5;57m";
    static constexpr const char *const ansi_nil_ = "\033[0m";
    static const std::unordered_map<Wall_line,std::string> wall_lines_;
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
    void build_path(int row, int col);
    void build_wall(int row, int col);
    Point choose_arbitrary_point(const std::unordered_set<Point>& maze_paths) const;
    void solve_with_dfs_threads();
    void solve_with_bfs_threads();
    bool bfs_thread_search(Point start, size_t thread_index, Thread_maze::Thread_tag thread_bit);
    bool dfs_thread_search(Point start, size_t thread_index, Thread_maze::Thread_tag thread_bit);
    Point pick_random_point(std::uniform_int_distribution<int>& row,
                            std::uniform_int_distribution<int>& col);
    Point find_nearest_square(Point choice);
    Point find_nearest_wall(Point choice);
    void print_solution_path();
    void print_maze() const;
    void print_overlap_key() const;
    void print_wall(Square square) const;
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

