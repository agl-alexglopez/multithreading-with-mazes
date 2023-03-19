#include <iostream>
#include "thread_maze.hh"

using namespace std;


int main() {
    Thread_maze maze(55, 211);

    maze.solve_with_dfs_threads();

    return 0;
}

