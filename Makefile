# CS111 threads lectures Makefile Hooks
PROGS = run_maze
# CXX = /usr/bin/clang++-10

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

# In this section, you list the files that are part of the project.
SOURCES = $(PROGS:=.cc)
OBJECTS = $(SOURCES:.cc=.o)

all:: $(PROGS)

$(PROGS): %:%.cc thread_maze.cc
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

# Phony means not a "real" target, it doesn't build anything
# The phony target "clean" is used to remove all compiled object files.
# The phony target "spartan" is used to remove all compilation products and extra backup files.

clean::
	rm -f $(PROGS) $(OBJECTS)

spartan:: clean
	\rm -fr *~

.PHONY: all clean spartan
