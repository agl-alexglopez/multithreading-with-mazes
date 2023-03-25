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
        arena,
    };

    enum class Maze_modification {
        none,
        add_cross,
        add_x
    };

    enum class Solver_algorithm {
        depth_first_search,
        randomized_depth_first_search,
        breadth_first_search,
    };

    /* -hunt - 4 threads compete from one starting point to find the finish.
     * -gather - 4 threads work together, communicating to gather 4 finish squares.
     */
    enum class Maze_game {
        hunt,
        gather,
    };

    enum class Maze_style {
        standard=0,
        rounded,
    };

    struct Packaged_args {
        size_t odd_rows = 31;
        size_t odd_cols = 111;
        Builder_algorithm builder = Builder_algorithm::randomized_depth_first;
        Maze_modification modification = Maze_modification::none;
        Solver_algorithm solver = Solver_algorithm::depth_first_search;
        Maze_game game = Maze_game::hunt;
        Maze_style style = Maze_style::standard;
    };

    struct Point {
        int row;
        int col;
    };

    /* Here is the scheme we will use to store tons of data in a square.
     *
     * wall structure----------------------||||
     * ------------------------------------||||
     * 0 thread paint--------------------| ||||
     * 1 thread paint-------------------|| ||||
     * 2 thread paint------------------||| ||||
     * 3 thread paint-----------------|||| ||||
     * -------------------------------|||| ||||
     * 0 thread cache---------------| |||| ||||
     * 1 thread cache--------------|| |||| ||||
     * 2 thread cache-------------||| |||| ||||
     * 3 thread cache------------|||| |||| ||||
     * --------------------------|||| |||| ||||
     * maze build bit----------| |||| |||| ||||
     * maze paths bit---------|| |||| |||| ||||
     * maze start bit--------||| |||| |||| ||||
     * maze goals bit-------|||| |||| |||| ||||
     *                    0b0000 0000 0000 0000
     */
    using Square = uint16_t;
    using Wall_line = uint8_t;
    using Thread_paint = uint16_t;
    using Thread_cache = uint16_t;

    Thread_maze(const Packaged_args& args);

    void solve_maze(Solver_algorithm solver);
    void solve_maze();

    void clear_paths();
    void new_maze();
    void new_maze(Builder_algorithm builder, Maze_game game, size_t odd_rows, size_t odd_cols);
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

    static constexpr std::array<const std::array<const char *const,16>,2> wall_styles_ = {
        {{
            // Walls are drawn in relation to neighboring walls in cardinal directions.
            // 0bWestSouthEastNorth
            // 0b0000  0b0001  0b0010  0b0011  0b0100  0b0101  0b0110  0b0111
                "┼",    "╵",     "╶",    "└",    "╷",    "│",    "┌",    "├",
            // 0b1000  0b1001  0b1010  0b1011  0b1100  0b1101  0b1110  0b1111
                "╴",    "┘",     "─",    "┴",    "┐",    "┤",    "┬",    "┼"
        },
        {
            // Same but with rounded corners.
            "┼",    "╵",     "╶",    "╰",    "╷",    "│",    "╭",    "├",
            "╴",    "╯",     "─",    "┴",    "╮",    "┤",    "┬",    "┼"
        }}
    };

    static constexpr Square builder_bit_ = 0b0001'0000'0000'0000;
    static constexpr Square path_bit_ =    0b0010'0000'0000'0000;
    static constexpr Square start_bit_ =   0b0100'0000'0000'0000;
    static constexpr Square finish_bit_ =  0b1000'0000'0000'0000;


    static constexpr int num_threads_ = 4;
    static constexpr Thread_paint thread_tag_offset_ = 4;
    static constexpr Thread_paint thread_mask_ =  0b1111'0000;
    static constexpr Thread_paint zero_thread_ =  0b0001'0000;
    static constexpr Thread_paint one_thread_ =   0b0010'0000;
    static constexpr Thread_paint two_thread_ =   0b0100'0000;
    static constexpr Thread_paint three_thread_ = 0b1000'0000;
    static constexpr std::array<Thread_paint,4> thread_masks_ = {
        zero_thread_ , one_thread_ , two_thread_ , three_thread_
    };

    static constexpr Square clear_cache_ = 0b0001'1111'1111'0000;

    static constexpr Thread_cache cache_mask_ =  0b1111'0000'0000;
    static constexpr Thread_cache zero_seen_ =   0b0001'0000'0000;
    static constexpr Thread_cache one_seen_ =    0b0010'0000'0000;
    static constexpr Thread_cache two_seen_ =    0b0100'0000'0000;
    static constexpr Thread_cache three_seen_ =  0b1000'0000'0000;

    static constexpr int starting_path_len_ = 4096;
    static constexpr const char *const ansi_red_ = "\033[38;5;1m";
    static constexpr const char *const ansi_grn_ = "\033[38;5;2m";
    static constexpr const char *const ansi_yel_ = "\033[38;5;3m";
    static constexpr const char *const ansi_blu_ = "\033[38;5;4m";
    static constexpr const char *const ansi_prp_ = "\033[38;5;183m";
    static constexpr const char *const ansi_mag_ = "\033[38;5;201m";
    static constexpr const char *const ansi_cyn_ = "\033[38;5;87m";
    static constexpr const char *const ansi_wit_ = "\033[38;5;231m";
    static constexpr const char *const ansi_dark_red_ = "\033[38;5;52m";
    static constexpr const char *const ansi_blu_mag_ = "\033[38;5;105m";
    static constexpr const char *const ansi_red_grn_blu_ = "\033[38;5;121m";
    static constexpr const char *const ansi_grn_prp_ = "\033[38;5;106m";
    static constexpr const char *const ansi_grn_blu_prp_ = "\033[38;5;60m";
    static constexpr const char *const ansi_red_grn_prp_ = "\033[38;5;105m";
    static constexpr const char *const ansi_red_blu_prp_ = "\033[38;5;89m";
    static constexpr const char *const ansi_dark_blu_mag_ = "\033[38;5;57m";
    static constexpr const char *const ansi_bold_ = "\033[1m";
    static constexpr const char *const ansi_nil_ = "\033[0m";
    static constexpr std::array<const char *const,16> thread_colors_ = {
        nullptr,
        // Threads Overlaps. The zero thread is the zero index bit with a value of 1.
        // 0b0001   0b0010     0b0011     0b0100     0b0101     0b0110        0b0111
        ansi_red_, ansi_grn_, ansi_yel_, ansi_blu_, ansi_mag_, ansi_cyn_, ansi_red_grn_blu_,
        // 0b1000    0b1001          0b1010           0b1011            0b1100
        ansi_prp_, ansi_dark_red_, ansi_grn_prp_, ansi_red_grn_prp_, ansi_dark_blu_mag_,
        // 0b1101              0b1110          0b1111
        ansi_red_blu_prp_, ansi_grn_blu_prp_, ansi_wit_,
    };
    //                                                               n      e     s      w
    static constexpr std::array<Point,4> cardinal_directions_ = { {{-1,0},{0,1},{1,0},{0,-1}} };
    static constexpr std::array<Point,4> generate_directions_ = { {{-2,0},{0,2},{2,0},{0,-2}} };
    static constexpr std::array<Point,7> all_directions_ = {
        { {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1} }
    };

    Builder_algorithm builder_;
    Maze_modification modification_;
    Solver_algorithm solver_;
    Maze_game game_;
    Maze_style style_;
    std::vector<std::vector<Square>> maze_;
    int maze_row_size_;
    int maze_col_size_;
    std::mt19937 generator_;
    std::uniform_int_distribution<int> row_random_;
    std::uniform_int_distribution<int> col_random_;
    std::vector<std::vector<Point>> thread_paths_;
    Point start_;
    Point finish_;
    int escape_path_index_;
    // I would rather have a maze of atomic ints, but I can't construct atomics at runtime.
    std::mutex maze_mutex_;
    void generate_maze(Builder_algorithm algorithm, Maze_game game);
    void generate_randomized_dfs_maze(Maze_game game);
    void generate_randomized_loop_erased_maze(Maze_game game);
    void build_path(int row, int col);
    void build_wall(int row, int col);
    void add_modification(int row, int col);
    Point choose_arbitrary_point() const;
    void solve_with_dfs_threads();
    void solve_with_randomized_dfs_threads();
    void solve_with_bfs_threads();
    bool dfs_thread_hunt(Point start, int thread_index, Thread_maze::Thread_paint thread_bit);
    bool randomized_dfs_thread_hunt(Point start, int thread_index, Thread_paint thread_bit);
    bool dfs_thread_gather(Point start, int thread_index, Thread_maze::Thread_paint thread_bit);
    bool randomized_dfs_thread_gather(Point start, int thread_index, Thread_paint thread_bit);
    bool bfs_thread_hunt(Point start, int thread_index, Thread_maze::Thread_paint thread_bit);
    bool bfs_thread_gather(Point start, int thread_index, Thread_maze::Thread_paint thread_bit);
    void place_start_finish();
    Point pick_random_point();
    Point find_nearest_square(Point choice);
    Point find_nearest_wall(Point choice);
    void print_solution_path();
    void print_maze() const;
    void print_overlap_key() const;
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

