# Maze Generators and Multithreaded Solvers

> **Note to All: In order to enjoy the mazes built and solved by this repository, you will need a terminal that supports ANSI escape sequences, 256 bit colors, and a Unicode compatible font installed for your terminal (see [Nerd Fonts](https://www.nerdfonts.com/)). Also, this project is intended to be built in a Unix-like environment, specifically I have tested it to work on Apple-ARM64, Apple-Intelx86-64, WSL2 for Windows, and PopOS for Linux. However, I make no promises for portability as this project is for my own personal enjoyment and is operating completely in terminal, without any 3rd party libraries. I will try to make the project more portable when time allows. Enjoy!**

![wilson-demo](/images/wilson-demo.png)

## Quick Start Guide

This project is a command line application that can be run with various combinations of commands. The basic principle behind the commands is that you can ask for any combination of settings, include any settings, exclude any settings, and the program will just work. There are sensible defaults for every flag so experiment with different combinations and tweaks until you get what you are looking for. To start, you should focus mainly on how big you want the maze to be, what algorithm you want to generate it, what algorithm you want to solve it, and if any of these algorithms should be animated in real time.

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

