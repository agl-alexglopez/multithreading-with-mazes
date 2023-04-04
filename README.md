# Maze Generators and Multithreaded Solvers

> **Note to All: In order to enjoy the mazes built and solved by this repository, you will need a terminal that supports ANSI escape sequences, 256 bit colors, and a Unicode compatible font installed for your terminal (see [Nerd Fonts](https://www.nerdfonts.com/)). Also, this project is intended to be built in a Unix-like environment, specifically I have tested it to work on Apple-ARM64, Apple-Intelx86-64, WSL2 for Windows, and PopOS for Linux. However, I make no promises for portability as this project is for my own personal enjoyment and is operating completely in terminal, without any 3rd party libraries. I will try to make the project more portable when time allows. Enjoy!**

![wilson-demo](/images/wilson-demo.png)

## Quick Start Guide

This project is a command line application that can be run with various combinations of commands. The basic principle behind the commands is that you can ask for any combination of settings, include any settings, exclude any settings, and the program will just work. There are sensible defaults for every flag so experiment with different combinations and tweaks until you get what you are looking for. To start, you should focus mainly on how big you want the maze to be, what algorithm you want to generate it, what algorithm you want to solve it, and if any of these algorithms should be animated in real time. For now, building the project is simple if you have a compiler that supports c++20. From the root directory of this project run make.

```zsh
$ make
```

Here is the help message that comes with the `-h` flag to get started.

Use flags, followed by arguments, in any order:

- `-r` Rows flag. Set rows for the maze.
	- Any number > 7. Zoom out for larger mazes!
- `-c` Columns flag. Set columns for the maze.
	- Any number > 7. Zoom out for larger mazes!
- `-b` Builder flag. Set maze building algorithm.
	- `rdfs` - Randomized Depth First Search.
	- `kruskal` - Randomized Kruskal's algorithm.
	- `prim` - Randomized Prim's algorithm.
	- `wilson` - Loop-Erased Random Path Carver.
	- `wilson-walls` - Loop-Erased Random Wall Adder.
	- `fractal` - Randomized recursive subdivision.
	- `grid` - A random grid pattern.
	- `arena` - Open floor with no walls.
- `-m` Modification flag. Add shortcuts to the maze.
	- `cross` - Add crossroads through the center.
	- `x` - Add an x of crossing paths through center.
- `-s` Solver flag. Set maze solving algorithm.
	- `dfs` - Depth First Search
	- `rdfs` - Randomized Depth First Search
	- `bfs` - Breadth First Search
- `-g` Game flag. Set the game for the solver threads to play.
	- `hunt` - 4 threads race to find one finish.
	- `gather` - 4 threads gather 4 finish squares.
	- `corners` - 4 threads race to the center.
- `-d` Draw flag. Set the line style for the maze.
	- `sharp` - The default straight lines.
	- `round` - Rounded corners.
	- `doubles` - Sharp double lines.
	- `bold` - Thicker straight lines.
	- `contrast` - Full block width and height walls.
	- `spikes` - Connected lines with spikes.
- `-sa` Solver Animation flag. Watch the maze solution.
	- Any number 1-7. Speed increases with number.
- `-ba` Builder Animation flag. Watch the maze build.
	- Any number 1-7. Speed increases with number.
- `-h` Help flag. Make this prompt appear.

If any flags are omitted, defaults are used.

Examples:

```zsh
./run_maze
./run_maze -r 51 -c 111 -b rdfs -s bfs -g hunt
./run_maze -c 111 -s bfs -g gather
./run_maze -s bfs -g corners -d round -b fractal
./run_maze -s dfs -ba 4 -sa 5 -b wilson-walls -m x
./run_maze -h
```

## Settings Detailed

### Row and Column

![dimension-showcase](/images/dimension-showcase.png)

The `-r` and `-c` flags let you set the dimensions of the maze. Note that my programs enforce that rows and columns must be odd, and it will enforce this by incrementing an even value, but this does not affect your usage of the program. As the image above demonstrates, zooming out with `<CTRL-(-)>`, or `CTRL-(+)` allows you to test huge mazes. Note that performance will decrease with size and WSL2 on Windows seems to struggle the most, while MacOS runs quite smoothly regardless of size. Try the `-d contrast` option as pictured if performance is an issue. This options seems to provide smooth performance on all tested platforms. 

### Builder Flag

The `-b` flag allows you to specify the algorithm that builds the maze. Maze generation is a deep and fascinating topic because it touches on so many interesting ideas, data structures, and implementation details. I will try to add a writeup for each algorithm in this repository in further detail below.

### Modification Flag

![modification-demo](/images/modification-demo.png)

The `-m` flag places user designated paths in a maze. Most algorithms in the maze generator produce *perfect* mazes. This means that there is a unique path between any two points in the maze, there are no loops, and all locations in the maze are reachable. We can completely ruin this concept by cutting a path through the maze, destroying all walls that lie in the path of our modification. This can create chaotic paths and overlaps between threads.

### Solver Flag

![rdfs-solver-demo](/images/rdfs-solver-demo.png)

The `-s` flag allows you to select the maze solver algorithm. The purpose of this repository is to explore how multithreading can apply to maze algorithms. So far, I have only implemented maze solvers that are multithreading, but I am looking forward to multithreading the maze generation algorithms that would support it. The options are simple for now with breadth and depth first search. However, randomized depth first search can provide interesting results on some maps, like the arena pictured above. As a bonus, breadth first search provides the shortest path for the winning thread, as highlighted in the title image in this repository, when threads are searching for one finish.

An important detail for the solvers is that you can trace the exact path of every thread due to my use of colors. Each thread has a unique color. When a thread walks along a maze path it will leave its color mark behind. If another thread crosses the same path, it will leave its color as well. This creates mixed colors that help you identify exactly where threads have gone in the maze. For depth first searches, I only have the threads paint the path they are currently on, not every square they have visited. This makes it easier to distinguish this algorithm from a depth first search that paints every seen maze square. If you are looking at static images, not the live animations, the solution you are seeing is a freeze frame of all the threads at the time the game is over: depth first search shows the current position of each thread and the path it took from the start to get there, and breadth first search shows every square visited by all threads at the time a game finishes. These colors create interesting results for the games they play.

### Game Flag

![games-showcase](/images/games-showcase.png)

The `-g` flag lets you determine the game that the thread solvers will play in the maze. 

The `hunt` game randomly places a start and a finish then sets the thread loose to see who finds it first.

The `gather` game forces every thread to find a finish square and will not stop until each has found their own. This is a colorful game to watch with a breadth first search solving algorithm.

The `corners` game places each thread in a corner of the maze and they all race to the center to get the finish square. This is a good test to make sure the mazes that the builder algorithms produce are perfect, especially when run with a breadth first search.

### Draw Flag

The `-d` flag determines the lines used to draw the maze. The walls are an interesting problem in this project and the way I chose to address walls has allowed me to easily implement both wall adder and path carver algorithms, which I am happy with. Unfortunately, Windows Terminal running WSL2 cannot perfectly connect the horizontal Unicode wall lines, but the result still looks good. MacOS and Linux distributions like PopOS draw everything perfectly and smoothly. You can try all the wall styles out to see which you like the most.

### Animation Flags

The `-ba` flag indicates the speed of the builder animation on a scale from 1-7. The `-sa` flag does the same for the solver animation. This allows you to decide how fast the build or solve process should run. Faster speeds are needed if you zoom out to draw very large mazes.

## Maze Generation Algorithms

When I started this project I was most interested in multithreading the maze solver algorithms. However, as I needed to come up with mazes for the threads to solve I found that the maze generation algorithms are far more interesting. There are even some algorithms in the collection that I think would be well suited for multithreading and I will definitely extend these when I get the chance. For the design of this project I gave myself some constraints and goals. They are as follows.

- No recursion. Many of the maze generation algorithms are recursive. However, I want to be able to produce arbitrarily large mazes as time and memory allows. So, any recursive algorithm must be re-implemented iteratively and produce the same traversals and space complexities as the recursive version. For example, the traditional depth first search generation or solver algorithm must be iterative, produce the same traversal order as a recursive depth first search, and have the same O(D) space complexity in its stack, where D is the current depth/path of the search. This means that the commonly taught depth first search using a stack that pushes all valid neighboring cells onto a stack before proceeding to the next level does not satisfy these constraints.
- Unique maze generation animations. When these algorithms are visualized and animated they should create visuals that are distinct from other generation algorithms. This means that the selection of generators should be broad and have good variety in their implementation details.
- Waste less space. I try my best to use the least amount of space possible, relying on what the maze already provides. Threads already are expensive in terms of space and any data structures they maintain. This means that bit manipulations and encodings are essential to this implementation. Whenever possible we should use the maze itself to store the information we need. I know there is some room for improvement here in many of my generators such as Kruskal's and Prim's. However, I think I have sufficiently met this goal in the recursive depth first search generator and both variations on Wilson's algorithm. Both of these only require O(1) auxiliary space to run.

### Randomized Recursive Depth First Search

### Kruskal's Algorithm

### Prim's Algorithm

### Wilson's Path Carver

### Wilson's Wall Adder

### Randomized Recursive Subdivision

### Randomized Grid

### Arena
