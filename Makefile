# CS111 threads lectures Makefile Hooks
SRCDIR = src
BINDIR = bin
OBJDIR = obj
SRC := $(wildcard $(SRCDIR)/*.cc)
OBJ := $(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.o,$(SRC))
DEPENDENCIES = $(SRCDIR)/thread_maze.hh
# CXX = /usr/bin/clang++-10

PROGS = run_maze

# In this section, you list the files that are part of the project.
MAZE_PROG = $(addprefix $(BINDIR)/,$(PROGS))

all:: $(MAZE_PROG)

# The CPPFLAGS variable sets compile flags for g++:
#  -g          compile with debug information
#  -Wall       give all diagnostic warnings
#  -pedantic   require compliance with ANSI standard
#  -O0         do not optimize generated code
#  -std=c++0x  go with the c++0x experimental extensions for thread support (and other nifty things)
#  -D_GLIBCXX_USE_NANOSLEEP included for this_thread::sleep_for and this_thread::sleep_until support
#  -D_GLIBCXX_USE_SCHED_YIELD included for this_thread::yield support
CPPFLAGS = -g -Wall -pedantic -O0 -std=c++2a -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD

# The LDFLAGS variable sets flags for linker
#  -lm       link in libm (math library)
#  -lpthread link in libpthread (thread library) to back C++11 extensions
LDFLAGS = -lm -lpthread

$(OBJ): $(OBJDIR)/%.o:$(SRCDIR)/%.cc $(DEPENDENCIES)
	$(CXX) $(CPPFLAGS) -c -o $@ $<

$(MAZE_PROG): $(BINDIR)/%:$(OBJ)
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

# Phony means not a "real" target, it doesn't build anything
# The phony target "clean" is used to remove all compiled object files.
# The phony target "spartan" is used to remove all compilation products and extra backup files.

clean::
	rm -f $(MAZE_PROG) $(OBJ)

spartan:: clean
	\rm -fr *~

.PHONY: all clean spartan
