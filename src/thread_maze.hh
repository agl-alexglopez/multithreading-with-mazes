#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <vector>
#include <stack>
#include <array>


class Thread_maze {

public:

    enum class Builder_algorithm {
        randomized_depth_first,
        randomized_loop_erased,
        randomized_fractal,
        randomized_grid,
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

    enum class Maze_game {
        hunt,
        gather,
        corners,
    };

    enum class Maze_style {
        sharp=0,
        round,
        doubles,
        bold,
        contrast,
        spikes,
    };

    enum class Builder_speed {
        instant=0,
        speed_1,
        speed_2,
        speed_3,
        speed_4,
        speed_5,
        speed_6,
        speed_7,
    };

    enum class Solver_speed {
        instant,
        speed_1,
        speed_2,
        speed_3,
        speed_4,
        speed_5,
        speed_6,
        speed_7,
    };

    struct Maze_args {
        size_t odd_rows = 31;
        size_t odd_cols = 111;
        Builder_algorithm builder = Builder_algorithm::randomized_depth_first;
        Maze_modification modification = Maze_modification::none;
        Solver_algorithm solver = Solver_algorithm::depth_first_search;
        Maze_game game = Maze_game::hunt;
        Maze_style style = Maze_style::sharp;
        Solver_speed solver_speed = Solver_speed::instant;
        Builder_speed builder_speed = Builder_speed::instant;
    };

    struct Point {
        int row;
        int col;
    };

    /* Here is the scheme we will use to store tons of data in a square.
     *
     * When building the maze here is how we will use the available bits.
     *
     * wall structure----------------------||||
     * ------------------------------------||||
     * 0 backtrack marker bit------------| ||||
     * 1 backtrack marker bit ----------|| ||||
     * 2 backtrack marker bit----------||| ||||
     * 3 unused-----------------------|||| ||||
     * -------------------------------|||| ||||
     * 0 unused bit-----------------| |||| ||||
     * 1 unused bit----------------|| |||| ||||
     * 2 unused bit---------------||| |||| ||||
     * 3 unused bit--------------|||| |||| ||||
     * --------------------------|||| |||| ||||
     * maze build bit----------| |||| |||| ||||
     * maze paths bit---------|| |||| |||| ||||
     * maze start bit--------||| |||| |||| ||||
     * maze goals bit-------|||| |||| |||| ||||
     *                    0b0000 0000 0000 0000
     *
     * The maze builder is responsible for zeroing out the direction bits as part of the
     * building process. When solving the maze we adjust how we use the middle bits.
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
    using Wall_line = uint16_t;
    using Backtrack_marker = uint16_t;
    using Thread_paint = uint16_t;
    using Thread_cache = uint16_t;

    Thread_maze(const Maze_args& args);

    void solve_maze(Solver_algorithm solver);
    void solve_maze();

    void clear_paths();
    void new_maze();
    void new_maze(Builder_algorithm builder, Maze_game game, size_t odd_rows, size_t odd_cols);
    size_t size() const;
    std::vector<Square>& operator[](size_t index);
    const std::vector<Square>& operator[](size_t index) const;

private:

    enum class Wilson_point {
        even,
        odd,
    };

    using Height = int;
    using Width = int;

    static constexpr int marker_shift_ = 4;
    static constexpr Backtrack_marker markers_mask_ = 0b1111'0000;
    static constexpr Backtrack_marker is_origin_ =    0b0000'0000;
    static constexpr Backtrack_marker from_north_ =   0b0001'0000;
    static constexpr Backtrack_marker from_east_ =    0b0010'0000;
    static constexpr Backtrack_marker from_south_ =   0b0011'0000;
    static constexpr Backtrack_marker from_west_ =    0b0100'0000;
    static constexpr std::array<Point,5> backtracking_marks_ = {
        {{0,0}, {-2,0}, {0,2}, {2,0}, {0,-2},}
    };

    static constexpr Wall_line wall_mask_ =     0b1111;
    static constexpr Wall_line floating_wall_ = 0b0000;
    static constexpr Wall_line north_wall_ =    0b0001;
    static constexpr Wall_line east_wall_ =     0b0010;
    static constexpr Wall_line south_wall_ =    0b0100;
    static constexpr Wall_line west_wall_ =     0b1000;

    /* Walls are constructed in terms of other walls they need to connect to. For example, read
     * 0b0011 as, "this is a wall square that must connect to other walls to the East and North."
     */
    static constexpr std::array<const std::array<const char *const,16>,6> wall_styles_ = {{
        {
            // 0bWestSouthEastNorth. Note: 0b0000 is a floating wall with no walls around.
            // 0b0000  0b0001  0b0010  0b0011  0b0100  0b0101  0b0110  0b0111
                "■",    "╵",     "╶",    "└",    "╷",    "│",    "┌",    "├",
            // 0b1000  0b1001  0b1010  0b1011  0b1100  0b1101  0b1110  0b1111
                "╴",    "┘",     "─",    "┴",    "┐",    "┤",    "┬",    "┼"
        },
        {
            // Same but with rounded corners.
                "●",    "╵",     "╶",    "╰",    "╷",    "│",    "╭",    "├",
                "╴",    "╯",     "─",    "┴",    "╮",    "┤",    "┬",    "┼"
        },
        {
            // Same but with double lines.
                "◫",    "║",     "═",    "╚",    "║",    "║",    "╔",    "╠",
                "═",    "╝",     "═",    "╩",    "╗",    "╣",    "╦",    "╬"
        },
        {
            // Same but with bold lines.
                "■",    "╹",     "╺",    "┗",    "╻",    "┃",    "┏",    "┣",
                "╸",    "┛",     "━",    "┻",    "┓",    "┫",    "┳",    "╋"
        },
        {
            // Simpler approach that creates well-connected high contrast black/white.
                "█",    "█",     "█",    "█",    "█",    "█",    "█",    "█",
                "█",    "█",     "█",    "█",    "█",    "█",    "█",    "█",
        },
        {
            // Same but with crosses that create spikes.
                "✸",    "╀",     "┾",    "╄",    "╁",    "╂",    "╆",    "╊",
                "┽",    "╃",     "┿",    "╇",    "╅",    "╉",    "╈",    "╋"
        },
    }};

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
    static constexpr const char *const ansi_prp_red_ = "\033[38;5;204m";
    static constexpr const char *const ansi_blu_mag_ = "\033[38;5;105m";
    static constexpr const char *const ansi_red_grn_blu_ = "\033[38;5;121m";
    static constexpr const char *const ansi_grn_prp_ = "\033[38;5;106m";
    static constexpr const char *const ansi_grn_blu_prp_ = "\033[38;5;60m";
    static constexpr const char *const ansi_red_grn_prp_ = "\033[38;5;105m";
    static constexpr const char *const ansi_red_blu_prp_ = "\033[38;5;89m";
    static constexpr const char *const ansi_dark_blu_mag_ = "\033[38;5;57m";
    static constexpr const char *const ansi_bold_ = "\033[1m";
    static constexpr const char *const ansi_nil_ = "\033[0m";
    static constexpr const char *const ansi_start_ = "\033[38;5;87mS";
    static constexpr const char *const ansi_clear_screen = "\033[2J\033[1;1H";
    static constexpr std::array<const char *const,16> thread_colors_ = {
        ansi_nil_,
        // Threads Overlaps. The zero thread is the zero index bit with a value of 1.
        // 0b0001   0b0010     0b0011     0b0100     0b0101     0b0110        0b0111
        ansi_red_, ansi_grn_, ansi_yel_, ansi_blu_, ansi_mag_, ansi_cyn_, ansi_red_grn_blu_,
        // 0b1000    0b1001          0b1010           0b1011            0b1100
        ansi_prp_, ansi_prp_red_, ansi_grn_prp_, ansi_red_grn_prp_, ansi_dark_blu_mag_,
        // 0b1101              0b1110          0b1111
        ansi_red_blu_prp_, ansi_grn_blu_prp_, ansi_wit_,
    };
    //                                                               n      e     s      w
    static constexpr std::array<Point,4> cardinal_directions_ = { {{-1,0},{0,1},{1,0},{0,-1}} };
    static constexpr std::array<Point,4> generate_directions_ = { {{-2,0},{0,2},{2,0},{0,-2}} };
    static constexpr std::array<Point,7> all_directions_ = {
        { {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1} }
    };

    static constexpr int overlap_key_and_message_height = 17;
    static constexpr std::array<int,8> solver_speeds_ = {
        0, 20000, 10000, 5000, 2000, 1000, 500, 250
    };

    static constexpr std::array<int,8> builder_speeds_ = {
        0, 10000, 5000, 2500, 1000, 500, 250, 100
    };

    Builder_algorithm builder_;
    Maze_modification modification_;
    Solver_algorithm solver_;
    Maze_game game_;
    Maze_style style_;
    int solver_speed_;
    int builder_speed_;
    std::vector<std::vector<Square>> maze_;
    int maze_row_size_;
    int maze_col_size_;
    std::mt19937 generator_;
    std::uniform_int_distribution<int> row_random_;
    std::uniform_int_distribution<int> col_random_;
    std::vector<std::vector<Point>> thread_paths_;
    Point start_;
    Point finish_;
    std::array<Point,4> corner_starts_;
    int escape_path_index_;
    // I would rather have a maze of atomic ints, but I can't construct atomics at runtime.
    std::mutex maze_mutex_;
    void generate_maze(Builder_algorithm algorithm, Maze_game game);
    void generate_randomized_dfs_maze();
    void generate_randomized_dfs_maze_animated();
    void generate_randomized_fractal_maze();
    void generate_randomized_fractal_maze_animated();
    void generate_randomized_loop_erased_maze();
    void generate_randomized_loop_erased_maze_animated();
    Point choose_arbitrary_point(Wilson_point start) const;
    void carve_path_walls(int row, int col);
    void carve_path_walls_animated(int row, int col);
    void carve_path_markings(const Point& cur, const Point& next);
    void carve_path_markings_animated(const Point& cur, const Point& next);
    void generate_randomized_grid();
    void generate_randomized_grid_animated();
    void generate_arena();
    void generate_arena_animated();
    void join_squares(const Point& cur, const Point& next);
    void join_squares_animated(const Point& cur, const Point& next);
    void build_path(int row, int col);
    void build_path_animated(int row, int col);
    void build_wall(int row, int col);
    void add_modification(int row, int col);
    void add_modification_animated(int row, int col);
    void solve_with_dfs_threads();
    void animate_with_dfs_threads();
    void solve_with_randomized_dfs_threads();
    void animate_with_randomized_dfs_threads();
    void solve_with_bfs_threads();
    void animate_with_bfs_threads();
    bool dfs_thread_hunt(Point start, int thread_index, Thread_maze::Thread_paint paint);
    bool dfs_thread_hunt_animated(Point start, int thread_index, Thread_maze::Thread_paint paint);
    bool randomized_dfs_thread_hunt(Point start, int thread_index, Thread_paint paint);
    bool randomized_dfs_thread_hunt_animated(Point start, int thread_index, Thread_paint paint);
    bool dfs_thread_gather(Point start, int thread_index, Thread_maze::Thread_paint paint);
    bool dfs_thread_gather_animated(Point start, int thread_index, Thread_maze::Thread_paint paint);
    bool randomized_dfs_thread_gather(Point start, int thread_index, Thread_paint paint);
    bool randomized_dfs_thread_gather_animated(Point start, int thread_index, Thread_paint paint);
    bool bfs_thread_hunt(Point start, int thread_index, Thread_maze::Thread_paint paint);
    bool bfs_thread_hunt_animated(Point start, int thread_index, Thread_maze::Thread_paint paint);
    bool bfs_thread_gather(Point start, int thread_index, Thread_maze::Thread_paint paint);
    bool bfs_thread_gather_animated(Point start, int thread_index, Thread_maze::Thread_paint paint);
    void place_start_finish();
    void place_start_finish_animated();
    Point pick_random_point();
    Point find_nearest_square(Point choice);
    void print_solution_path();
    void clear_and_flush_grid() const;
    void clear_screen() const;
    void flush_cursor_maze_coordinate(int row, int col) const;
    void set_cursor_position(int row, int col) const;
    void print_square(int row, int col) const;
    void print_builder() const;
    void print_solver() const;
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

