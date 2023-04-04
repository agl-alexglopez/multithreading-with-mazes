#ifndef THREAD_SOLVERS_HH
#define THREAD_SOLVERS_HH
#include "thread_maze.hh"
#include <vector>
#include <array>
#include <mutex>

class Thread_solvers {

public:

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

    struct Solver_args {
        Maze_game game = Maze_game::hunt;
        Solver_algorithm solver = Solver_algorithm::depth_first_search;
        Solver_speed speed = Solver_speed::instant;
    };

    Thread_solvers(Thread_maze& maze, const Solver_args& args);
    void solve_maze();
    void print_overlap_key() const;


private:


    /* The maze builder is responsible for zeroing out the direction bits as part of the
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
    using Thread_paint = uint16_t;
    using Thread_cache = uint16_t;


    static constexpr Thread_paint start_bit_ =   0b0100'0000'0000'0000;
    static constexpr Thread_paint finish_bit_ =  0b1000'0000'0000'0000;
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

    static constexpr Thread_cache clear_cache_ = 0b0001'1111'1111'0000;
    static constexpr Thread_cache cache_mask_ =  0b1111'0000'0000;
    static constexpr Thread_cache zero_seen_ =   0b0001'0000'0000;
    static constexpr Thread_cache one_seen_ =    0b0010'0000'0000;
    static constexpr Thread_cache two_seen_ =    0b0100'0000'0000;
    static constexpr Thread_cache three_seen_ =  0b1000'0000'0000;

    static constexpr int starting_path_len_ = 4096;
    static constexpr const char *const ansi_clear_screen_ = "\033[2J\033[1;1H";
    static constexpr const char *const ansi_red_ = "\033[38;5;1m█\033[0m";
    static constexpr const char *const ansi_grn_ = "\033[38;5;2m█\033[0m";
    static constexpr const char *const ansi_yel_ = "\033[38;5;3m█\033[0m";
    static constexpr const char *const ansi_blu_ = "\033[38;5;4m█\033[0m";
    static constexpr const char *const ansi_prp_ = "\033[38;5;183m█\033[0m";
    static constexpr const char *const ansi_mag_ = "\033[38;5;201m█\033[0m";
    static constexpr const char *const ansi_cyn_ = "\033[38;5;87m█\033[0m";
    static constexpr const char *const ansi_wit_ = "\033[38;5;231m█\033[0m";
    static constexpr const char *const ansi_prp_red_ = "\033[38;5;204m█\033[0m";
    static constexpr const char *const ansi_blu_mag_ = "\033[38;5;105m█\033[0m";
    static constexpr const char *const ansi_red_grn_blu_ = "\033[38;5;121m█\033[0m";
    static constexpr const char *const ansi_grn_prp_ = "\033[38;5;106m█\033[0m";
    static constexpr const char *const ansi_grn_blu_prp_ = "\033[38;5;60m█\033[0m";
    static constexpr const char *const ansi_red_grn_prp_ = "\033[38;5;105m█\033[0m";
    static constexpr const char *const ansi_red_blu_prp_ = "\033[38;5;89m█\033[0m";
    static constexpr const char *const ansi_dark_blu_mag_ = "\033[38;5;57m█\033[0m";
    static constexpr const char *const ansi_bold_ = "\033[1m";
    static constexpr const char *const ansi_nil_ = "\033[0m";
    static constexpr const char *const ansi_finish_ = "\033[1m\033[38;5;87mF\033[0m";
    static constexpr const char *const ansi_start_ = "\033[1m\033[38;5;87mS\033[0m";
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
    static constexpr std::array<Thread_maze::Point,4> cardinal_directions_ = {
        //  n      e     s      w
        { {-1,0},{0,1},{1,0},{0,-1} }
    };
    static constexpr std::array<Thread_maze::Point,4> generate_directions_ = {
        //  n      e     s      w
        { {-2,0},{0,2},{2,0},{0,-2} }
    };
    static constexpr std::array<Thread_maze::Point,7> all_directions_ = {
        { {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1} }
    };
    static constexpr int overlap_key_and_message_height = 11;
    static constexpr std::array<int,8> solver_speeds_ = {
        0, 20000, 10000, 5000, 2000, 1000, 500, 250
    };

    Thread_maze& maze_;
    std::mutex solver_mutex_;
    Maze_game game_;
    Solver_algorithm solver_;
    int solver_speed_;
    Thread_maze::Point start_;
    Thread_maze::Point finish_;
    std::array<Thread_maze::Point,4> corner_starts_;
    int escape_path_index_;
    std::vector<std::vector<Thread_maze::Point>> thread_paths_;
    std::mt19937 generator_;
    std::uniform_int_distribution<int> row_random_;
    std::uniform_int_distribution<int> col_random_;

    void place_start_finish();
    void place_start_finish_animated();
    void solve_with_dfs_threads();
    void animate_with_dfs_threads();
    void solve_with_randomized_dfs_threads();
    void animate_with_randomized_dfs_threads();
    void solve_with_bfs_threads();
    void animate_with_bfs_threads();
    bool dfs_thread_hunt(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool dfs_thread_hunt_animated(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool randomized_dfs_thread_hunt(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool randomized_dfs_thread_hunt_animated(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool dfs_thread_gather(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool dfs_thread_gather_animated(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool randomized_dfs_thread_gather(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool randomized_dfs_thread_gather_animated(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool bfs_thread_hunt(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool bfs_thread_hunt_animated(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool bfs_thread_gather(Thread_maze::Point start, int thread_index, Thread_paint paint);
    bool bfs_thread_gather_animated(Thread_maze::Point start, int thread_index, Thread_paint paint);
    Thread_maze::Point pick_random_point();
    Thread_maze::Point find_nearest_square(Thread_maze::Point choice);
    void print_solver() const;
    void print_solution_path() const;
    void clear_paths();
    void print_point(int row, int col) const;
    void clear_and_flush_paths() const;
    void clear_screen() const;
    void flush_cursor_path_coordinate(int row, int col) const;
    void set_cursor_point(int row, int col) const;
    void print_maze() const;

}; // Thread_solvers

#endif
